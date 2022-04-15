#include <QProcess>
#include <QRegularExpression>
#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QMessageBox>
#include <QDateTime>
#include "androiddevice.h"
#include "devicelistwindow.h"

QTemporaryDir AndroidDevice::_temp;
QMap<QString, QString> AndroidDevice::_name;
QMap<QString, QString> AndroidDevice::_storageLocation;
QMap<QString, QFileSystemWatcher*> AndroidDevice::_fileSystemWatchers;
QMap<QString, QTimer*> AndroidDevice::_pullTimers;
extern DeviceListWindow *deviceListWindow;

AndroidDevice::AndroidDevice(const QString &id):
    _id(id),
    _driveLetter(QSettings("Gustav Lindberg", "AndroidDrive").value(this->id() + "_driveLetter", 'D').toChar().toLatin1())
{
    if(!this->_name.contains(this->id())){
        QProcess adb;
        adb.start("adb.exe", {"-s", this->id(), "shell", "getprop", "ro.product.model"});
        adb.waitForFinished(-1);
        this->_name.insert(this->id(), adb.readAllStandardOutput().trimmed());
    }

    if(!this->_storageLocation.contains(this->id())){
        QString storageLocation = "/sdcard";
        while(true){
            QProcess adb;
            adb.start("adb.exe", {"-s", this->id(), "shell", "readlink", storageLocation});
            adb.waitForFinished(-1);
            const QByteArray &output = adb.readAllStandardOutput();
            if(output.isEmpty()){
                break;
            }
            else{
                storageLocation = output.trimmed();
            }
        }
        this->_storageLocation.insert(this->id(), storageLocation);
    }

    this->checkDriveLetter();
}

bool AndroidDevice::operator==(const AndroidDevice &other) const{
    return this->id() == other.id();
}

bool AndroidDevice::operator!=(const AndroidDevice &other) const{
    return !(*this == other);
}

QString AndroidDevice::id() const{
    return this->_id;
}

QString AndroidDevice::name() const{
    if(this->_name[this->id()].isEmpty()){
        return this->id();
    }
    return this->_name[this->id()];
}

QString AndroidDevice::storageLocation() const{
    return this->_storageLocation[this->id()];
}

bool AndroidDevice::isValid() const{
    return AndroidDevice::allDevices().contains(*this);
}

void AndroidDevice::setDriveLetter(char letter){
    if(letter >= 'A' && letter <= 'Z'){
        this->disconnectDrive();    //This is safe to call even if the drive isn't connected
        this->_driveLetter = letter;
        QSettings("Gustav Lindberg", "AndroidDrive").setValue(this->id() + "_driveLetter", letter);
        this->checkDriveLetter();
        this->autoconnectDrive();
    }
}

char AndroidDevice::driveLetter() const{
    return this->_driveLetter;
}

