#include <QDir>
#include <QFile>
#include "androiddevice.h"
#include "helperfunctions.h"

QString escapeSpecialCharactersForBash(QString filePath){
    //List of characters that need to be escaped from https://stackoverflow.com/a/27817504/4284627
    const char specialCharacters[] = {'\\', ' ', '!', '"', '#', '$', '&', '\'', '(', ')', '*', ',', ';', '<', '>', '?', '[', ']', '^', '`', '{', '|', '}', '~'};    //It's important for the backslash to be first otherwise it will be escaped when already part of an escape sequence
    for(const char character: specialCharacters){
        filePath.replace(character, QString("\\") + character);
    }
    return filePath;
}

QString windowsPathToAndroidPath(LPCWSTR windowsPath){
    QString androidPath = "/sdcard" + QString::fromWCharArray(windowsPath).replace("\\", "/");
    //Use the same trick as WSL for characters that are allowed on Android but not on Windows (this trick consists in replacing a special character with a Unicode version by adding 0xf000 to its char code)
    const char specialCharacters[] = {'\\', ':', '*', '?', '"', '<', '>', '|'};
    for(const char character: specialCharacters){
        androidPath.replace(QChar(character + 0xf000), QChar(character));
    }
    return androidPath;
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
