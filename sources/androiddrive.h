#ifndef ANDROIDDRIVE_H
#define ANDROIDDRIVE_H

#include <QObject>
#include <QString>
#include <QTemporaryDir>

#include <dokan/dokan.h>

class AndroidDevice;

class AndroidDrive : public QObject{
    Q_OBJECT

public:
    AndroidDrive(AndroidDevice *device, const QString &androidRootPath);
    virtual ~AndroidDrive();

    AndroidDrive(const AndroidDrive&) = delete;
    void operator=(const AndroidDrive&) = delete;

    static AndroidDrive *fromDokanFileInfo(PDOKAN_FILE_INFO dokanFileInfo);

    //These can't be called just connect and disconnect because QObject already has methods called that
    void connectDrive(char driveLetter);
    void disconnectDrive();
    bool isConnected() const;

    AndroidDevice *device() const;
    QString fileSystem() const;
    QString name() const;           //Name as it shows up in AndroidDrive's list of devices (doesn't need as much context since the name of the device will be displayed above it)
    QString completeName() const;   //Name as it shows up in the This PC folder (needs more context since it will be displayed as its own drive)
    QString id() const;             //A unique id for the drive, used internally to store settings related to it

    QString androidRootPath() const;
    QString localPath(const QString &remotePath) const;
    QString windowsPathToAndroidPath(LPCWSTR windowsPath) const;
    QString mountPoint() const;

signals:
    void driveConnected();                 //Emitted when the Dokan main loop starts
    void driveMounted(char driveLetter);   //Emitted when the drive appears in the This PC folder
    void driveUnmounted();                 //Emitted when the drive is removed from the This PC folder
    void driveDisconnected(int status);    //Emitted when the Dokan main loop finishes

private:
    static QList<AndroidDrive*> _instances;
    static DWORD _internalLogicalDrives;    //Bitmask similar to Windows API GetLogicalDrives() but which keeps track of which drives are used internally. Needed to avoid race conditions in case two drives are connected at the same time.

    DOKAN_OPERATIONS _dokanOperations;
    DOKAN_OPTIONS _dokanOptions;
    wchar_t _mountPoint[4] = L"?:\\";
    QTemporaryDir *_temporaryDir = nullptr;
    QThread *_thread = nullptr;

    bool _mounted = false;
    bool _shouldBeDisconnected = false;
    bool _mountPointRemoved = false;

    AndroidDevice *const _device;
    const QString _androidRootPath;

    //Cache for specific methods (it would be easier to declare these as static in their respective methods, but that would use the same cache accross instances, see https://stackoverflow.com/a/6223371/4284627)
    //These are mutable because the cache can be updated in const methods that don't really change the state of the object
    mutable QString _cachedFileSystem;
    mutable bool _fileSystemCached = false;
};

#endif // ANDROIDDRIVE_H