void AndroidDevice::connectDrive(const std::function<void(const QString &)> &callback){
    if(this->isConnected()){
        return;
    }

    //Prepare the drive letter and the temporary folder
    this->checkDriveLetter();
    if(this->temporaryDir().exists()){
        this->temporaryDir().removeRecursively();
    }
    this->temporaryDir().mkpath("sdcard");

    //Use ADB to copy files from the Android device to the temporary folder
    QProcess *adb = new QProcess();
    QObject::connect(adb, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), [adb, callback, self=*this](int exitCode, QProcess::ExitStatus){    //We need to capture this by value because sometimes the this object will be deleted before the lambda is run. We use self=*this because just capturing *this gives weird compiler errors.
        //Check ADB exit status
        if(exitCode != 0){
            callback(QObject::tr("An error occurred when reading device %1: %2").arg(self.name(), adb->readAllStandardError().isEmpty() ? adb->readAllStandardOutput() : adb->readAllStandardError()));
            adb->deleteLater();
            return;
        }
        adb->deleteLater();

        //Use subst to connect the drive
        QProcess subst;
        subst.start("C:\\Windows\\System32\\subst.exe", {self.driveLetter() + QString(":"), self.temporaryDir().path() + "/sdcard"});
        subst.waitForFinished(-1);
        if(subst.exitCode() != 0){
            callback(QObject::tr("An error occurred when connecting drive for %1: %2").arg(self.name(), subst.readAllStandardError()));
            return;
        }

        //Watch the file system
        if(!self._fileSystemWatchers.contains(self.id())){
            //Find all the files on the drive
            QDirIterator iterator(self.temporaryDir().path() + "/sdcard", QDirIterator::Subdirectories);
            QStringList filesToWatch;
            while(iterator.hasNext()){
                filesToWatch.append(iterator.next());
            }

            //Watch the files on the local file system
            QFileSystemWatcher *fileSystemWatcher = new QFileSystemWatcher(filesToWatch);
            self._fileSystemWatchers.insert(self.id(), fileSystemWatcher);

            QObject::connect(fileSystemWatcher, &QFileSystemWatcher::directoryChanged, [self, fileSystemWatcher](const QString &path){
                const QString androidPath = QString(path).replace(QRegularExpression("^" + QRegularExpression::escape(self.temporaryDir().path())), "");

                //As a safety measure disconnect the drive if the entire temporary folder was deleted, otherwise clearing temporary files while this program is running would wipe the entire Android device
                if(!QDir(self.temporaryDir().path() + "/sdcard").exists()){
                    self.disconnectDrive();
                    return;
                }

                //If the file or folder was deleted from the drive, delete it from the Android device as well
                else if(!QFileInfo::exists(path)){
                    QProcess::startDetached("adb.exe", {"-s", self.id(), "shell", "rm", "-rf", "'" + androidPath + "'"});
                }

                //If the file or folder still exists, copy the new version to the Android device
                else{
                    QProcess::startDetached("adb.exe", {"-s", self.id(), "push", "--sync", path, QFileInfo(path).isDir() ? QFileInfo(androidPath).dir().path() : androidPath});

                    //If it's a folder, check for new files
                    if(QFileInfo(path).isDir()){
                        QStringList createFolderCommand = {"-s", self.id(), "shell", "mkdir", "-p"};
                        QDirIterator iterator(path);
                        while(iterator.hasNext()){
                            const QString file = iterator.next();

                            //If a new folder was created, create it on the Android device because ADB doesn't do this automatically (if it already exists, this command will do nothing thanks to -p)
                            if(QFileInfo(file).isDir()){
                                createFolderCommand.append("'" + androidPath + "/" + QFileInfo(file).baseName() + "'");
                            }

                            //Add the new file to the file system watcher (if it already existed, this won't do anything)
                            fileSystemWatcher->addPath(file);
                        }
                        QProcess::startDetached("adb.exe", createFolderCommand);
                    }
                }
            });

            QObject::connect(fileSystemWatcher, &QFileSystemWatcher::fileChanged, fileSystemWatcher, &QFileSystemWatcher::directoryChanged);
        }

        //Check for changes on the Android device
        if(!self._pullTimers.contains(self.id())){
            QTimer *pullTimer = new QTimer();
            self._pullTimers.insert(self.id(), pullTimer);
            QObject::connect(pullTimer, &QTimer::timeout, [self, pullTimer](){
                if(!AndroidDevice::allDevices().contains(self)){
                    self.disconnectDrive();
                }
                else{
                    //See http://www.temblast.com/adbsync.htm for how to use adbsync.exe
                    QProcess *adbSync = new QProcess;
                    adbSync->start("adbsync.exe", {"/d" + self.id(), "/hscu", "/s", self.temporaryDir().path() + "/sdcard", self.storageLocation().replace(QRegularExpression("^/"), "")});
                    pullTimer->stop();
                    const auto restartTimer = [self, adbSync](){
                        QTimer *pullTimer = self._pullTimers.value(self.id(), nullptr);
                        if(pullTimer != nullptr){
                            pullTimer->start();
                        }
                        delete adbSync;
                    };
                    QObject::connect(adbSync, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), restartTimer);
                    QObject::connect(adbSync, &QProcess::errorOccurred, restartTimer);
                }
            });

            pullTimer->setInterval(5000);
            pullTimer->start();
        }

        QSettings("Gustav Lindberg", "AndroidDrive").setValue(self.id() + "_connected", true);
        callback("");

        if(QSettings("Gustav Lindberg", "AndroidDrive").value("openInExplorer", true).toBool()){
            QProcess::startDetached("C:\\Windows\\explorer.exe", {self.driveLetter() + QString(":\\")});
        }
    });

    QObject::connect(adb, &QProcess::errorOccurred, [self=*this, adb, callback](QProcess::ProcessError error){
        adb->deleteLater();
        switch(error){
        case QProcess::FailedToStart:
            callback(QObject::tr("An error occurred when reading device %1: Could not find file adb.exe.").arg(self.name()));
            break;
        case QProcess::Crashed:
            callback(QObject::tr("An error occurred when reading device %1: Process adb.exe crashed.").arg(self.name()));
            break;
        case QProcess::Timedout:
            callback(QObject::tr("An error occurred when reading device %1: Process adb.exe did not respond.").arg(self.name()));
            break;
        default:
            callback(QObject::tr("An unknown error occurred when reading device %1.").arg(self.name()));
            break;
        }
    });

    adb->start("adb.exe", {"-s", this->id(), "pull", "-a", "/sdcard/", this->temporaryDir().path()});
}

