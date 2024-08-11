#include "dokanoperations.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include "androiddrive.h"
#include "helperfunctions.h"
#include "settingswindow.h"
#include "temporaryfile.h"

NTSTATUS DOKAN_CALLBACK createFile(LPCWSTR fileName, PDOKAN_IO_SECURITY_CONTEXT, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG shareAccess, ULONG createDisposition, ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = drive->windowsPathToAndroidPath(fileName);

    ACCESS_MASK genericDesiredAccess;
    DWORD fileAttributesAndFlags, creationDisposition;
    DokanMapKernelToUserCreateFileFlags(desiredAccess, fileAttributes, createOptions, createDisposition, &genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);
    const QString output = drive->device()->runAdbCommand(QString("(test -d %1 && echo d) || (test -f %1 && echo f)").arg(escapeSpecialCharactersForBash(filePath)));
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
            drive->device()->runAdbCommand(QString("mkdir %1").arg(escapeSpecialCharactersForBash(filePath)), &ok, false);
            return ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
        }
        else if(!directoryExists){
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }
    else{
        TemporaryFile *temporaryFile = new TemporaryFile(drive, filePath, creationDisposition, shareAccess, desiredAccess, fileAttributes, createOptions, createDisposition, fileExists, getAltStream(fileName));

        const NTSTATUS errorCode = temporaryFile->errorCode();
        if(errorCode != STATUS_SUCCESS){
            delete temporaryFile;
            return errorCode;
        }
        dokanFileInfo->Context = reinterpret_cast<ULONG64>(temporaryFile);
    }
    return STATUS_SUCCESS;
}

void DOKAN_CALLBACK closeFile(LPCWSTR, PDOKAN_FILE_INFO dokanFileInfo){
    TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);

    if(temporaryFile != nullptr){
        delete temporaryFile;
        dokanFileInfo->Context = 0;
    }
}

void DOKAN_CALLBACK cleanup(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo){
    if(dokanFileInfo->DeleteOnClose){
        deleteFile(fileName, dokanFileInfo);
    }
    else{
        TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
        if(temporaryFile != nullptr){
            temporaryFile->push();
        }
    }
}

