#ifndef TEMPORARYFILE_H
#define TEMPORARYFILE_H

#include <QString>
#include <QTemporaryDir>
#include "androiddevice.h"

class TemporaryFile final{
public:
    TemporaryFile(const AndroidDevice *device, const QString &remotePath, DWORD creationDisposition, ULONG shareAccess, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG createOptions, ULONG createDisposition);
    ~TemporaryFile();

    NTSTATUS errorCode() const;

    NTSTATUS read(LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset) const;
    NTSTATUS write(LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo);
    NTSTATUS setAllocationSize(LONGLONG allocSize);
    NTSTATUS push();

private:
    const QTemporaryDir _temporaryDir;
    const QString _localPath, _remotePath;
    const AndroidDevice *const _device;
    HANDLE _handle;
    bool _modified;
    NTSTATUS _errorCode;
};

#endif // TEMPORARYFILE_H
