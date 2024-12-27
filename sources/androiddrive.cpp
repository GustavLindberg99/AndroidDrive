#include "androiddrive.hpp"

#include <QRegularExpression>
#include <QThread>

#include "androiddevice.hpp"
#include "dokanoperations.hpp"

QList<AndroidDrive*> AndroidDrive::_instances;
DWORD AndroidDrive::_internalLogicalDrives = 0;

AndroidDrive::AndroidDrive(QString androidRootPath, QString model, QString serialNumber):
    _androidRootPath(std::move(androidRootPath)),
    _model(std::move(model)),
    _serialNumber(std::move(serialNumber)),
    _settingsWindow(this)
{
    AndroidDrive::_instances.push_back(this);

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

    QObject::connect(this, &AndroidDrive::driveMounted, this, [this](){
        this->_mounted = true;
        if(this->_shouldBeDisconnected){
            this->disconnectDrive();
        }
    });
    QObject::connect(this, &AndroidDrive::driveUnmounted, this, [this](){
        this->_mounted = false;
    });

    //Do the cleanup after Dokan exit here to make sure that it's run on the main thread instead of the Dokan thread (otherwise the destructors will be called in the Dokan thread so the thread will try to join itself and crash).
    //Qt::ConnectionType::AutoConnection is the default so this will be run on the main thread since the connection is done on the main thread.
    QObject::connect(this, &AndroidDrive::driveDisconnected, this, [this](){
        std::lock_guard<std::mutex> lockGuard(this->_mutex);
        this->_temporaryFiles.clear();
        this->_temporaryDir = nullptr;
        this->_device = nullptr;
        AndroidDrive::_internalLogicalDrives &= ~(1 << (this->_mountPoint[0] - 'A'));
    });
}

AndroidDrive::~AndroidDrive(){
    this->disconnectDrive();
    if(this->_thread.joinable()){
        this->_thread.join();
    }
    AndroidDrive::_instances.removeAll(this);

    //Sometimes the mutex is already locked by the main thread when it gets destroyed. If that's the case, it must be unlocked otherwise it crashes. It can't be locked by another thread because the other thread was just joined.
    (void) this->_mutex.try_lock();
    this->_mutex.unlock();
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

void AndroidDrive::connectDrive(char driveLetter, const std::shared_ptr<AndroidDevice> &device){
    if(!this->isConnected()){
        if(this->_thread.joinable()){
            this->_thread.join();
        }

        const DWORD driveLetters = GetLogicalDrives() | AndroidDrive::_internalLogicalDrives;    //Bitmask where 1 means the drive is occupied and 0 means the drive is available. The least significant bit corresponds to A:\, the second least significant bit corresponds to B:\, etc. For example, 14 = 0b1110 means that B:\, C:\ and D:\ are occupied and everything else is available.
        for(int i = 0; (driveLetters & (1 << (driveLetter - 'A'))) && i < 26; i++){
            driveLetter++;
            if(driveLetter == 'Z' + 1){
                driveLetter = 'A';
            }
        }

        AndroidDrive::_internalLogicalDrives |= 1 << (driveLetter - 'A');
        this->_shouldBeDisconnected = false;
        this->_temporaryDir = std::make_unique<QTemporaryDir>();
        this->_device = device;
        this->_mountPoint[0] = driveLetter;    //We don't need to change dokanOptions.MountPoint because it points to the same memory address as this->_mountPoint

        const QString output = this->device()->runAdbCommand("mount | grep $(df /sdcard | sed \"s/.* //g\" | tail -n +2)");
        static const QRegularExpression fileSystemRegex("\\S+\\s+on\\s+\\S+\\s+type\\s+([a-zA-Z0-9]+)\\s+");
        const QRegularExpressionMatch match = fileSystemRegex.match(output);
        this->_fileSystem = match.hasMatch() ? match.captured(1) : "";

        this->_thread = std::thread([this](){
            const int status = DokanMain(&this->_dokanOptions, &this->_dokanOperations);
            emit this->driveDisconnected(status);
        });
        emit this->driveConnected();
    }
}

void AndroidDrive::disconnectDrive(){
    if(this->isConnected()){
        this->_shouldBeDisconnected = true;
        DokanRemoveMountPoint(this->_mountPoint);
    }
}

bool AndroidDrive::isConnected() const{
    return this->_temporaryDir != nullptr;
}

bool AndroidDrive::mountingInProgress() const{
    return !this->_mounted && !this->_shouldBeDisconnected && this->isConnected();
}

bool AndroidDrive::unmountingInProgress() const{
    return this->_shouldBeDisconnected && this->isConnected();
}

std::shared_ptr<AndroidDevice> AndroidDrive::device() const{
    return this->_device;
}

bool AndroidDrive::isInternalStorage() const{
    return this->_androidRootPath == "/sdcard";
}

QString AndroidDrive::fileSystem() const{
    return this->_fileSystem;
}

QString AndroidDrive::name() const{
    if(this->isInternalStorage()){
        return QObject::tr("Internal storage");
    }
    const QString sdCardName = QFileInfo(this->_androidRootPath).baseName();
    return QObject::tr("SD card %1").arg(sdCardName);
}

QString AndroidDrive::completeName() const{
    if(this->device() != nullptr && this->device()->numberOfDrives() == 1){
        return this->_model;
    }
    return this->_model + " " + this->name();
}

QString AndroidDrive::id() const{
    return this->_serialNumber + this->androidRootPath();
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

NTSTATUS AndroidDrive::addTemporaryFile(PDOKAN_FILE_INFO dokanFileInfo, const QString &remotePath, DWORD creationDisposition, ULONG shareAccess, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG createOptions, ULONG createDisposition, bool exists, const QString &altStream){
    std::lock_guard<std::mutex> lockGuard(this->_mutex);
    if(this->_device == nullptr){
        return STATUS_ALREADY_DISCONNECTED;
    }
    std::unique_ptr<TemporaryFile> temporaryFile = std::make_unique<TemporaryFile>(this, remotePath, creationDisposition, shareAccess, desiredAccess, fileAttributes, createOptions, createDisposition, exists, altStream);
    const NTSTATUS errorCode = temporaryFile->errorCode();

    //If there's no error, move the unique_ptr to the list of temporary files to keep it in memory, otherwise do nothing and it will be deleted at the end of the scope.
    if(errorCode == STATUS_SUCCESS){
        dokanFileInfo->Context = reinterpret_cast<ULONG64>(temporaryFile.get());
        this->_temporaryFiles.push_back(std::move(temporaryFile));
    }

    return errorCode;
}

void AndroidDrive::deleteTemporaryFile(PDOKAN_FILE_INFO dokanFileInfo){
    std::lock_guard<std::mutex> lockGuard(this->_mutex);
    TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
    dokanFileInfo->Context = 0;
    for(size_t i = 0; i < this->_temporaryFiles.size(); i++){
        if(this->_temporaryFiles[i].get() == temporaryFile){
            this->_temporaryFiles.erase(this->_temporaryFiles.begin() + i);
            break;
        }
    }
}

void AndroidDrive::openSettingsWindow(){
    this->_settingsWindow.show();
}