NTSTATUS DOKAN_CALLBACK readFile(LPCWSTR fileName, LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo){
    const TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
    if(temporaryFile == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    return temporaryFile->read(buffer, bufferLength, readLength, offset, getAltStream(fileName));
}

NTSTATUS DOKAN_CALLBACK writeFile(LPCWSTR fileName, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo){
    TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
    if(temporaryFile == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    return temporaryFile->write(buffer, numberOfBytesToWrite, numberOfBytesWritten, offset, dokanFileInfo, getAltStream(fileName));
}

NTSTATUS DOKAN_CALLBACK flushFileBuffers(LPCWSTR, PDOKAN_FILE_INFO dokanFileInfo){
    TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
    if(temporaryFile == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    return temporaryFile->push();
}

NTSTATUS DOKAN_CALLBACK getFileInformation(LPCWSTR fileName, LPBY_HANDLE_FILE_INFORMATION handleFileInformation, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = drive->windowsPathToAndroidPath(fileName);

    bool ok;
    const QString fileInfo = drive->device()->runAdbCommand(QString("stat -c \"%F %s %W %Y %X\" %1").arg(escapeSpecialCharactersForBash(filePath)), &ok);    //All devices don't support stat --format, so we have to use stat -c
    if(!ok){
        TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
        if(temporaryFile != nullptr){
            return temporaryFile->getFileInformation(handleFileInformation);
        }
        return STATUS_UNSUCCESSFUL;
    }

    static const QRegularExpression fileInfoRegex("^([A-Za-z\\s]+) ([0-9]+) ([0-9?]+) ([0-9?]+) ([0-9?]+)$");
    const QRegularExpressionMatch match = fileInfoRegex.match(fileInfo);
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

    handleFileInformation->dwFileAttributes = getFileAttributes(isDirectory, filePath.split("/").last());
    handleFileInformation->ftCreationTime = creationTimeKnown ? creationTime : unknownTime;
    handleFileInformation->ftLastWriteTime = lastWriteTimeKnown ? lastWriteTime : unknownTime;
    handleFileInformation->ftLastAccessTime = lastAccessTimeKnown ? lastAccessTime : unknownTime;
    handleFileInformation->nFileSizeHigh = fileSize.HighPart;
    handleFileInformation->nFileSizeLow = fileSize.LowPart;

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK findFiles(LPCWSTR fileName, PFillFindData fillFindData, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = drive->windowsPathToAndroidPath(fileName);

    /* Format sequences used for stat:
     * %F = File type ("directory" for directories, something else for files)
     * %s = File size in bytes
     * %W = Unix timestamp in seconds for creation time
     * %Y = Unix timestamp in seconds for last write time
     * %X = Unix timestamp in seconds for last access time
     * %n = File path
     */
    bool ok;
    static const QRegularExpression newlineRegex("[\r\n]+");
    const QStringList output = drive->device()->runAdbCommand(QString("stat -c \"%F %s %W %Y %X %n\" %1/* %1/.* || test -d %1").arg(escapeSpecialCharactersForBash(filePath)), &ok).split(newlineRegex);    //The || test -d is necessary so that it doesn't return an error for empty directories (otherwise it will give an error like "Can't find /sdcard/emptydirectory/*" since there are no files that match that pattern), but so that it still returns an error if the directory doesn't exist at all.
    if(!ok){
        return STATUS_UNSUCCESSFUL;
    }

    static const QRegularExpression fileInfoRegex("^([A-Za-z\\s]+) ([0-9]+) ([0-9?]+) ([0-9?]+) ([0-9?]+) (.+)$");
    for(const QString &fileInfo: output){
        const QRegularExpressionMatch match = fileInfoRegex.match(fileInfo);
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
        findData.dwFileAttributes = getFileAttributes(isDirectory, subfileName);
        findData.ftCreationTime = creationTimeKnown ? creationTime : unknownTime;
        findData.ftLastWriteTime = lastWriteTimeKnown ? lastWriteTime : unknownTime;
        findData.ftLastAccessTime = lastAccessTimeKnown ? lastAccessTime : unknownTime;
        findData.nFileSizeHigh = fileSize.HighPart;
        findData.nFileSizeLow = fileSize.LowPart;
        wcscpy_s(findData.cFileName, sizeof(findData.cFileName) / sizeof(findData.cFileName[0]), subfileName.toStdWString().c_str());
        findData.cAlternateFileName[0] = L'\0';    //There is no alternate file name, so just set the first byte to a null terminator to indicate an empty string
        fillFindData(&findData, dokanFileInfo);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK setFileAttributes(LPCWSTR, DWORD, PDOKAN_FILE_INFO){
    //Android doesn't have file attributes like Windows does, so just do nothing.
    //We have to return STATUS_SUCCESS because some Windows features expect this to be implemented so Windows Explorer won't work as expected if this doesn't return STATUS_SUCCESS.
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK setFileTime(LPCWSTR fileName, const FILETIME *creationTime, const FILETIME *lastAccessTime, const FILETIME *lastWriteTime, PDOKAN_FILE_INFO dokanFileInfo){
    UNREFERENCED_PARAMETER(creationTime);    //Android doesn't fully support creation times, so it's not possible to set the creation time

    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = drive->windowsPathToAndroidPath(fileName);

    drive->device()->runAdbCommand(QString("(test -d %1 || test -f %1) && touch -cm --date=\"@%2\" %1 && touch -ca --date=\"@%3\" %1").arg(escapeSpecialCharactersForBash(filePath), QString::number(microsoftTimeToUnixTime(*lastWriteTime)), QString::number(microsoftTimeToUnixTime(*lastAccessTime))), nullptr, false);

    TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
    if(temporaryFile != nullptr){
        temporaryFile->setFileTime(creationTime, lastAccessTime, lastWriteTime);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK deleteFile(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = drive->windowsPathToAndroidPath(fileName);

    bool ok;
    drive->device()->runAdbCommand(QString("rm %1").arg(escapeSpecialCharactersForBash(filePath)), &ok, false);
    if(!ok){
        return STATUS_UNSUCCESSFUL;
    }

    QFile localFile(drive->localPath(filePath));
    if(localFile.exists()){
        localFile.remove();
    }
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK deleteDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);
    const QString filePath = drive->windowsPathToAndroidPath(fileName);

    bool ok;
    drive->device()->runAdbCommand(QString("rm -rf %1").arg(escapeSpecialCharactersForBash(filePath)), &ok, false);
    if(!ok){
        return STATUS_UNSUCCESSFUL;
    }

    QDir localDir(drive->localPath(filePath));
    if(localDir.exists()){
        localDir.removeRecursively();
    }
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK moveFile(LPCWSTR oldFileName, LPCWSTR newFileName, BOOL replaceIfExisting, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);
    const QString oldFilePath = drive->windowsPathToAndroidPath(oldFileName);
    const QString newFilePath = drive->windowsPathToAndroidPath(newFileName);

    bool ok;
    drive->device()->runAdbCommand(QString("test -d %3 || mv %1 %2 %3").arg(replaceIfExisting ? "-f" : "-n", escapeSpecialCharactersForBash(oldFilePath), escapeSpecialCharactersForBash(newFilePath)), &ok, true);
    if(!ok){
        return STATUS_UNSUCCESSFUL;
    }

    QFileInfo oldLocalFile(drive->localPath(oldFilePath));
    if(oldLocalFile.isFile()){
        QFile(oldLocalFile.absoluteFilePath()).remove();
    }
    else if(oldLocalFile.isDir()){
        QDir(oldLocalFile.absoluteFilePath()).removeRecursively();
    }
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK setAllocationSize(LPCWSTR, LONGLONG allocSize, PDOKAN_FILE_INFO dokanFileInfo){
    TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
    if(temporaryFile == nullptr){
        return STATUS_INVALID_HANDLE;
    }

    return temporaryFile->setAllocationSize(allocSize);
}

NTSTATUS DOKAN_CALLBACK getVolumeInformation(LPWSTR volumeNameBuffer, DWORD volumeNameSize, LPDWORD volumeSerialNumber, LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, LPWSTR fileSystemNameBuffer, DWORD fileSystemNameSize, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);

    wcscpy_s(volumeNameBuffer, volumeNameSize, Settings().driveName(drive).toStdWString().c_str());

    if(volumeSerialNumber != nullptr){
        *volumeSerialNumber = drive->device()->serialNumber().toULongLong(nullptr, 36);
    }
    if(maximumComponentLength != nullptr){
        *maximumComponentLength = 255;
    }
    if(fileSystemFlags != nullptr){
        *fileSystemFlags = FILE_SUPPORTS_REMOTE_STORAGE | FILE_UNICODE_ON_DISK | FILE_PERSISTENT_ACLS | FILE_NAMED_STREAMS;
    }

    wcscpy_s(fileSystemNameBuffer, fileSystemNameSize, drive->fileSystem().toStdWString().c_str());

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK getDiskFreeSpace(PULONGLONG freeBytesAvailable, PULONGLONG totalNumberOfBytes, PULONGLONG totalNumberOfFreeBytes, PDOKAN_FILE_INFO dokanFileInfo){
    const AndroidDrive *drive = AndroidDrive::fromDokanFileInfo(dokanFileInfo);

    static const QRegularExpression newlineRegex("[\r\n]+");
    const QStringList dfOutput = drive->device()->runAdbCommand(QString("df %1").arg(drive->androidRootPath())).split(newlineRegex);
    if(dfOutput.size() <= 1){
        return STATUS_UNSUCCESSFUL;
    }

    static const QRegularExpression spaceRegex("\\s+");
    const QStringList values = dfOutput[1].split(spaceRegex);
    if(values.size() <= 3){
        return STATUS_UNSUCCESSFUL;
    }

    const qulonglong usedBytes = values[2].toULongLong() * 1024;    //Multiply by 1024 because df gives the sizes in kilobytes, but Dokan expects the size in bytes
    *freeBytesAvailable = *totalNumberOfFreeBytes = values[3].toULongLong() * 1024;
    *totalNumberOfBytes = usedBytes + *freeBytesAvailable;
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK unmounted(PDOKAN_FILE_INFO dokanFileInfo){
    emit AndroidDrive::fromDokanFileInfo(dokanFileInfo)->driveUnmounted();
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK mounted(LPCWSTR mountPoint, PDOKAN_FILE_INFO dokanFileInfo){
    emit AndroidDrive::fromDokanFileInfo(dokanFileInfo)->driveMounted(mountPoint[0]);
    return STATUS_SUCCESS;
}
