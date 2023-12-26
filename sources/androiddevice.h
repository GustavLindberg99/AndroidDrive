#ifndef ANDROIDDEVICE_H
#define ANDROIDDEVICE_H

#include <QList>
#include <QMap>
#include <QString>
#include <QTemporaryDir>
#include <QThread>
#include <functional>
#include <dokan/dokan.h>

class AndroidDrive;

class AndroidDevice: public QObject{
    Q_OBJECT

public:
    AndroidDevice(const QString &serialNumber);
    void shutdown();
    static void shutdownAllDevices(const std::function<void()> &callback);

    AndroidDevice(const AndroidDevice&) = delete;
    void operator=(const AndroidDevice&) = delete;

    void disconnectAllDrives();

    int numberOfDrives() const;
    int numberOfConnectedDrives() const;
    QList<AndroidDrive*> drives() const;

    QString runAdbCommand(const QString &command, bool *ok = nullptr, bool useCache = true) const;
    bool pullFromAdb(const QString &remoteFile, const QString &localFile) const;
    bool pushToAdb(const QString &localFile, const QString &remoteFile) const;

    QString model() const;
    QString serialNumber() const;

signals:
    void driveConnected(AndroidDrive *drive);                   //Emitted when the Dokan main loop starts
    void driveMounted(AndroidDrive *drive, char driveLetter);   //Emitted when the drive appears in the This PC folder
    void driveUnmounted(AndroidDrive *drive);                   //Emitted when the drive is removed from the This PC folder
    void driveDisconnected(AndroidDrive *drive, int status);    //Emitted when the Dokan main loop finishes

private:
    virtual ~AndroidDevice();    //Make the destructor private because the object should never be deleted directly, instead, shutdown() should be called. This is to give Dokan the time to shut down, otherwise invalid pointers will be passed to it.

    static QList<AndroidDevice*> _instances;
    static std::function<void()> _callbackOnLastShutdown;
    bool _willBeDeleted = false;

    const QString _serialNumber;

    QList<AndroidDrive*> _drives;

    //Cache for specific methods (it would be easier to declare these as static in their respective methods, but that would use the same cache accross instances, see https://stackoverflow.com/a/6223371/4284627)
    //These are mutable because the cache can be updated in const methods that don't really change the state of the object
    mutable QMap<QString, QPair<QString, qint64>> _adbCache;
    mutable QString _cachedModel;
};

#endif // ANDROIDDEVICE_H
