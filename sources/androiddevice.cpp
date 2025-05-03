#include "androiddevice.hpp"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>

#include "androiddrive.hpp"
#include "debuglogger.hpp"

QList<AndroidDevice*> AndroidDevice::_instances;
bool AndroidDevice::_quitOnLastDeletedDevice = false;

AndroidDevice::AndroidDevice(const QString &serialNumber):
    _serialNumber(serialNumber)
{
    AndroidDevice::_instances.push_back(this);

    DebugLogger::getInstance().log("Constructing device '{}'", serialNumber);

    //Get the paths to all the SD cards
    const QString internalStoragePath = this->runAdbCommand("realpath /sdcard");
    DebugLogger::getInstance().log("Device '{}': Internal storage path: {}", std::make_tuple(serialNumber, internalStoragePath));
    static const QRegularExpression newlineRegex("[\r\n]+"), spaceRegex("\\s+");
    const QString internalStorageDfOutput = this->runAdbCommand("df /sdcard");
    DebugLogger::getInstance().log("Device '{}': output of `df /sdcard`: {}", std::make_tuple(serialNumber, internalStorageDfOutput));
    const QStringList splittedInternalStorageDfOutput = internalStorageDfOutput.split(newlineRegex);
    if(splittedInternalStorageDfOutput.size() > 1){
        const QString storageFilesystem = splittedInternalStorageDfOutput[1].split(spaceRegex)[0];
        DebugLogger::getInstance().log("Device '{}': Storage file system: {}", std::make_tuple(serialNumber, storageFilesystem));
        const QStringList allDfOutput = this->runAdbCommand("df").split(newlineRegex);
        for(const QString &dfOutput: allDfOutput){
            const QStringList values = dfOutput.split(spaceRegex);
            if(values.size() > 5){
                DebugLogger::getInstance().log("Device '{}': Reading df output line {}", std::make_tuple(serialNumber, dfOutput));
                const QString filesystem = values[0];
                static const QRegularExpression storageRegex("^/mnt/.*media");
                if(filesystem != storageFilesystem && !filesystem.contains(storageRegex)){
                    DebugLogger::getInstance().log("Device '{}': File system '{}' does not match regex", std::make_tuple(serialNumber, filesystem));
                    continue;
                }
                QString androidPath = values[5];
                if(internalStoragePath.contains(androidPath)){
                    androidPath = "/sdcard";
                }
                DebugLogger::getInstance().log("Device '{}': Adding drive at Android path {}", std::make_tuple(serialNumber, androidPath));
                this->addDrive(androidPath);
            }
            else{
                DebugLogger::getInstance().log("Device '{}': Skipping df output line {}", std::make_tuple(serialNumber, dfOutput));
            }
        }
    }

    //If the above failed, just add /sdcard
    if(this->_drives.empty()){
        DebugLogger::getInstance().log("Device '{}': df output failed, adding /sdcard", serialNumber);
        this->addDrive("/sdcard");
    }
}

AndroidDevice::~AndroidDevice(){
    DebugLogger::getInstance().log("Deleting device '{}'", this->serialNumber());
    AndroidDevice::_instances.removeAll(this);
    if(AndroidDevice::_instances.isEmpty() && AndroidDevice::_quitOnLastDeletedDevice){
        DebugLogger::getInstance().log("Quitting");
        qApp->quit();
    }
}

void AndroidDevice::quitOnLastDeletedDevice(){
    DebugLogger::getInstance().log("Will quit on last deleted device");
    AndroidDevice::_quitOnLastDeletedDevice = true;
    if(AndroidDevice::_instances.isEmpty()){
        DebugLogger::getInstance().log("Quitting because device list is empty");
        qApp->quit();
    }
}

void AndroidDevice::connectAllDrives(){
    Settings settings;
    for(const std::unique_ptr<AndroidDrive> &drive: this->_drives){
        DebugLogger::getInstance().log("Connecting drive '{}'", drive.get());
        drive->connectDrive(settings.driveLetter(drive.get()), this->shared_from_this());
    }
}

void AndroidDevice::autoconnectAllDrives(){
    Settings settings;
    for(const std::unique_ptr<AndroidDrive> &drive: this->_drives){
        if(settings.autoConnect(drive.get())){
            DebugLogger::getInstance().log("Autoconnecting drive '{}'", drive.get());
            drive->connectDrive(settings.driveLetter(drive.get()), this->shared_from_this());
        }
        else{
            DebugLogger::getInstance().log("Not autoconnecting drive '{}', autoconnecting is disabled for this drive", drive.get());
        }
    }
}

void AndroidDevice::disconnectAllDrives(){
    for(const std::unique_ptr<AndroidDrive> &drive: this->_drives){
        DebugLogger::getInstance().log("Disconnecting drive '{}'", drive.get());
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
        const QString result = this->_adbCache.value(command).first;
        DebugLogger::getInstance().log("Device '{}': ADB command `{}` is cached, using previous result: {}", std::make_tuple(this->serialNumber(), command, result));
        return result;
    }

    DebugLogger::getInstance().log("Running ADB command `{}` on device '{}'", std::make_tuple(command, this->serialNumber()));
    QProcess adb;
    adb.start("adb.exe", {"-s", this->_serialNumber, "shell", command});
    adb.waitForFinished(-1);
    const int exitCode = adb.exitCode();
    DebugLogger::getInstance().log("Finished running ADB command `{}` on device '{}'. Exit code: {}.", std::make_tuple(command, this->serialNumber(), exitCode));
    if(ok != nullptr){
        *ok = exitCode == 0;
    }
    if(exitCode == 0){
        const QString result = adb.readAllStandardOutput().trimmed();
        DebugLogger::getInstance().log("Standard output: {}", result);
        if(useCache){
            this->_adbCache[command] = QPair<QString, qint64>(result, QDateTime::currentMSecsSinceEpoch());
        }
        return result;
    }
    else{
        DebugLogger::getInstance().log("Returning empty string because ADB command failed");
        return "";
    }
}

bool AndroidDevice::pullFromAdb(const QString &remoteFile, const QString &localFile) const{
    QProcess adb;
    DebugLogger::getInstance().log("Pulling '{}' to '{}'", std::make_tuple(remoteFile, localFile));
    adb.start("adb.exe", {"-s", this->_serialNumber, "pull", remoteFile, localFile});
    adb.waitForFinished(-1);
    const int exitCode = adb.exitCode();
    DebugLogger::getInstance().log("Pulling finished. Exit code: {}, standard output: '{}', standard error: '{}'", std::make_tuple(exitCode, adb.readAllStandardOutput().toStdString(), adb.readAllStandardError().toStdString()));
    return exitCode == 0;
}

bool AndroidDevice::pushToAdb(const QString &localFile, const QString &remoteFile) const{
    QProcess adb;
    DebugLogger::getInstance().log("Pushing '{}' to '{}'", std::make_tuple(localFile, remoteFile));
    adb.start("adb.exe", {"-s", this->_serialNumber, "push", localFile, remoteFile});
    adb.waitForFinished(-1);
    const int exitCode = adb.exitCode();
    DebugLogger::getInstance().log("Pushing finished. Exit code: {}, standard output: '{}', standard error: '{}'", std::make_tuple(exitCode, adb.readAllStandardOutput().toStdString(), adb.readAllStandardError().toStdString()));
    return exitCode == 0;
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
