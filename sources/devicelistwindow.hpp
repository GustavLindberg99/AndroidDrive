#ifndef DEVICELISTWINDOW_H
#define DEVICELISTWINDOW_H

#include <QDialog>
#include <QProcess>
#include <QPushButton>
#include <QTreeView>

#include "androiddevice.hpp"
#include "androiddrive.hpp"
#include "devicelistmodel.hpp"

class DeviceListWindow : public QDialog {
    Q_OBJECT

public:
    /**
     * Constructs a device list window without opening it.
     */
    DeviceListWindow();

    /**
     * Destructor.
     */
    virtual ~DeviceListWindow();

    /**
     * Gets the selected device.
     *
     * @return A non-owning pointer to the selected device, or nullptr if no device is selected.
     */
    AndroidDevice *selectedDevice() const;

    /**
     * Gets the selected drive.
     *
     * @return A non-owning pointer to the selected drive, or nullptr if no device is selected.
     */
    AndroidDrive *selectedDrive() const;

signals:
    /**
     * Emitted when a fatal error is encountered and the program needs to exit.
     */
    void encounteredFatalError();

private slots:
    /**
     * Checks the state of the drives and updates the text and disabled status of the buttons accordingly.
     */
    void updateButtons();

    /**
     * Updates which devices are connected according to ADB.
     *
     * @param exitCode - ADB's exit code.
     * @param exitStatus - ADB's exit status.
     */
    void updateDevices(int exitCode, QProcess::ExitStatus exitStatus);

    /**
     * Handles errors that occur when DokanMain exits with a failure code. Can also be called when Dokan exits successfully, but does nothing in that case.
     *
     * @param drive - The drive that the error occurred on.
     * @param status - The status code returned by Dokan.
     */
    void handleDokanError(AndroidDrive *drive, int status);

    /**
     * Handles errors that occur when the ADB process fails.
     *
     * @param error - The error that ADB failed with.
     */
    void handleAdbError(QProcess::ProcessError error);

private:
    QProcess _adb;
    bool _adbFailed = false;
    bool _dokanInstalling = false;

    DeviceListModel _model;

    QTreeView *const _view = new QTreeView(this);

    QPushButton *const _mountButton = new QPushButton(QObject::tr("&Mount drive"), this);
    QPushButton *const _settingsButton = new QPushButton(QObject::tr("Drive &settings"), this);
    QPushButton *const _openInExplorerButton = new QPushButton(QObject::tr("&Open in Explorer"), this);
};

#endif // DEVICELISTWINDOW_H
