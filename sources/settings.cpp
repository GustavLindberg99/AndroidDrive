#include "settings.h"

#include "androiddrive.h"

char Settings::driveLetter(const AndroidDrive *drive) const{
    return this->_settings.value(drive->id() + "_driveLetter", 'D').toChar().toLatin1();
}

void Settings::setDriveLetter(const AndroidDrive *drive, char driveLetter){
    this->_settings.setValue(drive->id() + "_driveLetter", driveLetter);
}

QString Settings::driveName(const AndroidDrive *drive) const{
    return this->_settings.value(drive->id() + "_driveName", drive->completeName()).toString();
}

void Settings::setDriveName(const AndroidDrive *drive, const QString &driveName){
    this->_settings.setValue(drive->id() + "_driveName", driveName);
}

bool Settings::autoConnect(const AndroidDrive *drive) const{
    return this->_settings.value(drive->id() + "_connectAutomatically", true).toBool();
}

void Settings::setAutoConnect(const AndroidDrive *drive, bool autoConnect){
    this->_settings.setValue(drive->id() + "_connectAutomatically", autoConnect);
}

bool Settings::openInExplorer() const{
    return this->_settings.value("openInExplorer", true).toBool();
}

void Settings::setOpenInExplorer(bool openInExplorer){
    this->_settings.setValue("openInExplorer", openInExplorer);
}

bool Settings::hideDotFiles() const{
    return this->_settings.value("hideDotFiles", true).toBool();
}

void Settings::setHideDotFiles(bool hideDotFiles){
    this->_settings.setValue("hideDotFiles", hideDotFiles);
}

QString Settings::language() const{
    return this->_settings.value("language", "auto").toString();
}

void Settings::setLanguage(const QString &language){
    this->_settings.setValue("language", language);
}
