#include "temporaryfile.hpp"

#include "androiddrive.hpp"
#include "debuglogger.hpp"
#include "helperfunctions.hpp"

//Since AndroidDrive reads and writes to files by copying them to local temporary files, a lot of this code is based on Dokan's Mirror example.

TemporaryFile::TemporaryFile(const AndroidDrive *drive, const QString &remotePath, DWORD creationDisposition, ULONG shareAccess, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG createOptions, ULONG createDisposition, bool exists, const QString &altStream):
    _localPath(drive->localPath(remotePath)),
    _remotePath(remotePath),
    _device(drive->device()),
    _handle(INVALID_HANDLE_VALUE),
    _errorCode(STATUS_SUCCESS),
    _modified(!exists)
{
    DebugLogger::getInstance().log("Creating temporary file. localPath: '{}', remotePath: '{}', device: '{}', exists: {}, creationDisposition: {}, shareAccess: {}, desiredAccess: {}, fileAttributes: {}, createOptions: {}, createDisposition: {}, altStream: {}", std::make_tuple(this->_localPath, this->_remotePath, this->_device->serialNumber(), exists, creationDisposition, shareAccess, desiredAccess, fileAttributes, createOptions, createDisposition, altStream));

    if(exists && !this->_device->pullFromAdb(remotePath, this->_localPath) && !QFileInfo(this->_localPath).isFile()){
        DebugLogger::getInstance().log("Failed to pul temporary file '{}' from ADB", this->_remotePath);
        this->_errorCode = STATUS_UNSUCCESSFUL;
        return;
    }

    ACCESS_MASK genericDesiredAccess;
    DWORD fileAttributesAndFlags;
    DokanMapKernelToUserCreateFileFlags(desiredAccess, fileAttributes, createOptions, createDisposition, &genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);
    if(creationDisposition == TRUNCATE_EXISTING){
        genericDesiredAccess |= GENERIC_WRITE;
    }
    DebugLogger::getInstance().log("genericDesiredAccess: {}, fileAttributesAndFlags: {}, creationDisposition: {}", std::make_tuple(genericDesiredAccess, fileAttributesAndFlags, creationDisposition));

    this->_handle = CreateFile(this->localPathWithAltStream(altStream).c_str(), genericDesiredAccess, shareAccess, nullptr, creationDisposition, fileAttributesAndFlags, nullptr);

    if(this->_handle == INVALID_HANDLE_VALUE){
        this->_errorCode = DokanNtStatusFromWin32(GetLastError());
        DebugLogger::getInstance().log("Invalid handle, error code: {}", this->_errorCode);
    }
    else{
        DebugLogger::getInstance().log("Valid handle: {}", this->_handle);
    }
}

TemporaryFile::~TemporaryFile(){
    DebugLogger::getInstance().log("Destroying temporary file with remote path '{}'", this->_remotePath);
    if(this->_handle != INVALID_HANDLE_VALUE){
        CloseHandle(this->_handle);
    }
}

NTSTATUS TemporaryFile::errorCode() const{
    return this->_errorCode;
}

