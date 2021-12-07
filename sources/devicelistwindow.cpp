#include "devicelistwindow.h"

DeviceListWindow::DeviceListWindow(QWidget *parent):
    QDialog(parent),
    _connectButton(QObject::tr("&Connect drive")),
    _driveLetterButton(QObject::tr("Select drive &letter")),
    _refreshButton(QObject::tr("&Refresh list")),
    _openInExplorer(QObject::tr("Open newly connected drives in Explorer"))
{
    this->_connectButton.setWhatsThis(QObject::tr("Connects a drive containing the internal storage of the selected Android device."));
    this->_driveLetterButton.setWhatsThis(QObject::tr("Allows you to select a new drive letter for the selected Android device. If you change the drive letter while a drive for this device is connected, it will be disconnected and then reconnected with the new drive letter."));
    this->_refreshButton.setWhatsThis(QObject::tr("Refreshes the list of available Android devices."));
    this->_openInExplorer.setWhatsThis(QObject::tr("If this checkbox is checked, whenever AndroidDrive is finished connecting a drive, it will open that drive in Windows Explorer."));
    this->_openInExplorer.setChecked(QSettings("Gustav Lindberg", "AndroidDrive").value("openInExplorer", true).toBool());

    this->setWindowTitle(QObject::tr("Devices"));
    this->setWindowIcon(QIcon(":/icon.ico"));

    this->_view.setModel(&this->_model);
    this->_view.setEditTriggers(QListView::NoEditTriggers);
    this->setLayout(&this->_layout);

    this->_layout.addWidget(&this->_view, 0, 0, 1, 3);
    this->_layout.addWidget(&this->_connectButton, 1, 0);
    this->_layout.addWidget(&this->_driveLetterButton, 1, 1);
    this->_layout.addWidget(&this->_refreshButton, 1, 2);
    this->_layout.addWidget(&this->_openInExplorer, 2, 0, 1, 3);

    QObject::connect(&this->_connectButton, &QPushButton::pressed, [this](){
        if(this->selectedDevice()->isConnected()){
            this->selectedDevice()->disconnectDrive(true);
            this->_connectButton.setText(QObject::tr("&Connect drive"));
            this->_connectButton.setWhatsThis(QObject::tr("Connects a drive containing the internal storage of the selected Android device."));
            this->_driveLetterButton.setDisabled(false);
        }
        else{
            this->setDeviceIsConnecting(true);
            this->selectedDevice()->connectDrive([this](const QString &errorMessage){
                if(!errorMessage.isEmpty()){
                    QMessageBox::critical(this, "", errorMessage);
                    //Don't return here, we still need to enable the buttons
                }
                this->setDeviceIsConnecting(false);
            });
        }
    });

    QObject::connect(&this->_driveLetterButton, &QPushButton::pressed, [this](){
        this->selectDriveLetter(this->selectedDevice());
    });

    QObject::connect(&this->_refreshButton, &QPushButton::pressed, this, &DeviceListWindow::updateDevices);

    QObject::connect(&this->_openInExplorer, &QCheckBox::clicked, [](bool checked){
        QSettings("Gustav Lindberg", "AndroidDrive").setValue("openInExplorer", checked);
    });

    QObject::connect(&this->_view, &QListView::clicked, [this](){
        this->setDeviceIsConnecting(this->deviceIsConnecting());
    });

    this->updateDevices();
}

AndroidDevice *DeviceListWindow::selectedDevice(){
    const int index = this->_view.currentIndex().row();
    if(index < 0 || index >= this->_devices.size()){
        return nullptr;
    }
    return &this->_devices[index];
}

void DeviceListWindow::selectDriveLetter(AndroidDevice *device){
    QDialog dialog(this);
    dialog.setWindowTitle(QObject::tr("Select a drive letter for %1").arg(device->name()));

    QHBoxLayout layout;
    QComboBox driveLetterList;
    QList<char> driveLetters;
    for(char letter = 'A'; letter <= 'Z'; letter++){
        if(!QDir(letter + QString(":")).exists() || letter == device->driveLetter()){
            driveLetterList.addItem(letter + QString(":"));
            driveLetters.append(letter);
            if(letter == device->driveLetter()){
                driveLetterList.setCurrentIndex(driveLetterList.count() - 1);
            }
        }
    }
    layout.addWidget(&driveLetterList);

    QPushButton okButton(QObject::tr("OK"));
    QObject::connect(&okButton, &QPushButton::pressed, [&](){
        device->setDriveLetter(driveLetters[driveLetterList.currentIndex()]);
        dialog.close();
    });
    layout.addWidget(&okButton);

    QPushButton cancelButton(QObject::tr("Cancel"));
    QObject::connect(&cancelButton, &QPushButton::pressed, &dialog, &QDialog::close);
    layout.addWidget(&cancelButton);
    dialog.setLayout(&layout);

    dialog.exec();
}

void DeviceListWindow::updateDevices(){
    QStringList deviceNames;
    this->_devices = AndroidDevice::allDevices();
    for(const AndroidDevice &device: qAsConst(this->_devices)){
        deviceNames.append(device.name());
    }
    this->_model.setStringList(deviceNames);

    this->_connectButton.setDisabled(true);
    this->_driveLetterButton.setDisabled(true);
}

void DeviceListWindow::setDeviceIsConnecting(bool connecting){
    if(connecting){
        this->_connectButton.setText(QObject::tr("Connecting..."));
        this->_connectButton.setDisabled(true);
        this->_driveLetterButton.setDisabled(true);
        this->_refreshButton.setDisabled(true);
    }
    else{
        const AndroidDevice *selectedDevice = this->selectedDevice();
        const bool connected = selectedDevice != nullptr && selectedDevice->isConnected();
        this->_connectButton.setText(connected ? QObject::tr("&Disconnect drive") : QObject::tr("&Connect drive"));
        this->_connectButton.setWhatsThis(connected ? QObject::tr("Disconnects the drive corresponding to the selected Android device. This only disconnects the drive, the Android device itself will remain connected, so you will still be able to access it for example through ADB. If you disconnect a drive manually, it won't be reconnected until you reconnect it manually.") : QObject::tr("Connects a drive containing the internal storage of the selected Android device."));
        this->_connectButton.setDisabled(selectedDevice == nullptr);
        this->_driveLetterButton.setDisabled(selectedDevice == nullptr);
        this->_refreshButton.setDisabled(false);
    }
}

bool DeviceListWindow::deviceIsConnecting() const{
    return this->_connectButton.text() == QObject::tr("Connecting...");
}
