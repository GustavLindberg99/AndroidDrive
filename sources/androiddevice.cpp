#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include "androiddevice.h"
#include "dokanoperations.h"

QList<AndroidDevice*> AndroidDevice::_instances;
std::function<void()> AndroidDevice::_callbackOnLastShutdown = [](){};

AndroidDevice::AndroidDevice(const QString &serialNumber):
    _willBeDeleted(false),
    _serialNumber(serialNumber),
    _thread(nullptr),
    _mounted(false),
    _shouldBeDisconnected(false),
    _mountPointRemoved(false),
    _mountPoint{L"?:\\"},    //The ? is a placeholder, it will be changed when the drive gets connected
    _fileSystemCached(false)
{
    AndroidDevice::_instances.append(this);

    ZeroMemory(&this->_dokanOptions, sizeof(DOKAN_OPTIONS));
    this->_dokanOptions.Version = DOKAN_VERSION;
    this->_dokanOptions.MountPoint = this->_mountPoint;    //dokanOptions.MountPoint and this->_mountPoint will point to the same memory address, which will be used in connectDrive() and fromDokanFileInfo()

    ZeroMemory(&this->_dokanOperations, sizeof(DOKAN_OPERATIONS));
    this->_dokanOperations.ZwCreateFile = createFile;
    this->_dokanOperations.CloseFile = closeFile;
    this->_dokanOperations.Cleanup = cleanup;
    this->_dokanOperations.ReadFile = readFile;
    this->_dokanOperations.WriteFile = writeFile;
    this->_dokanOperations.FlushFileBuffers = flushFileBuffers;
    this->_dokanOperations.GetFileInformation = getFileInformation;
    this->_dokanOperations.FindFiles = findFiles;
    this->_dokanOperations.SetFileTime = setFileTime;
    this->_dokanOperations.DeleteFile = deleteFile;
    this->_dokanOperations.DeleteDirectory = deleteDirectory;
    this->_dokanOperations.MoveFile = moveFile;
    this->_dokanOperations.SetEndOfFile = this->_dokanOperations.SetAllocationSize = setAllocationSize;
    this->_dokanOperations.GetDiskFreeSpace = getDiskFreeSpace;
    this->_dokanOperations.GetVolumeInformation = getVolumeInformation;
    this->_dokanOperations.Unmounted = unmounted;
    this->_dokanOperations.Mounted = mounted;

    QObject::connect(this, &AndroidDevice::driveMounted, [this](){
        this->_mounted = true;
        if(this->_shouldBeDisconnected){
            this->disconnectDrive();
        }
    });
    QObject::connect(this, &AndroidDevice::driveUnmounted, [this](){
        this->_mounted = false;
    });
}

AndroidDevice::~AndroidDevice(){
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
    if(this->_thread == nullptr){
        delete this;
    }
    else{
        QObject::connect(this, &AndroidDevice::driveDisconnected, [this](){
            delete this;
        });
        this->disconnectDrive();
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

AndroidDevice *AndroidDevice::fromDokanFileInfo(PDOKAN_FILE_INFO dokanFileInfo){
    const QList<AndroidDevice*> instances = AndroidDevice::_instances;
    for(AndroidDevice *device: instances){
        //device->_mountPoint and dokanOptions->MountPoint point to the same memory address if they belong to the same device, so comparing them by reference as below is a reliable way to tell which device goes with which DokanOptions object
        if(device->_mountPoint == dokanFileInfo->DokanOptions->MountPoint){
            return device;
        }
    }
    return nullptr;
}

void AndroidDevice::connectDrive(char driveLetter){
    const DWORD drives = GetLogicalDrives();    //Bitmask where 1 means the drive is occupied and 0 means the drive is available. The least significant bit corresponds to A:\, the second least significant bit corresponds to B:\, etc. For example, 14 = 0b1110 means that B:\, C:\ and D:\ are occupied and everything else is available.
    for(int i = 0; (drives & (1 << (driveLetter - 'A'))) && i < 26; i++){
        driveLetter++;
        if(driveLetter == 'Z' + 1){
            driveLetter = 'A';
        }
    }

    class Thread: public QThread{
    public:
        Thread(AndroidDevice *device): _device(device){}

    protected:
        virtual void run() override{
            const int status = DokanMain(&this->_device->_dokanOptions, &this->_device->_dokanOperations);
            emit this->_device->driveDisconnected(status);
            this->_device->_thread = nullptr;
            this->deleteLater();
        }

    private:
        AndroidDevice *_device;
    };

    if(this->_thread == nullptr){
        this->_mountPoint[0] = driveLetter;    //We don't need to change dokanOptions.MountPoint because it points to the same memory address as this->_mountPoint
        this->_mountPointRemoved = false;
        this->_thread = new Thread(this);
        this->_thread->start();
        emit this->driveConnected();
    }
}

void AndroidDevice::disconnectDrive(){
    if(this->_thread != nullptr && !this->_mountPointRemoved){
        this->_shouldBeDisconnected = !this->_mounted;
        if(this->_mounted){
            DokanRemoveMountPoint(this->_mountPoint);
            this->_mountPointRemoved = true;
        }
    }
}

bool AndroidDevice::isConnected() const{
    return this->_thread != nullptr;
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

QString AndroidDevice::fileSystem() const{
    if(!this->_fileSystemCached){
        const QString output = this->runAdbCommand("mount | grep $(df /sdcard | sed \"s/.* //g\" | tail -n +2)");
        static const QRegularExpression fileSystemRegex("\\S+\\s+on\\s+\\S+\\s+type\\s+([a-zA-Z0-9]+)\\s+");
        const QRegularExpressionMatch match = fileSystemRegex.match(output);
        this->_cachedFileSystem = match.hasMatch() ? match.captured(1) : "";
        this->_fileSystemCached = true;
    }
    return this->_cachedFileSystem;
}

QString AndroidDevice::serialNumber() const{
    return this->_serialNumber;
}
