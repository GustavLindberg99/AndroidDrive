#ifndef ANDROIDDRIVE_H
#define ANDROIDDRIVE_H

#include <QObject>
#include <QString>
#include <QTemporaryDir>

#include <dokan/dokan.h>
#include <memory>
#include <mutex>
#include <thread>

#include "settingswindow.hpp"
#include "temporaryfile.hpp"

class AndroidDevice;

class AndroidDrive : public QObject {
    Q_OBJECT

public:
    /**
     * Constructs a drive object.
     *
     * @param androidRootPath - The path to folder on Android that should be used as the root of the drive.
     * @param model - The model of the device that the drive belongs to.
     * @param serialNumber - The serial number of the device that the drive belongs to.
     */
    explicit AndroidDrive(QString androidRootPath, QString model, QString serialNumber);

    /**
     * Destructor, shuts down Dokan if needed and doesn't return until Dokan is shut down.
     */
    virtual ~AndroidDrive();

    /**
     * Disallow copying.
     */
    AndroidDrive(const AndroidDrive&) = delete;
    void operator=(const AndroidDrive&) = delete;

    /**
     * Gets a drive from a PDOKAN_FILE_INFO object.
     *
     * @param dokanFileInfo - The Dokan file info to get the drive from.
     *
     * @return A non-owning pointer to the drive that the file info refers to.
     */
    static AndroidDrive *fromDokanFileInfo(PDOKAN_FILE_INFO dokanFileInfo);

    /**
     * Connects the drive.
     *
     * Can't just be called connect because QObject already has a connect method.
     *
     * @param driveLetter - The drive letter to use.
     * @param device - The device that the drive belongs to. The drive should co-own the device as long as it's mounted so that the device doens't get deleted until Dokan has had time to shut down.
     */
    void connectDrive(char driveLetter, const std::shared_ptr<AndroidDevice> &device);

    /**
     * Disconnects the drive.
     *
     * Can't just be called disconnect because QObject already has a connect method.
     */
    void disconnectDrive();

    /**
     * Checks if the drive is connected. A drive is considered connected even while it's mounting/unmounting.
     *
     * @return True if the drive is connected, false otherwise.
     */
    bool isConnected() const;

    /**
     * Checks if the drive is has received a request to connect but isn't mounted yet.
     *
     * @return True if mounting is in progress, false otherwise.
     */
    bool mountingInProgress() const;

    /**
     * Checks if the drive is has received a request to disconnect but isn't unmounted yet.
     *
     * @return True if unmounting is in progress, false otherwise.
     */
    bool unmountingInProgress() const;

    /**
     * Gets the device that the drive belongs to if the drive is connected.
     *
     * @return The device if the drive is connected, nullptr otherwise. To get the device even if the drive isn't connected, use DeviceListModel::parentDevice.
     */
    std::shared_ptr<AndroidDevice> device() const;

    /**
     * Checks whether this drive is the interal storage drive.
     *
     * @return True if it's the device's internal storage drive, false if it's an external SD card.
     */
    bool isInternalStorage() const;

    /**
     * Gets the name of the file system. Don't use on disconnected drives.
     *
     * @return The name of the file system.
     */
    QString fileSystem() const;

    /**
     * Gets the name as it shows up in AndroidDrive's list of devices (doesn't need as much context since the name of the device will be displayed above it).
     *
     * @return The name of the drive.
     */
    QString name() const;

    /**
     * Gets the name as it shows up in the This PC folder (needs more context since it will be displayed as its own drive).
     *
     * @return The complete name of the drive.
     */
    QString completeName() const;

    /**
     * A unique id for the drive, used internally to store settings related to it.
     *
     * @return The id of the drive.
     */
    QString id() const;

    /**
     * Gets the path to folder on Android that should be used as the root of the drive.
     *
     * @return The Android path to use as root.
     */
    QString androidRootPath() const;

    /**
     * Converts an Android path to a Windows path.
     *
     * @param remotePath - The Android path.
     *
     * @return The Windows path.
     */
    QString localPath(const QString &remotePath) const;

