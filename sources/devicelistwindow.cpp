#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>
#include "devicelistwindow.h"

DeviceListWindow::DeviceListWindow():
    _adbFailed(false),
    _dokanInstalling(false),
    _connectButton(QObject::tr("&Connect drive")),
    _settingsButton(QObject::tr("Device &settings"))
{
    this->_connectButton.setWhatsThis(QObject::tr("Connects a drive containing the internal storage of the selected Android device."));
    this->_settingsButton.setWhatsThis(QObject::tr("Allows you to change the settings for this device, for example select a new drive letter or choose whether it should connect automatically."));

    this->setWindowTitle(QObject::tr("AndroidDrive - Devices"));
    this->setWindowIcon(QIcon(":/icon.ico"));
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, true);

    this->_view.setModel(&this->_model);
    this->_view.setEditTriggers(QListView::NoEditTriggers);
    this->setLayout(&this->_layout);

    this->_layout.addWidget(&this->_view, 0, 0, 1, 2);
    this->_layout.addWidget(&this->_connectButton, 1, 0);
    this->_layout.addWidget(&this->_settingsButton, 1, 1);

    QObject::connect(&this->_connectButton, &QPushButton::pressed, this, [this](){
        AndroidDevice *device = this->selectedDevice();
        if(device != nullptr){
            if(device->isConnected()){
                device->disconnectDrive();
            }
            else{
                device->connectDrive(Settings().driveLetter(device));
            }
            this->updateButtons();
        }
    });

    QObject::connect(&this->_settingsButton, &QPushButton::pressed, this, [this](){
        AndroidDevice *device = this->selectedDevice();
        if(device != nullptr){
            this->_settingsWindows[device]->show();
        }
    });

    QObject::connect(&this->_view, &QListView::clicked, this, [this](){
        this->updateButtons();
    });

    QObject::connect(&this->_adb, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &DeviceListWindow::updateDevices);
    QObject::connect(&this->_adb, &QProcess::errorOccurred, this, &DeviceListWindow::handleAdbError);
    this->_adb.start("adb.exe", {"devices"});

    this->updateButtons();
}

DeviceListWindow::~DeviceListWindow(){
    this->_adb.disconnect();
    this->_adb.close();
}

AndroidDevice *DeviceListWindow::selectedDevice(){
    const int index = this->_view.currentIndex().row();
    if(index < 0 || index >= this->_devices.size()){
        return nullptr;
    }
    return this->_devices.values()[index];
}

void DeviceListWindow::updateButtons(){
    const AndroidDevice *device = this->selectedDevice();
    this->_connectButton.setEnabled(device != nullptr);
    this->_settingsButton.setEnabled(device != nullptr);
    if(device != nullptr && device->isConnected()){
        this->_connectButton.setText(QObject::tr("&Disconnect drive"));
        this->_connectButton.setWhatsThis(QObject::tr("Disconnects the drive corresponding to the selected Android device.<br/><br/>This only disconnects the drive, the Android device itself will remain connected, so you will still be able to access it for example through ADB."));
    }
    else{
        this->_connectButton.setText(QObject::tr("&Connect drive"));
        this->_connectButton.setWhatsThis(QObject::tr("Connects a drive containing the internal storage of the selected Android device."));
    }
}