void AndroidDevice::autoconnectDrive(){
    if(QSettings("Gustav Lindberg", "AndroidDrive").value(this->id() + "_connected", true).toBool() && !deviceListWindow->deviceIsConnecting() && !this->isConnected()){
        deviceListWindow->setDeviceIsConnecting(true);
        this->connectDrive([](const QString &errorMessage){
            if(!errorMessage.isEmpty()){
                QMessageBox::critical(nullptr, "", errorMessage);
            }
            deviceListWindow->setDeviceIsConnecting(false);
        });
    }
}

void AndroidDevice::disconnectDrive(bool disconnectInSettings) const{
    QFileSystemWatcher *fileSystemWatcher = this->_fileSystemWatchers.value(this->id(), nullptr);
    if(fileSystemWatcher != nullptr){
        delete fileSystemWatcher;
    }
    this->_fileSystemWatchers.remove(this->id());

    QTimer *pullTimer = this->_pullTimers.value(this->id(), nullptr);
    if(pullTimer != nullptr){
        delete pullTimer;
    }
    this->_pullTimers.remove(this->id());

    if(this->isConnected()){
        QProcess::startDetached("C:\\Windows\\System32\\subst.exe", {this->driveLetter() + QString(":"), "/d"});
        this->temporaryDir().removeRecursively();
    }

    if(disconnectInSettings){
        QSettings("Gustav Lindberg", "AndroidDrive").setValue(this->id() + "_connected", false);
    }
}

bool AndroidDevice::isConnected() const{
    QProcess subst;
    subst.start("C:\\Windows\\System32\\subst.exe", QStringList());
    subst.waitForFinished(-1);
    const QByteArray output = subst.readAllStandardOutput();
    QRegularExpressionMatch match = QRegularExpression(QString("(?:^|[\r\n])") + this->_driveLetter + QString(":\\\\:\\s=>\\s([^\r\n]+)(?:$|[\r\n])")).match(output);
    return match.hasMatch() && match.captured(1) == QDir::toNativeSeparators(this->temporaryDir().path() + "/sdcard");
}

QDir AndroidDevice::temporaryDir() const{
    return this->_temp.path() + "/" + this->id();
}

void AndroidDevice::checkDriveLetter(){
    if(QDir(this->driveLetter() + QString(":")).exists() && !this->isConnected()){
        for(char letter = 'D'; letter <= 'Z'; letter++){
            if(!QDir(letter + QString(":")).exists()){
                this->_driveLetter = letter;    //Don't use the setter here because (1) we only want to save the drive letter to the settings when the user selects a drive letter manually and (2) it would cause infinite recursion
                break;
            }
        }
    }
}

QList<AndroidDevice> AndroidDevice::allDevices(){
    QProcess adb;
    adb.start("adb.exe", {"devices"});
    adb.waitForFinished(-1);
    if(adb.exitStatus() != QProcess::NormalExit){
        return QList<AndroidDevice>();
    }
    const QStringList &allDeviceStrings = QString::fromUtf8(adb.readAllStandardOutput()).split(QRegularExpression("[^\\S\r\n]*[\r\n]+[^\\S\r\n]*"));
    QList<AndroidDevice> toReturn;
    for(const QString &deviceString: allDeviceStrings){
        const QRegularExpressionMatch &match = QRegularExpression("^([0-9A-Za-z]+)\t").match(deviceString);
        if(match.hasMatch()){
            toReturn.append(AndroidDevice(match.captured(1)));
        }
    }
    return toReturn;
}
