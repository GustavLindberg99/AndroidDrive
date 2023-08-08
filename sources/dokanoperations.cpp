#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include "androiddevice.h"
#include "dokanoperations.h"
#include "helperfunctions.h"
#include "settingswindow.h"

NTSTATUS DOKAN_CALLBACK createFile(LPCWSTR fileName, PDOKAN_IO_SECURITY_CONTEXT, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG, ULONG createDisposition, ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = windowsPathToAndroidPath(fileName);

    ACCESS_MASK genericDesiredAccess;
    DWORD fileAttributesAndFlags, creationDisposition;
    DokanMapKernelToUserCreateFileFlags(desiredAccess, fileAttributes, createOptions, createDisposition, &genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);

    const QString output = device->runAdbCommand(QString("(test -d %1 && echo d) || (test -f %1 && echo f)").arg(escapeSpecialCharactersForBash(filePath)));
    const bool directoryExists = output == "d";
    const bool fileExists = output == "f";

    if(directoryExists){
        if(createOptions & FILE_NON_DIRECTORY_FILE){
            return STATUS_FILE_IS_A_DIRECTORY;
        }
        dokanFileInfo->IsDirectory = true;
    }

    if(dokanFileInfo->IsDirectory){
        if(fileExists){
            return STATUS_NOT_A_DIRECTORY;
        }
        if(creationDisposition == CREATE_NEW || creationDisposition == OPEN_ALWAYS){
            //Create a new directory
            if(fileExists){
                return STATUS_NOT_A_DIRECTORY;
            }
            else if(directoryExists){
                return STATUS_OBJECT_NAME_COLLISION;
            }
            bool ok;
            device->runAdbCommand(QString("mkdir %1").arg(escapeSpecialCharactersForBash(filePath)), &ok, false);
            return ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
        }
        else if(!directoryExists){
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }
    else{
        QByteArray *byteArray;    //This byte array will contain the contents of the file, and then a last byte which is 1 if it's been modified and 0 if it hasn't. The last byte is useful for performance reasons, so that we don't have to push the file each time a file is opened for reading.
        if(createDisposition == CREATE_ALWAYS){
            if(fileExists){
                return STATUS_OBJECT_NAME_COLLISION;
            }
            bool ok;
            device->runAdbCommand(QString("touch %1").arg(escapeSpecialCharactersForBash(filePath)), &ok, false);
            if(!ok){
                return STATUS_UNSUCCESSFUL;
            }
            byteArray = new QByteArray("");    //Create an empty byte array with one last byte equal to 0 indicating it hasn't been modified. So initialize it to {'\0'}, except the compiler won't accept that syntax, so use an empty string with an implicit null terminator instead.
        }
        else if(!fileExists){
            return STATUS_OBJECT_PATH_NOT_FOUND;
        }
        else{
            const QString temporaryFilePath = getTemporaryFilePath();
            if(!device->pullFromAdb(filePath, temporaryFilePath)){
                return STATUS_UNSUCCESSFUL;
            }
            QFile file(temporaryFilePath);
            if(!file.open(QIODevice::ReadOnly)){
                return STATUS_ACCESS_DENIED;
            }
            byteArray = new QByteArray(file.readAll());
            byteArray->append('\0');    //Append a zero byte to indicate that it hasn't been modified. Use '\0' instead of just 0 because just 0 can be interpreted as a null pointer which can be passed to another overload.
            file.close();
            file.remove();
        }
        dokanFileInfo->Context = reinterpret_cast<ULONG64>(byteArray);
    }
    return STATUS_SUCCESS;
}

void DOKAN_CALLBACK closeFile(LPCWSTR, PDOKAN_FILE_INFO dokanFileInfo){
    QByteArray *byteArray = reinterpret_cast<QByteArray*>(dokanFileInfo->Context);

    if(byteArray != nullptr){
        delete byteArray;
        dokanFileInfo->Context = 0;
    }
}

void DOKAN_CALLBACK cleanup(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo){
    if(dokanFileInfo->DeleteOnClose){
        deleteFile(fileName, dokanFileInfo);
    }
    else{
        QByteArray *byteArray = reinterpret_cast<QByteArray*>(dokanFileInfo->Context);
        if(byteArray != nullptr){
            pushQByteArrayToAdb(byteArray, fileName, dokanFileInfo);
        }
    }
}

NTSTATUS DOKAN_CALLBACK readFile(LPCWSTR, LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo){
    const QByteArray *byteArray = reinterpret_cast<QByteArray*>(dokanFileInfo->Context);
    if(byteArray == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    for(*readLength = 0; *readLength < bufferLength && *readLength + offset < byteArray->size() - 1; (*readLength)++){    //byteArray->size() - 1 because we don't want to read the last byte that's used to indicate if it has been modified
        static_cast<char*>(buffer)[*readLength] = byteArray->at(*readLength + offset);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK writeFile(LPCWSTR, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo){
    QByteArray *byteArray = reinterpret_cast<QByteArray*>(dokanFileInfo->Context);
    if(byteArray == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    if(offset + numberOfBytesToWrite > byteArray->size() - 1){
        byteArray->resize(offset + numberOfBytesToWrite + 1);
    }
    byteArray->replace(byteArray->size() - 1, 1, "\1", 1);    //Change the last byte to 1 to indicate that it's been modified (for some reason just doing {1} doesn't work, so we need to do "\1" which is equivalent to {1, 0} and tell it to ignore the zero with the last argument)
    byteArray->replace(offset, numberOfBytesToWrite, static_cast<const char*>(buffer), numberOfBytesToWrite);
    *numberOfBytesWritten = numberOfBytesToWrite;

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK flushFileBuffers(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo){
    const QByteArray *byteArray = reinterpret_cast<QByteArray*>(dokanFileInfo->Context);
    if(byteArray == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    return pushQByteArrayToAdb(byteArray, fileName, dokanFileInfo);
}

NTSTATUS DOKAN_CALLBACK getFileInformation(LPCWSTR fileName, LPBY_HANDLE_FILE_INFORMATION handleFileInformation, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = windowsPathToAndroidPath(fileName);

    bool ok;
    const QString fileInfo = device->runAdbCommand(QString("stat -c \"%F %s %W %Y %X\" %1").arg(escapeSpecialCharactersForBash(filePath)), &ok);    //All devices don't support stat --format, so we have to use stat -c
    if(!ok){
        return STATUS_UNSUCCESSFUL;
    }

    const QRegularExpressionMatch match = QRegularExpression("^([A-Za-z\\s]+) ([0-9]+) ([0-9?]+) ([0-9?]+) ([0-9?]+)$").match(fileInfo);
    if(!match.hasMatch()){
        return STATUS_UNSUCCESSFUL;
    }

    const bool isDirectory = match.captured(1) == "directory";
    LARGE_INTEGER fileSize;
    fileSize.QuadPart = isDirectory ? 0 : match.captured(2).toLongLong();
    FILETIME unknownTime;
    unknownTime.dwHighDateTime = unknownTime.dwLowDateTime = 0;
    bool creationTimeKnown;
    const FILETIME creationTime = unixTimeToMicrosftTime(match.captured(3).toLongLong(&creationTimeKnown));
    bool lastWriteTimeKnown;
    const FILETIME lastWriteTime = unixTimeToMicrosftTime(match.captured(4).toLongLong(&lastWriteTimeKnown));
    bool lastAccessTimeKnown;
    const FILETIME lastAccessTime = unixTimeToMicrosftTime(match.captured(5).toLongLong(&lastAccessTimeKnown));

    handleFileInformation->dwFileAttributes = 0;
    if(isDirectory){
        handleFileInformation->dwFileAttributes |= Attribute::Directory;
    }
    if(filePath.split("/").last().startsWith(".") && Settings().hideDotFiles()){
        handleFileInformation->dwFileAttributes |= Attribute::Hidden;
    }
    handleFileInformation->ftCreationTime = creationTimeKnown ? creationTime : unknownTime;
    handleFileInformation->ftLastWriteTime = lastWriteTimeKnown ? lastWriteTime : unknownTime;
    handleFileInformation->ftLastAccessTime = lastAccessTimeKnown ? lastAccessTime : unknownTime;
    handleFileInformation->nFileSizeHigh = fileSize.HighPart;
    handleFileInformation->nFileSizeLow = fileSize.LowPart;

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK findFiles(LPCWSTR fileName, PFillFindData fillFindData, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = windowsPathToAndroidPath(fileName);

    /* Format sequences used for stat:
     * %F = File type ("directory" for directories, something else for files)
     * %s = File size in bytes
     * %W = Unix timestamp in seconds for creation time
     * %Y = Unix timestamp in seconds for last write time
     * %X = Unix timestamp in seconds for last access time
     * %n = File path
     */
    bool ok;
    const QStringList output = device->runAdbCommand(QString("stat --format=\"%F %s %W %Y %X %n\" %1/* %1/.* || test -d %1").arg(escapeSpecialCharactersForBash(filePath)), &ok).split(QRegularExpression("[\r\n]+"));    //The || test -d is necessary so that it doesn't return an error for empty directories (otherwise it will give an error like "Can't find /sdcard/emptydirectory/*" since there are no files that match that pattern), but so that it still returns an error if the directory doesn't exist at all.
    if(!ok){
        return STATUS_UNSUCCESSFUL;
    }

    const bool hideDotFiles = Settings().hideDotFiles();
    for(const QString &fileInfo: output){
        const QRegularExpressionMatch match = QRegularExpression("^([A-Za-z\\s]+) ([0-9]+) ([0-9?]+) ([0-9?]+) ([0-9?]+) (.+)$").match(fileInfo);
        if(!match.hasMatch()){
            continue;
        }

        const bool isDirectory = match.captured(1) == "directory";
        LARGE_INTEGER fileSize;
        fileSize.QuadPart = isDirectory ? 0 : match.captured(2).toLongLong();
        FILETIME unknownTime;
        unknownTime.dwHighDateTime = unknownTime.dwLowDateTime = 0;
        bool creationTimeKnown;
        const FILETIME creationTime = unixTimeToMicrosftTime(match.captured(3).toLongLong(&creationTimeKnown));
        bool lastWriteTimeKnown;
        const FILETIME lastWriteTime = unixTimeToMicrosftTime(match.captured(4).toLongLong(&lastWriteTimeKnown));
        bool lastAccessTimeKnown;
        const FILETIME lastAccessTime = unixTimeToMicrosftTime(match.captured(5).toLongLong(&lastAccessTimeKnown));
        const QString subfileName = androidFileNameToWindowsFileName(match.captured(6).split("/").last());

        WIN32_FIND_DATAW findData;
        findData.dwFileAttributes = 0;
        if(isDirectory){
            findData.dwFileAttributes |= Attribute::Directory;
        }
        if(hideDotFiles && subfileName.startsWith(".")){
            findData.dwFileAttributes |= Attribute::Hidden;
        }
        findData.ftCreationTime = creationTimeKnown ? creationTime : unknownTime;
        findData.ftLastWriteTime = lastWriteTimeKnown ? lastWriteTime : unknownTime;
        findData.ftLastAccessTime = lastAccessTimeKnown ? lastAccessTime : unknownTime;
        findData.nFileSizeHigh = fileSize.HighPart;
        findData.nFileSizeLow = fileSize.LowPart;
        findData.cFileName[subfileName.toWCharArray(findData.cFileName)] = L'\0';    //toWCharArray does most of the work, but it doesn't add a null terminator, so add one manually at the end of the string (the length of the string is returned by toWCharArray)
        findData.cAlternateFileName[0] = L'\0';    //There is no alternate file name, so just set the first byte to a null terminator to indicate an empty string
        fillFindData(&findData, dokanFileInfo);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK setFileTime(LPCWSTR fileName, const FILETIME *creationTime, const FILETIME *lastAccessTime, const FILETIME *lastWriteTime, PDOKAN_FILE_INFO dokanFileInfo){
    UNREFERENCED_PARAMETER(creationTime);    //Android doesn't fully support creation times, so it's not possible to set the creation time

    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = windowsPathToAndroidPath(fileName);

    bool ok;
    device->runAdbCommand(QString("(test -d %1 || test -f %1) && touch -cm --date=\"@%2\" %1 && touch -ca --date=\"@%3\" %1").arg(escapeSpecialCharactersForBash(filePath), QString::number(microsoftTimeToUnixTime(*lastWriteTime)), QString::number(microsoftTimeToUnixTime(*lastAccessTime))), &ok, false);
    return ok ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;
}

NTSTATUS DOKAN_CALLBACK deleteFile(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = windowsPathToAndroidPath(fileName);

    bool ok;
    device->runAdbCommand(QString("rm %1").arg(escapeSpecialCharactersForBash(filePath)), &ok, false);
    return ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS DOKAN_CALLBACK deleteDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = windowsPathToAndroidPath(fileName);

    bool ok;
    device->runAdbCommand(QString("rm -rf %1").arg(escapeSpecialCharactersForBash(filePath)), &ok, false);
    return ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS DOKAN_CALLBACK moveFile(LPCWSTR oldFileName, LPCWSTR newFileName, BOOL replaceIfExisting, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);
    const QString oldFilePath = "/sdcard" + QString::fromWCharArray(oldFileName).replace("\\", "/");
    const QString newFilePath = "/sdcard" + QString::fromWCharArray(newFileName).replace("\\", "/");

    bool ok;
    device->runAdbCommand(QString("test -d %3 || mv %1 %2 %3").arg(replaceIfExisting ? "-f" : "-n", escapeSpecialCharactersForBash(oldFilePath), escapeSpecialCharactersForBash(newFilePath)), &ok, true);
    return ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS DOKAN_CALLBACK setAllocationSize(LPCWSTR, LONGLONG allocSize, PDOKAN_FILE_INFO dokanFileInfo){
    QByteArray *byteArray = reinterpret_cast<QByteArray*>(dokanFileInfo->Context);
    if(byteArray == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    if(byteArray->size() != allocSize + 1){
        const char modified = byteArray->back();
        byteArray->resize(allocSize + 1);
        byteArray->insert(allocSize, modified);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK getVolumeInformation(LPWSTR volumeNameBuffer, DWORD volumeNameSize, LPDWORD volumeSerialNumber, LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, LPWSTR fileSystemNameBuffer, DWORD fileSystemNameSize, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);

    volumeNameBuffer[device->model().left(volumeNameSize - 1).toWCharArray(volumeNameBuffer)] = L'\0';

    if(volumeSerialNumber != nullptr){
        *volumeSerialNumber = device->serialNumber().toULongLong(nullptr, 36);
    }
    if(maximumComponentLength != nullptr){
        *maximumComponentLength = 255;
    }
    if(fileSystemFlags != nullptr){
        *fileSystemFlags = FILE_SUPPORTS_REMOTE_STORAGE | FILE_UNICODE_ON_DISK | FILE_PERSISTENT_ACLS | FILE_NAMED_STREAMS;
    }

    fileSystemNameBuffer[device->fileSystem().left(fileSystemNameSize - 1).toWCharArray(fileSystemNameBuffer)] = L'\0';

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK getDiskFreeSpace(PULONGLONG freeBytesAvailable, PULONGLONG totalNumberOfBytes, PULONGLONG totalNumberOfFreeBytes, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDevice *device = AndroidDevice::fromDokanFileInfo(dokanFileInfo);

    const QStringList dfOutput = device->runAdbCommand("df /sdcard").split(QRegularExpression("[\r\n]+"));
    if(dfOutput.size() <= 1){
        return STATUS_UNSUCCESSFUL;
    }

    const QStringList values = dfOutput[1].split(QRegularExpression("\\s+"));
    if(values.size() <= 3){
        return STATUS_UNSUCCESSFUL;
    }

    const qulonglong usedBytes = values[2].toULongLong() * 1024;    //Multiply by 1024 because df gives the sizes in kilobytes, but Dokan expects the size in bytes
    *freeBytesAvailable = *totalNumberOfFreeBytes = values[3].toULongLong() * 1024;
    *totalNumberOfBytes = usedBytes + *freeBytesAvailable;
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK unmounted(PDOKAN_FILE_INFO dokanFileInfo){
    emit AndroidDevice::fromDokanFileInfo(dokanFileInfo)->driveUnmounted();
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK mounted(LPCWSTR mountPoint, PDOKAN_FILE_INFO dokanFileInfo){
    emit AndroidDevice::fromDokanFileInfo(dokanFileInfo)->driveMounted(mountPoint[0]);
    return STATUS_SUCCESS;
}
