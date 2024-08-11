#include "androiddrive.h"

#include <QRegularExpression>
#include <QThread>

#include "androiddevice.h"
#include "dokanoperations.h"

QList<AndroidDrive*> AndroidDrive::_instances;
DWORD AndroidDrive::_internalLogicalDrives = 0;

AndroidDrive::AndroidDrive(AndroidDevice *device, const QString &androidRootPath):
    _device(device),
    _androidRootPath(androidRootPath)
{
    AndroidDrive::_instances.append(this);

    ZeroMemory(&this->_dokanOptions, sizeof(DOKAN_OPTIONS));
    this->_dokanOptions.Version = DOKAN_VERSION;
    this->_dokanOptions.MountPoint = this->_mountPoint;    //this->_dokanOptions.MountPoint and this->_mountPoint will point to the same memory address, which will be used in connectDrive() and fromDokanFileInfo()
    this->_dokanOptions.Options |= DOKAN_OPTION_ALT_STREAM;

    ZeroMemory(&this->_dokanOperations, sizeof(DOKAN_OPERATIONS));
    this->_dokanOperations.ZwCreateFile = createFile;
    this->_dokanOperations.CloseFile = closeFile;
    this->_dokanOperations.Cleanup = cleanup;
    this->_dokanOperations.ReadFile = readFile;
    this->_dokanOperations.WriteFile = writeFile;
    this->_dokanOperations.FlushFileBuffers = flushFileBuffers;
    this->_dokanOperations.GetFileInformation = getFileInformation;
    this->_dokanOperations.FindFiles = findFiles;
    this->_dokanOperations.SetFileAttributes = setFileAttributes;
    this->_dokanOperations.SetFileTime = setFileTime;
    this->_dokanOperations.DeleteFile = deleteFile;
    this->_dokanOperations.DeleteDirectory = deleteDirectory;
    this->_dokanOperations.MoveFile = moveFile;
    this->_dokanOperations.SetEndOfFile = this->_dokanOperations.SetAllocationSize = setAllocationSize;
    this->_dokanOperations.GetDiskFreeSpace = getDiskFreeSpace;
    this->_dokanOperations.GetVolumeInformation = getVolumeInformation;
    this->_dokanOperations.Unmounted = unmounted;
    this->_dokanOperations.Mounted = mounted;

    QObject::connect(this, &AndroidDrive::driveMounted, [this](){
        this->_mounted = true;
        if(this->_shouldBeDisconnected){
            this->disconnectDrive();
        }
    });
    QObject::connect(this, &AndroidDrive::driveUnmounted, [this](){
        this->_mounted = false;
    });
}

AndroidDrive::~AndroidDrive(){
    AndroidDrive::_instances.removeAll(this);
}

AndroidDrive *AndroidDrive::fromDokanFileInfo(PDOKAN_FILE_INFO dokanFileInfo){
    const QList<AndroidDrive*> instances = AndroidDrive::_instances;
    for(AndroidDrive *drive: instances){
        //device->_mountPoint and dokanOptions->MountPoint point to the same memory address if they belong to the same device, so comparing them by reference as below is a reliable way to tell which device goes with which DokanOptions object
        if(drive->_mountPoint == dokanFileInfo->DokanOptions->MountPoint){
            return drive;
        }
    }
    return nullptr;
}

