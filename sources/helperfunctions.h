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

QString escapeSpecialCharactersForBash(QString filePath);
QString getAltStream(LPCWSTR windowsPath);
QString androidFileNameToWindowsFileName(QString fileName);
qlonglong microsoftTimeToUnixTime(FILETIME microsoftTime);
FILETIME unixTimeToMicrosftTime(qlonglong unixTime);

#endif // HELPERFUNCTIONS_H
