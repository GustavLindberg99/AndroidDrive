#ifndef ANDROIDDEVICE_H
#define ANDROIDDEVICE_H

#include <QList>
#include <QMap>
#include <QString>
#include <QTemporaryDir>
#include <QThread>

#include <dokan/dokan.h>
#include <memory>

class AndroidDrive;

class AndroidDevice: public QObject, public std::enable_shared_from_this<AndroidDevice> {
    Q_OBJECT

public:
    /**
     * Constructs a device object.
     *
     * @param serialNumber - The serial number of the device.
     */
    AndroidDevice(const QString &serialNumber);

    /**
     * Destructor.
     */
    virtual ~AndroidDevice();

    /**
     * Quits the application as soon as the last device is deleted. If there are no devices, quits immediately.
     */
    static void quitOnLastDeletedDevice();

    /**
     * Disallow copying.
     */
    AndroidDevice(const AndroidDevice&) = delete;
    void operator=(const AndroidDevice&) = delete;

    /**
     * Connects all the device's drives.
     */
    void connectAllDrives();

    /**
     * Connects the device's drives that are supposed to be automatically connected according to the settings. Does nothing with the others.
     */
    void autoconnectAllDrives();

    /**
     * Disconnects all the device's drives.
     */
    void disconnectAllDrives();

    /**
     * @return The number of drives that the device has.
     */
    int numberOfDrives() const;

    /**
     * @return The number of the device's drives that are currently connected.
     */
    int numberOfConnectedDrives() const;

    /**
     * Checks if this device is the parent of the given drive.
     *
     * @param drive - The drive to check for.
     *
     * @return True if the drive's parent is this device, false otherwise.
     */
    bool isParentOfDrive(const AndroidDrive *drive) const;

    /**
     * Gets the drive at the given index.
     *
     * @param index - The index to get the drive at. 0 for the device's first drive, 1 for the device's second drive, etc.
     *
     * @return A non-owning pointer to the drive at the given index, or nullptr if the index is out of range.
     */
    AndroidDrive *driveAt(int index) const;

    /**
     * Runs a bash command through ADB on the given device.
     *
     * @param command - The bash command to run.
     * @param ok - Will be set to true if the command exits correctly and false otherwise.
     * @param useCache - If true and the result of the command has been cached less than a second ago, returns the cached value and doesn't run anything through ADB. If true but no recent cache exists, caches the result only if the command is successful.
     *
     * @return The standard output of the command, excluding leading and trailing whitespace.
     */
    QString runAdbCommand(const QString &command, bool *ok = nullptr, bool useCache = true) const;

    /**
     * Copies a file from Android to Windows.
     *
     * @param remoteFile - The path of the Android file to download.
     * @param localFile - The path of the Windows file to create. If the file already exists, overwrites it.
     *
     * @return True if the file was successfully copied, false otherwise.
     */
    bool pullFromAdb(const QString &remoteFile, const QString &localFile) const;

    /**
     * Copies a file from Windows to Android.
     *
     * @param localFile - The path of the Windows file to upload.
     * @param remoteFile - The path of the Android file to create. If the file already exists, overwrites it.
     *
     * @return True if the file was successfully copied, false otherwise.
     */
    bool pushToAdb(const QString &localFile, const QString &remoteFile) const;

    /**
     * @return The model of the device.
     */
    QString model() const;

    /**
     * @return The serial number of the device.
     */
    QString serialNumber() const;

signals:
    /**
     * Emitted when the Dokan main loop starts.
     *
     * @param drive - The drive that's connected.
     */
    void driveConnected(AndroidDrive *drive);

    /**
     * Emitted when the drive appears in the This PC folder.
     *
     * @param drive - The drive that's connected.
     * @param driveLetter - The drive letter that the drive is connected as.
     */
    void driveMounted(AndroidDrive *drive, char driveLetter);

    /**
     * Emitted when the drive is removed from the This PC folder.
     *
     * @param drive - The drive that's connected.
     */
    void driveUnmounted(AndroidDrive *drive);

    /**
     * Emitted when the Dokan main loop finishes.
     *
     * @param drive - The drive that's connected.
     * @param status - The status code returned by Dokan.
     */
    void driveDisconnected(AndroidDrive *drive, int status);

private:
    /**
     * Adds a drive to the device.
     *
     * @param androidRootPath - The root path of the drive to add.
     */
    void addDrive(QString androidRootPath);

    static QList<AndroidDevice*> _instances;
    static bool _quitOnLastDeletedDevice;
    bool _willBeDeleted = false;

    const QString _serialNumber;

    std::vector<std::unique_ptr<AndroidDrive>> _drives;

    //Cache for specific methods (it would be easier to declare these as static in their respective methods, but that would use the same cache accross instances, see https://stackoverflow.com/a/6223371/4284627)
    //These are mutable because the cache can be updated in const methods that don't really change the state of the object
    mutable QMap<QString, QPair<QString, qint64>> _adbCache;
    mutable QString _cachedModel;
};

#endif // ANDROIDDEVICE_H
