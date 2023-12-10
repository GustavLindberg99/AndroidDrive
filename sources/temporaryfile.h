#ifndef TEMPORARYFILE_H
#define TEMPORARYFILE_H

#include <QString>
#include "androiddevice.h"

class TemporaryFile final{
public:
    TemporaryFile(const AndroidDevice *device, const QString &remotePath, DWORD creationDisposition, ULONG shareAccess, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG createOptions, ULONG createDisposition, bool exists, const QString &altStream);
    ~TemporaryFile();

    TemporaryFile(const TemporaryFile&) = delete;
    void operator=(const TemporaryFile&) = delete;

    NTSTATUS errorCode() const;

    NTSTATUS read(LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset, const QString &altStream) const;
    NTSTATUS write(LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo, const QString &altStream);
    NTSTATUS setAllocationSize(LONGLONG allocSize);
    NTSTATUS getFileInformation(LPBY_HANDLE_FILE_INFORMATION handleFileInformation);
    NTSTATUS push();

private:
    std::wstring localPathWithAltStream(const QString &altStream) const;

    const QString _localPath, _remotePath;
    const AndroidDevice *const _device;
    HANDLE _handle;
    bool _modified;
    NTSTATUS _errorCode;
};

#endif // TEMPORARYFILE_H
