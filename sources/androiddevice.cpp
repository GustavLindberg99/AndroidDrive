#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include "androiddevice.h"
#include "androiddrive.h"

QList<AndroidDevice*> AndroidDevice::_instances;
std::function<void()> AndroidDevice::_callbackOnLastShutdown = [](){};

AndroidDevice::AndroidDevice(const QString &serialNumber):
    _serialNumber(serialNumber)
{
    AndroidDevice::_instances.append(this);

    //Get the paths to all the SD cards
    const QString internalStoragePath = this->runAdbCommand("realpath /sdcard");
    static const QRegularExpression newlineRegex("[\r\n]+"), spaceRegex("\\s+");
    const QStringList internalStorageDfOutput = this->runAdbCommand("df /sdcard").split(newlineRegex);
    if(internalStorageDfOutput.size() > 1){
        const QString storageFilesystem = internalStorageDfOutput[1].split(spaceRegex)[0];
        const QStringList allDfOutput = this->runAdbCommand(QString("df | grep %1").arg(storageFilesystem)).split(newlineRegex);
        for(const QString &dfOutput: allDfOutput){
            const QStringList values = dfOutput.split(spaceRegex);
            if(values.size() > 5){
                QString androidPath = dfOutput.split(spaceRegex)[5];
                if(internalStoragePath.contains(androidPath)){
                    androidPath = "/sdcard";
                }
                AndroidDrive *drive = new AndroidDrive(this, androidPath);
                this->_drives.append(drive);
                QObject::connect(drive, &AndroidDrive::driveConnected, this, [this, drive](){emit this->driveConnected(drive);});
                QObject::connect(drive, &AndroidDrive::driveMounted, this, [this, drive](char driveLetter){emit this->driveMounted(drive, driveLetter);});
                QObject::connect(drive, &AndroidDrive::driveUnmounted, this, [this, drive](){emit this->driveUnmounted(drive);});
                QObject::connect(drive, &AndroidDrive::driveDisconnected, this, [this, drive](int status){emit this->driveDisconnected(drive, status);});
            }
        }
    }

    //If the above failed, just add /sdcard
    if(this->_drives.empty()){
        this->_drives.append(new AndroidDrive(this, "/sdcard"));
    }
}

AndroidDevice::~AndroidDevice(){
    for(AndroidDrive *drive: this->_drives){
        delete drive;
    }

    AndroidDevice::_instances.removeAll(this);
    if(AndroidDevice::_instances.isEmpty()){
        AndroidDevice::_callbackOnLastShutdown();
        AndroidDevice::_callbackOnLastShutdown = [](){};
    }
}

void AndroidDevice::shutdown(){
    if(this->_willBeDeleted){
        return;
    }
    this->_willBeDeleted = true;
    if(this->numberOfConnectedDrives() == 0){
        delete this;
    }
    else{
        QObject::connect(this, &AndroidDevice::driveDisconnected, this, [this](){
            if(this->numberOfConnectedDrives() == 0){
                delete this;
            }
        });
        this->disconnectAllDrives();
    }
}

void AndroidDevice::shutdownAllDevices(const std::function<void()> &callback){
    AndroidDevice::_callbackOnLastShutdown = callback;
    const QList<AndroidDevice*> instances = AndroidDevice::_instances;
    if(instances.isEmpty()){
        callback();
    }
    else for(AndroidDevice *device: instances){
        device->shutdown();
    }
}

void AndroidDevice::disconnectAllDrives(){
    for(AndroidDrive *drive: this->_drives){
        drive->disconnectDrive();
    }
}

int AndroidDevice::numberOfDrives() const{
    return this->_drives.size();
}

int AndroidDevice::numberOfConnectedDrives() const{
    int result = 0;
    for(const AndroidDrive *drive: this->_drives){
        result += drive->isConnected();
    }
    return result;
}

QList<AndroidDrive*> AndroidDevice::drives() const{
    return this->_drives;
}

QString AndroidDevice::runAdbCommand(const QString &command, bool *ok, bool useCache) const{
    if(useCache && this->_adbCache.contains(command) && QDateTime::currentMSecsSinceEpoch() - this->_adbCache.value(command).second < 1000){
        if(ok != nullptr){
            *ok = true;
        }
        return this->_adbCache.value(command).first;
    }

    QProcess adb;
    adb.start("adb.exe", {"-s", this->_serialNumber, "shell", command});
    adb.waitForFinished(-1);
    const int exitCode = adb.exitCode();
    if(ok != nullptr){
        *ok = exitCode == 0;
    }
    if(exitCode == 0){
        const QString result = adb.readAllStandardOutput().trimmed();
        if(useCache){
            this->_adbCache[command] = QPair<QString, qint64>(result, QDateTime::currentMSecsSinceEpoch());
        }
        return result;
    }
    else{
        return "";
    }
}

bool AndroidDevice::pullFromAdb(const QString &remoteFile, const QString &localFile) const{
    QProcess adb;
    adb.start("adb.exe", {"-s", this->_serialNumber, "pull", remoteFile, localFile});
    adb.waitForFinished(-1);
    return adb.exitCode() == 0;
}

bool AndroidDevice::pushToAdb(const QString &localFile, const QString &remoteFile) const{
    QProcess adb;
    adb.start("adb.exe", {"-s", this->_serialNumber, "push", localFile, remoteFile});
    adb.waitForFinished(-1);
    return adb.exitCode() == 0;
}

QString AndroidDevice::model() const{
    if(this->_cachedModel.isEmpty()){
        this->_cachedModel = this->runAdbCommand("getprop ro.product.model");
        if(this->_cachedModel.isEmpty()){
            this->_cachedModel = this->_serialNumber;
        }
    }
    return this->_cachedModel;
}

QString AndroidDevice::serialNumber() const{
    return this->_serialNumber;
}
