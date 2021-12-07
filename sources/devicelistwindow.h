#ifndef DEVICELISTWINDOW_H
#define DEVICELISTWINDOW_H

#include <QtWidgets>
#include "androiddevice.h"

class DeviceListWindow : public QDialog{
    Q_OBJECT

public:
    DeviceListWindow(QWidget *parent = nullptr);

    AndroidDevice *selectedDevice();
    void selectDriveLetter(AndroidDevice *device);

    void setDeviceIsConnecting(bool connecting);
    bool deviceIsConnecting() const;

public slots:
    void updateDevices();

private:
    QStringListModel _model;
    QList<AndroidDevice> _devices;

    QListView _view;
    QGridLayout _layout;

    QPushButton _connectButton, _driveLetterButton, _refreshButton;
    QCheckBox _openInExplorer;
};

#endif // DEVICELISTWINDOW_H
