#ifndef DOKANOPERATIONS_H
#define DOKANOPERATIONS_H

#include <dokan/dokan.h>

NTSTATUS DOKAN_CALLBACK createFile(LPCWSTR fileName, PDOKAN_IO_SECURITY_CONTEXT securityContext, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG shareAccess, ULONG createDisposition, ULONG createOptions, PDOKAN_FILE_INFO dokanFileInfo);
void DOKAN_CALLBACK closeFile(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo);
void DOKAN_CALLBACK cleanup(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK readFile(LPCWSTR fileName, LPVOID buffer, DWORD bufferLength, LPDWORD readLength, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK writeFile(LPCWSTR fileName, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LONGLONG offset, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK flushFileBuffers(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK getFileInformation(LPCWSTR fileName, LPBY_HANDLE_FILE_INFORMATION handleFileInformation, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK findFiles(LPCWSTR fileName, PFillFindData fillFindData, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK setFileAttributes(LPCWSTR fileName, DWORD fileAttributes, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK setFileTime(LPCWSTR fileName, const FILETIME *creationTime, const FILETIME *lastAccessTime, const FILETIME *lastWriteTime, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK deleteFile(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK deleteDirectory(LPCWSTR fileName, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK moveFile(LPCWSTR oldFileName, LPCWSTR newFileName, BOOL replaceIfExisting, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK setAllocationSize(LPCWSTR fileName, LONGLONG allocSize, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK getVolumeInformation(LPWSTR volumeNameBuffer, DWORD volumeNameSize, LPDWORD volumeSerialNumber, LPDWORD maximumComponentLength, LPDWORD fileSystemFlags, LPWSTR fileSystemNameBuffer, DWORD fileSystemNameSize, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK getDiskFreeSpace(PULONGLONG freeBytesAvailable, PULONGLONG totalNumberOfBytes, PULONGLONG totalNumberOfFreeBytes, PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK unmounted(PDOKAN_FILE_INFO dokanFileInfo);
NTSTATUS DOKAN_CALLBACK mounted(LPCWSTR mountPoint, PDOKAN_FILE_INFO dokanFileInfo);

#endif // DOKANOPERATIONS_H