void AndroidDrive::connectDrive(char driveLetter){
    class Thread: public QThread{
    public:
        Thread(AndroidDrive *drive): _drive(drive){}

    protected:
        virtual void run() override{
            const int status = DokanMain(&this->_drive->_dokanOptions, &this->_drive->_dokanOperations);
            delete this->_drive->_temporaryDir;
            this->_drive->_temporaryDir = nullptr;
            this->_drive->_thread = nullptr;
            AndroidDrive::_internalLogicalDrives &= ~(1 << (this->_drive->_mountPoint[0] - 'A'));
            emit this->_drive->driveDisconnected(status);
            this->deleteLater();
        }

    private:
        AndroidDrive *_drive;
    };

    if(this->_thread == nullptr){
        const DWORD driveLetters = GetLogicalDrives() | AndroidDrive::_internalLogicalDrives;    //Bitmask where 1 means the drive is occupied and 0 means the drive is available. The least significant bit corresponds to A:\, the second least significant bit corresponds to B:\, etc. For example, 14 = 0b1110 means that B:\, C:\ and D:\ are occupied and everything else is available.
        for(int i = 0; (driveLetters & (1 << (driveLetter - 'A'))) && i < 26; i++){
            driveLetter++;
            if(driveLetter == 'Z' + 1){
                driveLetter = 'A';
            }
        }

        AndroidDrive::_internalLogicalDrives |= 1 << (driveLetter - 'A');
        this->_temporaryDir = new QTemporaryDir();
        this->_mountPoint[0] = driveLetter;    //We don't need to change dokanOptions.MountPoint because it points to the same memory address as this->_mountPoint
        this->_mountPointRemoved = false;
        this->_thread = new Thread(this);
        this->_thread->start();
        emit this->driveConnected();
    }
}

void AndroidDrive::disconnectDrive(){
    if(this->_thread != nullptr && !this->_mountPointRemoved){
        this->_shouldBeDisconnected = !this->_mounted;
        if(this->_mounted){
            DokanRemoveMountPoint(this->_mountPoint);
            this->_mountPointRemoved = true;
        }
    }
}

bool AndroidDrive::isConnected() const{
    return this->_thread != nullptr;
}

AndroidDevice *AndroidDrive::device() const{
    return this->_device;
}

QString AndroidDrive::fileSystem() const{
    if(!this->_fileSystemCached){
        const QString output = this->device()->runAdbCommand("mount | grep $(df /sdcard | sed \"s/.* //g\" | tail -n +2)");
        static const QRegularExpression fileSystemRegex("\\S+\\s+on\\s+\\S+\\s+type\\s+([a-zA-Z0-9]+)\\s+");
        const QRegularExpressionMatch match = fileSystemRegex.match(output);
        this->_cachedFileSystem = match.hasMatch() ? match.captured(1) : "";
        this->_fileSystemCached = true;
    }
    return this->_cachedFileSystem;
}

QString AndroidDrive::name() const{
    if(this->_androidRootPath == "/sdcard"){
        return QObject::tr("Internal storage");
    }
    const QString sdCardName = QFileInfo(this->_androidRootPath).baseName();
    return QObject::tr("SD card %1").arg(sdCardName);
}

QString AndroidDrive::completeName() const{
    if(this->device()->numberOfDrives() == 1){
        return this->device()->model();
    }
    return this->device()->model() + " " + this->name();
}

QString AndroidDrive::id() const{
    return this->device()->serialNumber() + this->androidRootPath();
}

QString AndroidDrive::androidRootPath() const{
    return this->_androidRootPath;
}

QString AndroidDrive::localPath(const QString &remotePath) const{
    static const QRegularExpression leadingSlashes("^[/\\\\]+");
    const QString remoteRelativePath = QString(remotePath).replace(leadingSlashes, "");
    const QString result = QDir::toNativeSeparators(this->_temporaryDir->filePath(remoteRelativePath));
    QFileInfo(result).dir().mkpath(".");
    return result;
}

QString AndroidDrive::windowsPathToAndroidPath(LPCWSTR windowsPath) const{
    QString androidPath = this->_androidRootPath + QString::fromWCharArray(windowsPath).replace("\\", "/").split(":")[0];
    //Use the same trick as WSL for characters that are allowed on Android but not on Windows (this trick consists in replacing a special character with a Unicode version by adding 0xf000 to its char code)
    const char specialCharacters[] = {'\\', ':', '*', '?', '"', '<', '>', '|'};
    for(const char character: specialCharacters){
        androidPath.replace(QChar(character + 0xf000), QChar(character));
    }
    return androidPath;
}

QString AndroidDrive::mountPoint() const{
    return QString::fromWCharArray(this->_mountPoint);
}
