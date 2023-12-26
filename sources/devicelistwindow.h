#ifndef DEVICELISTWINDOW_H
#define DEVICELISTWINDOW_H

#include <QDialog>
#include <QProcess>
#include <QPushButton>
#include <QTreeView>
#include "androiddevice.h"
#include "devicelistmodel.h"
#include "settingswindow.h"

class DeviceListWindow : public QDialog{
    Q_OBJECT

public:
    DeviceListWindow();
    virtual ~DeviceListWindow();

    AndroidDevice *selectedDevice() const;
    AndroidDrive *selectedDrive() const;

signals:
    void encounteredFatalError();

private slots:
    void updateButtons();
    void updateDevices(int exitCode, QProcess::ExitStatus exitStatus);
    void handleDokanError(AndroidDrive *drive, int status);
    void handleAdbError(QProcess::ProcessError error);

private:
    QProcess _adb;
    bool _adbFailed = false;
    bool _dokanInstalling = false;

    DeviceListModel _model;
    QMap<AndroidDrive*, SettingsWindow*> _settingsWindows;

    QTreeView *const _view = new QTreeView(this);

    QPushButton *const _mountButton = new QPushButton(QObject::tr("&Mount drive"), this);
    QPushButton *const _settingsButton = new QPushButton(QObject::tr("Drive &settings"), this);
    QPushButton *const _openInExplorerButton = new QPushButton(QObject::tr("&Open in Explorer"), this);
};

#endif // DEVICELISTWINDOW_H
