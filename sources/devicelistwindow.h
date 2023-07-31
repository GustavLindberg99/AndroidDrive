#ifndef DEVICELISTWINDOW_H
#define DEVICELISTWINDOW_H

#include <QDialog>
#include <QGridLayout>
#include <QListView>
#include <QProcess>
#include <QPushButton>
#include <QStringListModel>
#include "androiddevice.h"
#include "settingswindow.h"

class DeviceListWindow : public QDialog{
    Q_OBJECT

public:
    DeviceListWindow();
    virtual ~DeviceListWindow();

    AndroidDevice *selectedDevice();

signals:
    void encounteredFatalError();

private slots:
    void updateButtons();
    void updateDevices(int exitCode, QProcess::ExitStatus exitStatus);
    void handleDokanError(AndroidDevice *device, int status);
    void handleAdbError(QProcess::ProcessError error);

private:
    QProcess _adb;
    bool _adbFailed;
    bool _dokanInstalling;

    QMap<QString, AndroidDevice*> _devices;
    QMap<AndroidDevice*, SettingsWindow*> _settingsWindows;

    QStringListModel _model;
    QListView _view;
    QGridLayout _layout;

    QPushButton _connectButton, _settingsButton;
};

#endif // DEVICELISTWINDOW_H
