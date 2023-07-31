#ifndef ANDROIDDEVICE_H
#define ANDROIDDEVICE_H

#include <QList>
#include <QMap>
#include <QString>
#include <QThread>
#include <functional>
#include <dokan/dokan.h>

class AndroidDevice: public QObject{
    Q_OBJECT

public:
    AndroidDevice(const AndroidDevice&) = delete;
    void operator=(const AndroidDevice&) = delete;

    AndroidDevice(const QString &serialNumber);
    void shutdown();
    static void shutdownAllDevices(const std::function<void()> &callback);

    static AndroidDevice *fromDokanFileInfo(PDOKAN_FILE_INFO dokanFileInfo);

    void connectDrive(char driveLetter);
    void disconnectDrive();
    bool isConnected() const;

    QString runAdbCommand(const QString &command, bool *ok = nullptr, bool useCache = true) const;
    bool pullFromAdb(const QString &remoteFile, const QString &localFile) const;
    bool pushToAdb(const QString &localFile, const QString &remoteFile) const;

    QString model() const;
    QString fileSystem() const;
    QString serialNumber() const;

signals:
    void driveConnected();                 //Emitted when the Dokan main loop starts
    void driveMounted(char driveLetter);   //Emitted when the drive appears in the This PC folder
    void driveUnmounted();                 //Emitted when the drive is removed from the This PC folder
    void driveDisconnected(int status);    //Emitted when the Dokan main loop finishes

private:
    virtual ~AndroidDevice();    //Make the destructor private because the object should never be deleted directly, instead, shutdown() should be called. This is to give Dokan the time to shut down, otherwise invalid pointers will be passed to it.

    static QList<AndroidDevice*> _instances;
    static std::function<void()> _callbackOnLastShutdown;
    bool _willBeDeleted;

    const QString _serialNumber;

    QThread *_thread;
    bool _mounted;
    bool _shouldBeDisconnected;
    bool _mountPointRemoved;

    DOKAN_OPERATIONS _dokanOperations;
    DOKAN_OPTIONS _dokanOptions;
    wchar_t _mountPoint[4];

    //Cache for specific methods (it would be easier to declare these as static in their respective methods, but that would use the same cache accross instances, see https://stackoverflow.com/a/6223371/4284627)
    //These are mutable because the cache can be updated in const methods that don't really change the state of the object
    mutable QMap<QString, QPair<QString, qint64>> _adbCache;
    mutable QString _cachedModel;
    mutable QString _cachedFileSystem;
    mutable bool _fileSystemCached;
};

#endif // ANDROIDDEVICE_H