void DeviceListWindow::updateDevices(int exitCode, QProcess::ExitStatus){
    //Check that it exited correctly
    if(exitCode != 0){
        if(this->_adbFailed){
            QMessageBox::critical(nullptr, "", QObject::tr("Fatal error: Could not list Android devices.<br/><br/>ADB exited with code %1.").arg(exitCode));
            emit this->encounteredFatalError();
        }
        else{
            this->_adbFailed = true;
            this->_adb.start("adb.exe", {"devices"});
        }
        return;
    }
    this->_adbFailed = false;

    //Create newly conneced devices
    static const QRegularExpression newlineRegex("[\r\n]+");
    const QStringList result = QString::fromUtf8(this->_adb.readAllStandardOutput()).trimmed().split(newlineRegex);
    QStringList serialNumbers;
    bool changed = false;
    static const QRegularExpression spaceRegex("\\s+");
    for(const QString &line: result){
        if(line == "List of devices attached" || line.isEmpty()){
            continue;
        }
        const QString serialNumber = line.split(spaceRegex)[0];
        if(!this->_devices.contains(serialNumber)){
            AndroidDevice *device = new AndroidDevice(serialNumber);
            Settings settings;
            if(settings.autoConnect(device)){
                device->connectDrive(settings.driveLetter(device));
            }
            QObject::connect(device, &AndroidDevice::driveConnected, this, &DeviceListWindow::updateButtons);
            QObject::connect(device, &AndroidDevice::driveDisconnected, this, &DeviceListWindow::updateButtons);
            QObject::connect(device, &AndroidDevice::driveDisconnected, this, [this, serialNumber](int status){
                AndroidDevice *device = this->_devices.value(serialNumber, nullptr);
                if(device != nullptr){
                    this->handleDokanError(device, status);
                }
            });
            QObject::connect(device, &AndroidDevice::driveMounted, [](char driveLetter){
                if(Settings().openInExplorer()){
                    QProcess::startDetached("C:\\Windows\\explorer.exe", {driveLetter + QString(":\\")});
                }
            });
            this->_devices[serialNumber] = device;
            this->_settingsWindows[device] = new SettingsWindow(device);
            changed = true;
        }
        serialNumbers.append(serialNumber);
    }

    //Delete disconnected devices
    const QList<AndroidDevice*> oldDevices = this->_devices.values();
    for(AndroidDevice *device: oldDevices){
        if(!serialNumbers.contains(device->serialNumber())){
            delete this->_settingsWindows[device];
            this->_settingsWindows.remove(device);
            this->_devices.remove(device->serialNumber());
            device->shutdown();
            this->_dokanInstalling = false;
            changed = true;
        }
    }

    //Update the model
    if(changed){
        QStringList models;
        const QList<AndroidDevice*> devices = this->_devices.values();
        for(const AndroidDevice *device: devices){
            models.append(device->model());
        }
        this->_model.setStringList(models);
        this->updateButtons();
    }

    //Start ADB again to continue updating
    this->_adb.start("adb.exe", {"devices"});
}

void DeviceListWindow::handleDokanError(AndroidDevice *device, int status){
    if(status == DOKAN_SUCCESS){
        return;
    }
    QString errorMessage;
    QMessageBox::StandardButtons buttons = QMessageBox::Ok;
    switch(status){
    case DOKAN_DRIVE_LETTER_ERROR:
        errorMessage = QObject::tr("Could not create a drive with the given drive letter.");
        break;
    case DOKAN_DRIVER_INSTALL_ERROR:
        if(this->_dokanInstalling){
            device->connectDrive(Settings().driveLetter(device));    //Try connecting it again in case Dokan is finished installing
            return;
        }
        errorMessage = QObject::tr("Dokan doesn't seem to be installed. Would you like to install it now?");
        buttons = QMessageBox::Yes | QMessageBox::No;
        break;
    case DOKAN_START_ERROR:
        errorMessage = QObject::tr("Could not start the driver.");
        break;
    case DOKAN_MOUNT_ERROR:
    case DOKAN_MOUNT_POINT_ERROR:
        errorMessage = QObject::tr("Could not assign a drive letter.<br/><br/>Try changing the drive letter in Device Settings to an available drive letter.");
        break;
    case DOKAN_VERSION_ERROR:
        errorMessage = QObject::tr("Dokan version error.");
        break;
    default:
        errorMessage = QObject::tr("An unknown error occurred.");
        break;
    }
    if(QMessageBox::critical(nullptr, "", QObject::tr("Could not connect device %1: %2").arg(device->model(), errorMessage), buttons) == QMessageBox::Yes){
        QDesktopServices::openUrl(QUrl("https://github.com/dokan-dev/dokany/releases/download/v2.0.6.1000/DokanSetup.exe"));
        this->_dokanInstalling = true;
    }
};

void DeviceListWindow::handleAdbError(QProcess::ProcessError error){
    if(!this->_adbFailed){    //If it only fails once in a while, don't bother the user with it. But if it fails twice in a row, there's probably something wrong, in which case we show an error message and exit.
        this->_adbFailed = true;
        this->_adb.start("adb.exe", {"devices"});
        return;
    }

    QString errorMessage;
    switch(error){
    case QProcess::Timedout:
        errorMessage = QObject::tr("ADB timed out.");
        break;
    case QProcess::ReadError:
        errorMessage = QObject::tr("An error occurred when attempting to read from the ADB process.");
        break;
    case QProcess::WriteError:
        errorMessage = QObject::tr("An error occurred when attempting to write to the ADB process.");
        break;
    case QProcess::FailedToStart:
        errorMessage = QObject::tr("ADB failed to start. Either the adb.exe file is missing, or you may have insufficient permissions to invoke the program.");
        break;
    case QProcess::Crashed:
        errorMessage = QObject::tr("ADB crashed.");
        break;
    default:
        errorMessage = QObject::tr("ADB encountered an unknown error.");
        break;
    }
    QMessageBox::critical(nullptr, "", QObject::tr("Fatal error: Could not list Android devices: %1").arg(errorMessage));
    emit this->encounteredFatalError();
}
