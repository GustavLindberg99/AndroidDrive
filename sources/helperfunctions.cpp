#include "helperfunctions.hpp"

#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include "settings.hpp"

QString escapeSpecialCharactersForBash(QString filePath){
    //List of characters that need to be escaped from https://stackoverflow.com/a/27817504/4284627
    const char specialCharacters[] = {'\\', ' ', '!', '"', '#', '$', '&', '\'', '(', ')', '*', ',', ';', '<', '>', '?', '[', ']', '^', '`', '{', '|', '}', '~'};    //It's important for the backslash to be first otherwise it will be escaped when already part of an escape sequence
    for(const char character: specialCharacters){
        filePath.replace(character, QString("\\") + character);
    }
    return filePath;
}

QString getAltStream(LPCWSTR windowsPath){
    static const QRegularExpression altStreamRegex("^[^:]+(:|$)");
    return QString::fromWCharArray(windowsPath).replace(altStreamRegex, "");
}

QString androidFileNameToWindowsFileName(QString fileName){
    //Use the same trick as WSL for characters that are allowed on Android but not on Windows (this trick consists in replacing a special character with a Unicode version by adding 0xf000 to its char code)
    const char specialCharacters[] = {'\\', ':', '*', '?', '"', '<', '>', '|'};
    for(const char character: specialCharacters){
        fileName.replace(QChar(character), QChar(character + 0xf000));
    }
    return fileName;
}

qlonglong microsoftTimeToUnixTime(FILETIME microsoftTime){
    LARGE_INTEGER mstLargeInt;
    mstLargeInt.LowPart = microsoftTime.dwLowDateTime;
    mstLargeInt.HighPart = microsoftTime.dwHighDateTime;
    const qlonglong secondsSinceMsEpoch = mstLargeInt.QuadPart / 10000000;
    constexpr qlonglong msEpochUnixTimestamp = -11644473600;
    return secondsSinceMsEpoch + msEpochUnixTimestamp;
}

FILETIME unixTimeToMicrosftTime(qlonglong unixTime){
    constexpr qlonglong msEpochUnixTimestamp = -11644473600;
    const qlonglong secondsSinceMsEpoch = unixTime - msEpochUnixTimestamp;
    LARGE_INTEGER mstLargeInt;
    mstLargeInt.QuadPart = secondsSinceMsEpoch * 10000000;
    FILETIME microsftTime;
    microsftTime.dwLowDateTime = mstLargeInt.LowPart;
    microsftTime.dwHighDateTime = mstLargeInt.HighPart;
    return microsftTime;
}

DWORD getFileAttributes(bool isDirectory, const QString &fileName){
    DWORD fileAttributes = 0;
    if(isDirectory){
        fileAttributes |= Attribute::Directory;
    }
    if((fileName.startsWith(".") && Settings().hideDotFiles()) || fileName == "desktop.ini"){
        fileAttributes |= Attribute::Hidden;
    }
    //This is needed for custom folder icons to be displayed correctly (source: https://superuser.com/q/882442/513819)
    if(isDirectory || fileName == "desktop.ini"){
        fileAttributes |= Attribute::System;
    }
    return fileAttributes;
}