    /**
     * Converts a Windows path to an Android path.
     *
     * @param windowsPath - The Windows path.
     *
     * @return The Android path.
     */
    QString windowsPathToAndroidPath(LPCWSTR windowsPath) const;

    /**
     * Gets the mount point, i.e. the drive letter followed by ":\".
     *
     * @return The mount point.
     */
    QString mountPoint() const;

    /**
     * Downloads a file from the Android device to a local temporary file.
     *
     * @param dokanFileInfo - The Dokan file info object that the temporary file should be added to as Context.
     * @param remotePath - The path of the Android file to download.
     * @param creationDisposition - The creation disposition to use when creating the handle.
     * @param shareAccess - The share access to use when creating the handle.
     * @param desiredAccess - The desired access to use when creating the handle.
     * @param fileAttributes - The file attributes to use when creating the handle.
     * @param createOptions - The create options to use when creating the handle.
     * @param createDisposition - The create disposition to use when creating the handle.
     * @param exists - True if opening an existing file (in which case it should be downloaded), false when creating a new file (in which case it should just be created locally).
     * @param altStream - The alt stream to use when creating the handle.
     *
     * @return STATUS_SUCCESS on success, an error status on failure.
     */
    NTSTATUS addTemporaryFile(PDOKAN_FILE_INFO dokanFileInfo, const QString &remotePath, DWORD creationDisposition, ULONG shareAccess, ACCESS_MASK desiredAccess, ULONG fileAttributes, ULONG createOptions, ULONG createDisposition, bool exists, const QString &altStream);

    /**
     * Deletes the temporary file.
     *
     * @param dokanFileInfo - The Dokan file info that contains the temporary file as Context, the Context will be set to null.
     */
    void deleteTemporaryFile(PDOKAN_FILE_INFO dokanFileInfo);

    /**
     * Opens the drive's settings window.
     */
    void openSettingsWindow();

signals:
    /**
     * Emitted when the Dokan main loop starts.
     */
    void driveConnected();

    /**
     * Emitted when the drive appears in the This PC folder.
     *
     * @param driveLetter - The drive letter that the drive is connected as.
     */
    void driveMounted(char driveLetter);

    /**
     * Emitted when the drive is removed from the This PC folder.
     */
    void driveUnmounted();

    /**
     * Emitted when the Dokan main loop finishes.
     *
     * @param status - The status code returned by Dokan.
     */
    void driveDisconnected(int status);

private:
    static QList<AndroidDrive*> _instances;
    static DWORD _internalLogicalDrives;    //Bitmask similar to Windows API GetLogicalDrives() but which keeps track of which drives are used internally. Needed to avoid race conditions in case two drives are connected at the same time.

    DOKAN_OPERATIONS _dokanOperations;
    DOKAN_OPTIONS _dokanOptions;

    const QString _androidRootPath;
    const QString _model;
    const QString _serialNumber;
    QString _fileSystem;

    std::shared_ptr<AndroidDevice> _device = nullptr;
    std::thread _thread;
    std::mutex _mutex;
    std::unique_ptr<QTemporaryDir> _temporaryDir = nullptr;
    std::vector<std::unique_ptr<TemporaryFile>> _temporaryFiles;

    wchar_t _mountPoint[4] = L"?:\\";
    bool _mounted = false;
    bool _shouldBeDisconnected = true;

    SettingsWindow _settingsWindow;
};

template<>
struct std::formatter<const AndroidDrive*>: public std::formatter<std::string> {
    auto format(const AndroidDrive *drive, std::format_context &ctx) const {
        const auto device = drive->device();
        const std::string serialNumber = device == nullptr ? "nullptr" : device->serialNumber().toStdString();
        return std::formatter<std::string>::format(serialNumber + ":" + drive->androidRootPath().toStdString(), ctx);
    }
};

template<>
struct std::formatter<AndroidDrive*>: public std::formatter<const AndroidDrive*> {
    auto format(AndroidDrive *drive, std::format_context &ctx) const {
        return std::formatter<const AndroidDrive*>::format(drive, ctx);
    }
};

#endif // ANDROIDDRIVE_H
