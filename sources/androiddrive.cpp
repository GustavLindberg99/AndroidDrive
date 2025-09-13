#include "androiddrive.hpp"

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QThread>

#include "androiddevice.hpp"
#include "debuglogger.hpp"
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
        DebugLogger::getInstance().log("Drive '{}' mounted", this);
        if(this->_shouldBeDisconnected){
            DebugLogger::getInstance().log("Disconnecting drive '{}'", this);
            this->disconnectDrive();
        }
    });
    QObject::connect(this, &AndroidDrive::driveUnmounted, this, [this](){
        this->_mounted = false;
        DebugLogger::getInstance().log("Drive '{}' unmounted", this);
    });

    //Do the cleanup after Dokan exit here to make sure that it's run on the main thread instead of the Dokan thread (otherwise the destructors will be called in the Dokan thread so the thread will try to join itself and crash).
    //Qt::ConnectionType::AutoConnection is the default so this will be run on the main thread since the connection is done on the main thread.
    QObject::connect(this, &AndroidDrive::driveDisconnected, this, [this](){
        DebugLogger::getInstance().log("Disconnecting drive '{}', waiting for mutex", this);
        std::lock_guard<std::mutex> lockGuard(this->_mutex);
        DebugLogger::getInstance().log("Disconnecting drive '{}', mutex locked", this);
        this->_temporaryFiles.clear();
        this->_temporaryDir = nullptr;
        this->_device = nullptr;
        AndroidDrive::_internalLogicalDrives &= ~(1 << (this->_mountPoint[0] - 'A'));
    });
}

AndroidDrive::~AndroidDrive(){
    this->disconnectDrive();
    if(this->_thread.joinable()){
        DebugLogger::getInstance().log("Joining Dokan thread for drive '{}'", this);
        this->_thread.join();
    }
    DebugLogger::getInstance().log("Deleting drive '{}'", this);
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
            DebugLogger::getInstance().log("Joining Dokan thread for drive '{}'", this);
            this->_thread.join();
        }

        DebugLogger::getInstance().log("Attempting to connect drive '{}' with drive letter '{}'", std::make_tuple(this, driveLetter));
        const DWORD driveLetters = GetLogicalDrives() | AndroidDrive::_internalLogicalDrives;    //Bitmask where 1 means the drive is occupied and 0 means the drive is available. The least significant bit corresponds to A:\, the second least significant bit corresponds to B:\, etc. For example, 14 = 0b1110 means that B:\, C:\ and D:\ are occupied and everything else is available.
        for(int i = 0; (driveLetters & (1 << (driveLetter - 'A'))) && i < 26; i++){
            driveLetter++;
            if(driveLetter == 'Z' + 1){
                driveLetter = 'A';
            }
        }
        DebugLogger::getInstance().log("Connecting drive '{}' with drive letter '{}'", std::make_tuple(this, driveLetter));

        AndroidDrive::_internalLogicalDrives |= 1 << (driveLetter - 'A');
        this->_shouldBeDisconnected = false;
        this->_temporaryDir = std::make_unique<QTemporaryDir>();
        DebugLogger::getInstance().log("Creating temporary folder '{}' for drive '{}'", std::make_tuple(this->_temporaryDir->path(), this));
        this->_device = device;
        this->_mountPoint[0] = driveLetter;    //We don't need to change dokanOptions.MountPoint because it points to the same memory address as this->_mountPoint

        const QString output = this->device()->runAdbCommand("mount | grep $(df /sdcard | sed \"s/.* //g\" | tail -n +2)");
        static const QRegularExpression fileSystemRegex("\\S+\\s+on\\s+\\S+\\s+type\\s+([a-zA-Z0-9]+)\\s+");
        const QRegularExpressionMatch match = fileSystemRegex.match(output);
        this->_fileSystem = match.hasMatch() ? match.captured(1) : "";

        this->_thread = std::thread([this](){
            DebugLogger::getInstance().log("Starting Dokan thread for drive '{}'", this);
            const int status = DokanMain(&this->_dokanOptions, &this->_dokanOperations);
            emit this->driveDisconnected(status);
            DebugLogger::getInstance().log("Exiting Dokan thread for drive '{}'", this);
        });
        emit this->driveConnected();
    }
}

void AndroidDrive::disconnectDrive(){
    if(this->isConnected()){
        this->_shouldBeDisconnected = true;
        DebugLogger::getInstance().log("Removing Dokan mount point for drive '{}'", this);
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
    //Hash the path to avoid problems with paths that are too long, see issue #54
    const QString hashedRelativePath = QCryptographicHash::hash(remoteRelativePath.toUtf8(), QCryptographicHash::Md4).toHex();
    const QString result = QDir::toNativeSeparators(this->_temporaryDir->filePath(hashedRelativePath));
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
    DebugLogger::getInstance().log("Adding temporary file for '{}', waiting for mutex", remotePath);
    std::lock_guard<std::mutex> lockGuard(this->_mutex);
    DebugLogger::getInstance().log("Adding temporary file for '{}', mutex locked", remotePath);
    if(this->_device == nullptr){
        DebugLogger::getInstance().log("Not adding temporary file, device is disconnected");
        return STATUS_ALREADY_DISCONNECTED;
    }
    std::unique_ptr<TemporaryFile> temporaryFile = std::make_unique<TemporaryFile>(this, remotePath, creationDisposition, shareAccess, desiredAccess, fileAttributes, createOptions, createDisposition, exists, altStream);
    const NTSTATUS errorCode = temporaryFile->errorCode();

    //If there's no error, move the unique_ptr to the list of temporary files to keep it in memory, otherwise do nothing and it will be deleted at the end of the scope.
    if(errorCode == STATUS_SUCCESS){
        DebugLogger::getInstance().log("Temporary file for '{}' created successfully", remotePath);
        dokanFileInfo->Context = reinterpret_cast<ULONG64>(temporaryFile.get());
        this->_temporaryFiles.push_back(std::move(temporaryFile));
    }
    else{
        DebugLogger::getInstance().log("Failed to create temporary file for '{}': error {}", std::make_tuple(remotePath, errorCode));
    }

    return errorCode;
}

void AndroidDrive::deleteTemporaryFile(PDOKAN_FILE_INFO dokanFileInfo){
    DebugLogger::getInstance().log("Deleting temporary file on drive '{}', waiting for mutex", this);
    std::lock_guard<std::mutex> lockGuard(this->_mutex);
    DebugLogger::getInstance().log("Deleting temporary file on drive '{}', mutex locked", this);
    TemporaryFile *temporaryFile = reinterpret_cast<TemporaryFile*>(dokanFileInfo->Context);
    dokanFileInfo->Context = 0;
    for(size_t i = 0; i < this->_temporaryFiles.size(); i++){
        if(this->_temporaryFiles[i].get() == temporaryFile){
            DebugLogger::getInstance().log("Temporary file found in list of temporary files");
            this->_temporaryFiles.erase(this->_temporaryFiles.begin() + i);
            break;
        }
    }
}

void AndroidDrive::openSettingsWindow(){
    this->_settingsWindow.show();
}
