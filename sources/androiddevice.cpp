#include "androiddevice.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>

#include "androiddrive.h"

QList<AndroidDevice*> AndroidDevice::_instances;
bool AndroidDevice::_quitOnLastDeletedDevice = false;

AndroidDevice::AndroidDevice(const QString &serialNumber):
    _serialNumber(serialNumber)
{
    AndroidDevice::_instances.push_back(this);

    //Get the paths to all the SD cards
    const QString internalStoragePath = this->runAdbCommand("realpath /sdcard");
    static const QRegularExpression newlineRegex("[\r\n]+"), spaceRegex("\\s+");
    const QStringList internalStorageDfOutput = this->runAdbCommand("df /sdcard").split(newlineRegex);
    if(internalStorageDfOutput.size() > 1){
        const QString storageFilesystem = internalStorageDfOutput[1].split(spaceRegex)[0];
        const QStringList allDfOutput = this->runAdbCommand("df").split(newlineRegex);
        for(const QString &dfOutput: allDfOutput){
            const QStringList values = dfOutput.split(spaceRegex);
            if(values.size() > 5){
                const QString filesystem = values[0];
                static const QRegularExpression storageRegex("^/mnt/.*media");
                if(filesystem != storageFilesystem && !filesystem.contains(storageRegex)){
                    continue;
                }
                QString androidPath = values[5];
                if(internalStoragePath.contains(androidPath)){
                    androidPath = "/sdcard";
                }
                this->addDrive(androidPath);
            }
        }
    }

    //If the above failed, just add /sdcard
    if(this->_drives.empty()){
        this->addDrive("/sdcard");
    }
}

AndroidDevice::~AndroidDevice(){
    AndroidDevice::_instances.removeAll(this);
    if(AndroidDevice::_instances.isEmpty() && AndroidDevice::_quitOnLastDeletedDevice){
        qApp->quit();
    }
}

void AndroidDevice::quitOnLastDeletedDevice(){
    AndroidDevice::_quitOnLastDeletedDevice = true;
    if(AndroidDevice::_instances.isEmpty()){
        qApp->quit();
    }
}

void AndroidDevice::connectAllDrives(){
    Settings settings;
    for(const std::unique_ptr<AndroidDrive> &drive: this->_drives){
        drive->connectDrive(settings.driveLetter(drive.get()), this->shared_from_this());
    }
}

void AndroidDevice::autoconnectAllDrives(){
    Settings settings;
    for(const std::unique_ptr<AndroidDrive> &drive: this->_drives){
        if(settings.autoConnect(drive.get())){
            drive->connectDrive(settings.driveLetter(drive.get()), this->shared_from_this());
        }
    }
}

void AndroidDevice::disconnectAllDrives(){
    for(const std::unique_ptr<AndroidDrive> &drive: this->_drives){
        drive->disconnectDrive();
    }
}

int AndroidDevice::numberOfDrives() const{
    return static_cast<int>(this->_drives.size());
}

int AndroidDevice::numberOfConnectedDrives() const{
    int result = 0;
    for(const std::unique_ptr<AndroidDrive> &drive: this->_drives){
        result += drive->isConnected();
    }
    return result;
}

bool AndroidDevice::isParentOfDrive(const AndroidDrive *drive) const{
    for(const std::unique_ptr<AndroidDrive> &otherDrive: this->_drives){
        if(drive == otherDrive.get()){
            return true;
        }
    }
    return false;
}

AndroidDrive *AndroidDevice::driveAt(int index) const{
    if(index < 0 || index > this->numberOfDrives()){
        return nullptr;
    }
    return this->_drives[index].get();
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

void AndroidDevice::addDrive(QString androidRootPath){
    std::unique_ptr<AndroidDrive> drive = std::make_unique<AndroidDrive>(std::move(androidRootPath), this->model(), this->serialNumber());
    QObject::connect(drive.get(), &AndroidDrive::driveConnected, this, [this, drive = drive.get()](){emit this->driveConnected(drive);});
    QObject::connect(drive.get(), &AndroidDrive::driveMounted, this, [this, drive = drive.get()](char driveLetter){emit this->driveMounted(drive, driveLetter);});
    QObject::connect(drive.get(), &AndroidDrive::driveUnmounted, this, [this, drive = drive.get()](){emit this->driveUnmounted(drive);});
    QObject::connect(drive.get(), &AndroidDrive::driveDisconnected, this, [this, drive = drive.get()](int status){emit this->driveDisconnected(drive, status);});
    this->_drives.push_back(std::move(drive));
}