NTSTATUS TemporaryFile::read(LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset, const QString &altStream) const{
    DebugLogger::getInstance().log("Calling read on temporary file with local path '{}': bufferLength: {}, offset: {}, altStream: {}", std::make_tuple(this->_localPath, bufferLength, offset, altStream));

    NTSTATUS status = STATUS_SUCCESS;
    bool closeHandleWhenFinished = false;

    HANDLE handle = this->_handle;
    if(handle == INVALID_HANDLE_VALUE){
        handle = CreateFile(this->localPathWithAltStream(altStream).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if(handle == INVALID_HANDLE_VALUE){
            DebugLogger::getInstance().log("Failed to create handle during read");
            return DokanNtStatusFromWin32(GetLastError());
        }
        DebugLogger::getInstance().log("Handle not initialized, creating it during read");
        closeHandleWhenFinished = true;
    }

    LARGE_INTEGER distanceToMove;
    distanceToMove.QuadPart = offset;
    if(!SetFilePointerEx(handle, distanceToMove, nullptr, FILE_BEGIN) || !ReadFile(handle, buffer, bufferLength, readLength, nullptr)){
        status = DokanNtStatusFromWin32(GetLastError());
        DebugLogger::getInstance().log("Reading file '{}' failed. Error code: {}", std::make_tuple(this->_localPath, status));
    }
    else{
        DebugLogger::getInstance().log("Reading file '{}' succeeded", this->_localPath);
    }

    if(closeHandleWhenFinished){
        DebugLogger::getInstance().log("Closing handle after read");
        CloseHandle(handle);
    }

    return status;
}

NTSTATUS TemporaryFile::write(LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo, const QString &altStream){
    DebugLogger::getInstance().log("Calling write on temporary file with local path '{}': numberOfBytesToWrite: {}, offset: {}, altStream: {}", std::make_tuple(this->_localPath, numberOfBytesToWrite, offset, altStream));

    NTSTATUS status = STATUS_SUCCESS;
    bool closeHandleWhenFinished = false;

    HANDLE handle = this->_handle;
    if(handle == INVALID_HANDLE_VALUE){
        handle = CreateFile(this->localPathWithAltStream(altStream).c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if(handle == INVALID_HANDLE_VALUE){
            DebugLogger::getInstance().log("Failed to create handle during write");
            return DokanNtStatusFromWin32(GetLastError());
        }
        DebugLogger::getInstance().log("Handle not initialized, creating it during write");
        closeHandleWhenFinished = true;
    }

    UINT64 fileSize = 0;
    DWORD fileSizeLow = 0;
    DWORD fileSizeHigh = 0;
    fileSizeLow = GetFileSize(handle, &fileSizeHigh);
    if(fileSizeLow == INVALID_FILE_SIZE){
        status = DokanNtStatusFromWin32(GetLastError());
        DebugLogger::getInstance().log("Failed to get file size. Error code: {}", status);
    }
    else{
        fileSize = (static_cast<UINT64>(fileSizeHigh) << 32) | fileSizeLow;

        LARGE_INTEGER distanceToMove;
        if(dokanFileInfo->WriteToEndOfFile){
            LARGE_INTEGER z;
            z.QuadPart = 0;
            if(!SetFilePointerEx(handle, z, nullptr, FILE_END)){
                status = DokanNtStatusFromWin32(GetLastError());
                DebugLogger::getInstance().log("Failed to write to end of file. Error code: {}", status);
            }
        }
        else{
            if(dokanFileInfo->PagingIo){
                if(static_cast<UINT64>(offset) >= fileSize){
                    DebugLogger::getInstance().log("Offset {} greater than file size {}, no need to write anything", std::make_tuple(offset, fileSize));
                    *numberOfBytesWritten = 0;
                    if(closeHandleWhenFinished){
                        DebugLogger::getInstance().log("Closing handle after write");
                        CloseHandle(handle);
                    }
                    return STATUS_SUCCESS;
                }
                else if((static_cast<UINT64>(offset) + numberOfBytesToWrite) > fileSize){
                    UINT64 bytes = fileSize - offset;
                    if(bytes >> 32){
                        numberOfBytesToWrite = static_cast<DWORD>(bytes & 0xFFFFFFFFUL);
                    }
                    else{
                        numberOfBytesToWrite = static_cast<DWORD>(bytes);
                    }
                }
            }

            DebugLogger::getInstance().log("Writing {} bytes to file '{}'", std::make_tuple(numberOfBytesToWrite, this->_localPath));
            distanceToMove.QuadPart = offset;
            if(!SetFilePointerEx(handle, distanceToMove, nullptr, FILE_BEGIN)){
                status = DokanNtStatusFromWin32(GetLastError());
                DebugLogger::getInstance().log("Error during SetFilePointerEx. Error code: {}");
            }
        }

        if(status == STATUS_SUCCESS){
            if(WriteFile(handle, buffer, numberOfBytesToWrite, numberOfBytesWritten, nullptr)){
                this->_modified = true;
                DebugLogger::getInstance().log("Write to file '{}' succeeded", this->_localPath);
            }
            else{
                status = DokanNtStatusFromWin32(GetLastError());
                DebugLogger::getInstance().log("Write to file '{}' failed. Error code: {}", std::make_tuple(this->_localPath, status));
            }
        }
    }

    if(closeHandleWhenFinished){
        DebugLogger::getInstance().log("Closing handle after write");
        CloseHandle(handle);
    }

    return status;
}

NTSTATUS TemporaryFile::setAllocationSize(LONGLONG allocSize){
    DebugLogger::getInstance().log("Calling setAllocationSize on temporary file with local path '{}': allocSize: {}", std::make_tuple(this->_localPath, allocSize));

    if(this->_handle == INVALID_HANDLE_VALUE){
        DebugLogger::getInstance().log("Cannot set allocation size of file '{}' because handle is not valid", this->_localPath);
        return STATUS_INVALID_HANDLE;
    }

    LARGE_INTEGER fileSize;
    if(!GetFileSizeEx(this->_handle, &fileSize)){
        DebugLogger::getInstance().log("Failed to get file size of file '{}'", this->_localPath);
        return DokanNtStatusFromWin32(GetLastError());
    }

    DebugLogger::getInstance().log("File size of '{}': {}", std::make_tuple(this->_localPath, fileSize.QuadPart));
    if(allocSize < fileSize.QuadPart){
        fileSize.QuadPart = allocSize;
        if(!SetFilePointerEx(this->_handle, fileSize, nullptr, FILE_BEGIN) || !SetEndOfFile(this->_handle)){
            DebugLogger::getInstance().log("Failed to set allocation size of file '{}'", this->_localPath);
            return DokanNtStatusFromWin32(GetLastError());
        }
        else{
            DebugLogger::getInstance().log("Successfully set allocation size of file '{}'", this->_localPath);
        }
    }
    else{
        DebugLogger::getInstance().log("No need to set allocation size  of '{}': allocation size {} is less than file size {}", std::make_tuple(this->_localPath, allocSize, fileSize.QuadPart));
    }

    this->_modified = true;
    return STATUS_SUCCESS;
}

NTSTATUS TemporaryFile::push(){
    DebugLogger::getInstance().log("Calling push on temporary file with local path '{}' and remote path '{}'", std::make_tuple(this->_localPath, this->_remotePath));

    if(this->_modified){
        BY_HANDLE_FILE_INFORMATION fileInformation;
        this->getFileInformation(&fileInformation);
        if(!this->_device->pushToAdb(this->_localPath, this->_remotePath)){
            //If it failed while the handle is opened, close the handle because sometimes it fails because the open handle causes it to not have read permission
            CloseHandle(this->_handle);
            this->_handle = INVALID_HANDLE_VALUE;
            DebugLogger::getInstance().log("Failed to push with open handle, trying again with closed handle");
            if(!this->_device->pushToAdb(this->_localPath, this->_remotePath)){
                DebugLogger::getInstance().log("Failed to push local file '{}' to remote '{}'", std::make_tuple(this->_localPath, this->_remotePath));
                return STATUS_UNSUCCESSFUL;
            }
            else{
                DebugLogger::getInstance().log("Pushing local file '{}' to remote file '{}' succeeded after closing handle", std::make_tuple(this->_localPath, this->_remotePath));
            }
        }
        else{
            DebugLogger::getInstance().log("Pushing local file '{}' to remote file '{}' succeeded on first try", std::make_tuple(this->_localPath, this->_remotePath));
        }
        this->_device->runAdbCommand(QString("(test -d %1 || test -f %1) && touch -cm --date=\"@%2\" %1 && touch -ca --date=\"@%3\" %1").arg(escapeSpecialCharactersForBash(this->_remotePath), QString::number(microsoftTimeToUnixTime(fileInformation.ftLastWriteTime)), QString::number(microsoftTimeToUnixTime(fileInformation.ftLastAccessTime))), nullptr, false);
        this->_modified = false;
    }
    else{
        DebugLogger::getInstance().log("File '{}' not modified, no need to push", this->_localPath);
    }
    return STATUS_SUCCESS;
}

NTSTATUS TemporaryFile::getFileInformation(LPBY_HANDLE_FILE_INFORMATION handleFileInformation){
    const bool success = GetFileInformationByHandle(this->_handle, handleFileInformation);
    return success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS TemporaryFile::setFileTime(const FILETIME *creationTime, const FILETIME *lastAccessTime, const FILETIME *lastWriteTime){
    const bool success = SetFileTime(this->_handle, creationTime, lastAccessTime, lastWriteTime);
    return success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

std::wstring TemporaryFile::localPathWithAltStream(const QString &altStream) const{
    if(altStream.isEmpty()){
        return this->_localPath.toStdWString();
    }
    return (this->_localPath + ":" + altStream).toStdWString();
}
