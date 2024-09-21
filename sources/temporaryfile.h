#ifndef TEMPORARYFILE_H
#define TEMPORARYFILE_H

#include <QString>

#include <memory>

#include "androiddevice.h"

class TemporaryFile final {
public:
    /**
     * Downloads a file from the Android device to a local temporary file.
     *
     * @param dokanFileInfo - The Dokan file info object that the temporary file should be added to as Context.
     * @param remotePath - The path of the Android file to download.
     * @param creationDisposition - The creation disposition to use when creating the handle.
     * @param shareAccess - The share access to use when creating the handle.
     * @param desiredAccess - The desired access to use when creating the handle.
     * @param fileAttributes - The file attributes to use when creating the handle.
     * @param createOptions - The create options to use when creating the handle.
     * @param createDisposition - The create disposition to use when creating the handle.
     * @param exists - True if opening an existing file (in which case it should be downloaded), false when creating a new file (in which case it should just be created locally).
     * @param altStream - The alt stream to use when creating the handle.
     *
     * @return STATUS_SUCCESS on success, an error status on failure.
     */
    TemporaryFile(PDOKAN_FILE_INFO dokanFileInfo, const AndroidDrive *drive, const QString &remotePath, DWORD creationDisposition, ULONG shareAccess, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG createOptions, ULONG createDisposition, bool exists, const QString &altStream);

    /**
     * Destructor. Closes the handle and resets dokanFileInfo->Context.
     */
    ~TemporaryFile();

    /**
     * Disallow copying.
     */
    TemporaryFile(const TemporaryFile&) = delete;
    void operator=(const TemporaryFile&) = delete;

    /**
     * Gets the error code of the last error that occurred in the constructor.
     *
     * @return The error code of the last error, or STATUS_SUCCESS if no error has occurred in the constructor.
     */
    NTSTATUS errorCode() const;

    /**
     * Reads from the local copy of the file.
     *
     * @param buffer - Read buffer that will be filled with the read result.
     * @param bufferLength - Buffer length and read size to continue with.
     * @param readLength - Total data size that has been read.
     * @param offset - Offset from where the read has to be continued.
     * @param altStream - The alt stream of the file that was requested.
     *
     * @return STATUS_SUCCESS on success, an error code on failure.
     */
    NTSTATUS read(LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset, const QString &altStream) const;

    /**
     * Writes to the local copy of the file.
     *
     * @param buffer - Data to write.
     * @param numberOfBytesToWrite - Buffer length and write size to continue with.
     * @param numberOfBytesWritten - Total number of bytes that have been written.
     * @param offset - Offset from where the write has to be continued.
     * @param dokanFileInfo - The Dokan file info of the file.
     * @param altStream - The alt stream of the file that was requested.
     *
     * @return STATUS_SUCCESS on success, an error code on failure.
     */
    NTSTATUS write(LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo, const QString &altStream);

    /**
     * Truncates or extends the local copy of the file.
     *
     * @param allocSize - File length to set.
     *
     * @return STATUS_SUCCESS on success, an error code on failure.
     */
    NTSTATUS setAllocationSize(LONGLONG allocSize);

    /**
     * Gets information about the local copy of the file.
     *
     * @param handleFileInformation The file information struct that will be filled.
     *
     * @return STATUS_SUCCESS on success, an error code on failure.
     */
    NTSTATUS getFileInformation(LPBY_HANDLE_FILE_INFORMATION handleFileInformation);

    /**
     * Sets the file time on the local file.
     *
     * @param creationTime - The creation time to set.
     * @param lastAccessTime - The last access time to set.
     * @param lastWriteTime - The last write time to set.
     *
     * @return STATUS_SUCCESS on success, an error code on failure.
     */
    NTSTATUS setFileTime(const FILETIME *creationTime, const FILETIME *lastAccessTime, const FILETIME *lastWriteTime);

    /**
     * Pushes the local copy to the Android device.
     *
     * @return STATUS_SUCCESS on success, an error code on failure.
     */
    NTSTATUS push();

private:
    /**
     * Gets the path of the local copy of this file with the given alt stream.
     *
     * @param altStream - The alt stream to append to the path. If empty, just returns the local path as is.
     *
     * @return The local path and the alt stream separated by a ':' character.
     */
    std::wstring localPathWithAltStream(const QString &altStream) const;

    const QString _localPath, _remotePath;
    const std::shared_ptr<const AndroidDevice> _device;
    const PDOKAN_FILE_INFO _dokanFileInfo;
    HANDLE _handle;
    NTSTATUS _errorCode;
    bool _modified;
};

#endif // TEMPORARYFILE_H
