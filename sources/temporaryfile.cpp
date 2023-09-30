#include "helperfunctions.h"
#include "temporaryfile.h"

//Since AndroidDrive reads and writes to files by copying them to local temporary files, a lot of this code is based on Dokan's Mirror example.

TemporaryFile::TemporaryFile(const AndroidDevice *device, const QString &remotePath, DWORD creationDisposition, ULONG shareAccess, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG createOptions, ULONG createDisposition):
    _localPath(QDir::toNativeSeparators(this->_temporaryDir.path() + "/AndroidDrive.tmp")),
    _remotePath(remotePath),
    _device(device),
    _handle(INVALID_HANDLE_VALUE),
    _modified(false),
    _errorCode(STATUS_SUCCESS)
{
    if(createDisposition == CREATE_ALWAYS){
        bool ok;
        device->runAdbCommand(QString("touch %1").arg(escapeSpecialCharactersForBash(remotePath)), &ok, false);
        if(!ok){
            this->_errorCode = STATUS_UNSUCCESSFUL;
            return;
        }
    }
    else if(!device->pullFromAdb(remotePath, this->_localPath)){
        this->_errorCode = STATUS_UNSUCCESSFUL;
        return;
    }

    ACCESS_MASK genericDesiredAccess;
    DWORD fileAttributesAndFlags;
    DokanMapKernelToUserCreateFileFlags(desiredAccess, fileAttributes, createOptions, createDisposition, &genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);
    if(creationDisposition == TRUNCATE_EXISTING){
        genericDesiredAccess |= GENERIC_WRITE;
    }

    this->_handle = CreateFile(this->_localPath.toStdWString().c_str(), genericDesiredAccess, shareAccess, nullptr, creationDisposition, fileAttributesAndFlags, nullptr);

    if(this->_handle == INVALID_HANDLE_VALUE){
        this->_errorCode = DokanNtStatusFromWin32(GetLastError());
        return;
    }
}

TemporaryFile::~TemporaryFile(){
    if(this->_handle != INVALID_HANDLE_VALUE){
        CloseHandle(this->_handle);
    }
}

NTSTATUS TemporaryFile::errorCode() const{
    return this->_errorCode;
}

NTSTATUS TemporaryFile::read(LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset) const{
    NTSTATUS status = STATUS_SUCCESS;
    bool closeHandleWhenFinished = false;

    HANDLE handle = this->_handle;
    if(handle == INVALID_HANDLE_VALUE){
        handle = CreateFile(this->_localPath.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if(handle == INVALID_HANDLE_VALUE){
            return DokanNtStatusFromWin32(GetLastError());
        }
        closeHandleWhenFinished = true;
    }

    LARGE_INTEGER distanceToMove;
    distanceToMove.QuadPart = offset;
    if(!SetFilePointerEx(handle, distanceToMove, nullptr, FILE_BEGIN) || !ReadFile(handle, buffer, bufferLength, readLength, nullptr)){
        status = DokanNtStatusFromWin32(GetLastError());
    }

    if(closeHandleWhenFinished){
        CloseHandle(handle);
    }

    return status;
}

NTSTATUS TemporaryFile::write(LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo){
    NTSTATUS status = STATUS_SUCCESS;
    bool closeHandleWhenFinished = false;

    HANDLE handle = this->_handle;
    if(handle == INVALID_HANDLE_VALUE){
        handle = CreateFile(this->_localPath.toStdWString().c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if(handle == INVALID_HANDLE_VALUE){
            return DokanNtStatusFromWin32(GetLastError());
        }
        closeHandleWhenFinished = true;
    }

    UINT64 fileSize = 0;
    DWORD fileSizeLow = 0;
    DWORD fileSizeHigh = 0;
    fileSizeLow = GetFileSize(handle, &fileSizeHigh);
    if(fileSizeLow == INVALID_FILE_SIZE){
        status = DokanNtStatusFromWin32(GetLastError());
    }
    else{
        fileSize = (static_cast<UINT64>(fileSizeHigh) << 32) | fileSizeLow;

        LARGE_INTEGER distanceToMove;
        if(dokanFileInfo->WriteToEndOfFile){
            LARGE_INTEGER z;
            z.QuadPart = 0;
            if(!SetFilePointerEx(handle, z, nullptr, FILE_END)){
                status = DokanNtStatusFromWin32(GetLastError());
            }
        }
        else{
            if(dokanFileInfo->PagingIo){
                if(static_cast<UINT64>(offset) >= fileSize){
                    *numberOfBytesWritten = 0;
                    if(closeHandleWhenFinished){
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

            distanceToMove.QuadPart = offset;
            if(!SetFilePointerEx(handle, distanceToMove, NULL, FILE_BEGIN)){
                status = DokanNtStatusFromWin32(GetLastError());
            }
        }

        if(status == STATUS_SUCCESS){
            if(WriteFile(handle, buffer, numberOfBytesToWrite, numberOfBytesWritten, nullptr)){
                this->_modified = true;
            }
            else{
                status = DokanNtStatusFromWin32(GetLastError());
            }
        }
    }

    if(closeHandleWhenFinished){
        CloseHandle(handle);
    }

    return status;
}

NTSTATUS TemporaryFile::setAllocationSize(LONGLONG allocSize){
    if(this->_handle == INVALID_HANDLE_VALUE){
        return STATUS_INVALID_HANDLE;
    }

    LARGE_INTEGER fileSize;
    if(!GetFileSizeEx(this->_handle, &fileSize)){
        return DokanNtStatusFromWin32(GetLastError());
    }

    if(allocSize < fileSize.QuadPart){
        fileSize.QuadPart = allocSize;
        if(!SetFilePointerEx(this->_handle, fileSize, nullptr, FILE_BEGIN) || !SetEndOfFile(this->_handle)){
            return DokanNtStatusFromWin32(GetLastError());
        }
    }

    this->_modified = true;
    return STATUS_SUCCESS;
}

NTSTATUS TemporaryFile::push(){
    /*if(this->_handle != INVALID_HANDLE_VALUE){
        CloseHandle(this->_handle);
        this->_handle = INVALID_HANDLE_VALUE;
    }*/
    if(this->_modified){
        if(!this->_device->pushToAdb(this->_localPath, this->_remotePath)){
            return STATUS_UNSUCCESSFUL;
        }
        this->_modified = false;
    }
    return STATUS_SUCCESS;
}
