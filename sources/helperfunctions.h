#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <QString>

#include <dokan/dokan.h>

enum Attribute{
    ReadOnly         = 0x0001,
    Hidden           = 0x0002,
    System           = 0x0004,
    VolumeLabel      = 0x0008,
    Directory        = 0x0010,
    Archive          = 0x0020,
    NtfsEfs          = 0x0040,
    Normal           = 0x0080,
    Temporary        = 0x0100,
    Sparse           = 0x0200,
    ReparsePointData = 0x0400,
    Compressed       = 0x0800,
    Offline          = 0x1000
};

/**
 * Escapes special characters so that a file path can be passed to a bash command.
 *
 * @param filePath - The raw file path to be escaped.
 *
 * @return The escaped file path that can be passed to a bash command.
 */
QString escapeSpecialCharactersForBash(QString filePath);

/**
 * Gets the alt stream from a Windows path, i.e. the characters after the first ':' character.
 *
 * @param windowsPath - The path to get the alt stream from.
 *
 * @return The alt stream.
 */
QString getAltStream(LPCWSTR windowsPath);

/**
 * Use the same trick as WSL to escape characters that are allowed on Android/Linux but not on Windows (this trick consists in replacing a special character with a Unicode version by adding 0xf000 to its char code).
 *
 * @param fileName - The unescaped name of the file.
 *
 * @return The name of the file with all characters that are disallowed by Windows replaced with a Unicode version.
 */
QString androidFileNameToWindowsFileName(QString fileName);

/**
 * Converts a Microsoft time to a Unix time.
 *
 * @param microsoftTime - The Microsoft timestamp to convert.
 *
 * @return The corresponding Unix timestamp.
 */
qlonglong microsoftTimeToUnixTime(FILETIME microsoftTime);

/**
 * Converts a Unix time to a Microsoft time.
 *
 * @param unixTime - The Unix timestamp to convert.
 *
 * @return The corresponding Microsoft timestamp.
 */
FILETIME unixTimeToMicrosftTime(qlonglong unixTime);

/**
 * Gets the Windows file attributes of an Android file based on the file name and whether it's a directory.
 *
 * @param isDirectory - True if it's a directory, false otherwise.
 * @param fileName - The name of the file.
 *
 * @return A bitmask with the file attributes that Windows should use for this file.
 */
DWORD getFileAttributes(bool isDirectory, const QString &fileName);

#endif // HELPERFUNCTIONS_H
