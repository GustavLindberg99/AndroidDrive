#include "devicelistwindow.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QGridLayout>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>

DeviceListWindow::DeviceListWindow(){
    //Initialize the UI
    QGridLayout *layout = new QGridLayout(this);

    this->_mountButton->setWhatsThis(QObject::tr("Mounts a drive containing the internal storage of the selected Android device, as well as a drive for each external SD card that the selected device has, if any."));
    this->_settingsButton->setWhatsThis(QObject::tr("Allows you to change the settings for this device, for example select a new drive letter or choose whether it should be mounted automatically."));
    this->_openInExplorerButton->setWhatsThis(QObject::tr("Opens the selected drive in Windows Explorer."));

    this->setWindowTitle(QObject::tr("AndroidDrive - Devices"));
    this->setWindowIcon(QIcon(":/icon.ico"));
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, true);
    this->setMinimumWidth(400);

    this->_view->setModel(&this->_model);
    this->_view->setEditTriggers(QTreeView::NoEditTriggers);
    this->_view->setColumnWidth(0, 200);

    layout->addWidget(this->_view, 0, 0, 1, 3);
    layout->addWidget(this->_mountButton, 1, 0);
    layout->addWidget(this->_settingsButton, 1, 1);
    layout->addWidget(this->_openInExplorerButton, 1, 2);

    this->setLayout(layout);

    //Handle the mount button pressed signal
    QObject::connect(this->_mountButton, &QPushButton::pressed, this, [this](){
        AndroidDevice *device = this->selectedDevice();
        if(device != nullptr){
            if(device->numberOfConnectedDrives() > 0){
                device->disconnectAllDrives();
            }
            else{
                device->connectAllDrives();
            }
            this->updateButtons();
        }
        else{
            AndroidDrive *drive = this->selectedDrive();
            if(drive != nullptr){
                if(drive->isConnected()){
                    drive->disconnectDrive();
                }
                else{
                    drive->connectDrive(Settings().driveLetter(drive), this->_model.parentDevice(drive));
                }
                this->updateButtons();
            }
        }
    });

    //Handle the settings button pressed signal
    QObject::connect(this->_settingsButton, &QPushButton::pressed, this, [this](){
        AndroidDrive *drive = this->selectedDrive();
        if(drive != nullptr){
            drive->openSettingsWindow();
        }
    });

    //Handle the open in explorer button pressed signal
    QObject::connect(this->_openInExplorerButton, &QPushButton::pressed, this, [this](){
        AndroidDrive *drive = this->selectedDrive();
        if(drive != nullptr){
            QProcess::startDetached("C:\\Windows\\explorer.exe", {drive->mountPoint()});
        }
    });

    //Update the buttons when the view is clicked, meaning that the user selected a new item
    QObject::connect(this->_view, &QTreeView::clicked, this, [this](){
        this->updateButtons();
    });

    //Handle when a new device is connected
    QObject::connect(&this->_model, &DeviceListModel::rowsInserted, this, [this](const QModelIndex &parent, int first, int last){
        //For some reason this is necessary for the view to update
        this->_view->hide();
        this->_view->show();

        //If the parent isn't the root, the child isn't a device, but this function only cares about when devices are connected
        if(parent != this->_model.rootIndex()){
            return;
        }

        const QList<std::shared_ptr<AndroidDevice>> devices = this->_model.devices();
        for(int i = first; i <= last; i++){
            const std::shared_ptr<AndroidDevice> device = devices[i];
            Settings settings;

            //Mount automatically if the setting for that is enabled
            device->autoconnectAllDrives();

            //Update the buttons whenever the drive is connected or disconnected
            QObject::connect(device.get(), &AndroidDevice::driveConnected, this, &DeviceListWindow::updateButtons);
            QObject::connect(device.get(), &AndroidDevice::driveMounted, this, &DeviceListWindow::updateButtons);
            QObject::connect(device.get(), &AndroidDevice::driveUnmounted, this, &DeviceListWindow::updateButtons);
            QObject::connect(device.get(), &AndroidDevice::driveDisconnected, this, &DeviceListWindow::updateButtons);

            //Handle errors
            QObject::connect(device.get(), &AndroidDevice::driveDisconnected, this, &DeviceListWindow::handleDokanError);

            //Open in Explorer if the setting for that is enabled
            QObject::connect(device.get(), &AndroidDevice::driveMounted, [](AndroidDrive*, char driveLetter){
                if(Settings().openInExplorer()){
                    QProcess::startDetached("C:\\Windows\\explorer.exe", {driveLetter + QString(":\\")});
                }
            });

            //Expand items by default
            this->_view->expand(this->_model.deviceToIndex(device));
        }
    });

    //Start ADB to see which devices there are
    QObject::connect(&this->_adb, &QProcess::finished, this, &DeviceListWindow::updateDevices);
    QObject::connect(&this->_adb, &QProcess::errorOccurred, this, &DeviceListWindow::handleAdbError);
    this->_adb.start("adb.exe", {"devices"});

    this->updateButtons();
}

DeviceListWindow::~DeviceListWindow(){
    for(const std::shared_ptr<AndroidDevice> &device: this->_model.devices()){
        device->disconnectAllDrives();
    }
    this->_adb.disconnect();
    this->_adb.close();
}

AndroidDevice *DeviceListWindow::selectedDevice() const{
    const QModelIndex selectedIndex = this->_view->currentIndex();
    return this->_model.indexToDevice(selectedIndex);
}

AndroidDrive *DeviceListWindow::selectedDrive() const{
    const QModelIndex selectedIndex = this->_view->currentIndex();
    return this->_model.indexToDrive(selectedIndex);
}

void DeviceListWindow::updateButtons(){
    //For some reason this is necessary for the view to update
    this->_view->hide();
    this->_view->show();

    const AndroidDevice *device = this->selectedDevice();
    const AndroidDrive *drive = this->selectedDrive();

    this->_mountButton->setEnabled(device != nullptr || drive != nullptr);
    this->_settingsButton->setEnabled(drive != nullptr);
    this->_openInExplorerButton->setEnabled(drive != nullptr && drive->isConnected());

    if(device != nullptr){
        if(device->numberOfConnectedDrives() > 0){
            this->_mountButton->setText(QObject::tr("&Unmount all drives"));
            this->_mountButton->setWhatsThis(QObject::tr("Unmounts all drives corresponding to the selected Android device.<br/><br/>This only unmounts the drives, the Android device itself will remain connected, so you will still be able to access it for example through ADB."));
        }
        else{
            this->_mountButton->setText(QObject::tr("&Mount all drives"));
            this->_mountButton->setWhatsThis(QObject::tr("Mounts a drive containing the internal storage of the selected Android device, as well as a drive for each external SD card that the selected device has, if any."));
        }
    }
    else if(drive != nullptr){
        if(drive->isConnected()){
            this->_mountButton->setText(QObject::tr("&Unmount drive"));
            this->_mountButton->setWhatsThis(QObject::tr("Unmounts the selected drive.<br/><br/>This only unmounts the drive, the Android device itself will remain connected, so you will still be able to access it for example through ADB."));
        }
        else{
            this->_mountButton->setText(QObject::tr("&Mount drive"));
            this->_mountButton->setWhatsThis(QObject::tr("Mounts a drive containing the selected internal storage or external SD card."));
        }
        this->_mountButton->setEnabled(!drive->mountingInProgress() && !drive->unmountingInProgress());
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

    //Find which devices are connected
    static const QRegularExpression newlineRegex("[\r\n]+");
    const QStringList result = QString::fromUtf8(this->_adb.readAllStandardOutput()).trimmed().split(newlineRegex);
    QStringList serialNumbers, offlineSerialNumbers;
    static const QRegularExpression spaceRegex("\\s+");
    for(const QString &line: result){
        if(line == "List of devices attached" || line.isEmpty()){
            continue;
        }
        const QStringList splittedLine = line.split(spaceRegex);
        const QString serialNumber = splittedLine[0];
        const bool offline = splittedLine[1] == "offline";
        const bool unauthorized = splittedLine[1] == "unauthorized";
        if(offline || unauthorized){
            offlineSerialNumbers.push_back(serialNumber);
            if(this->_model.timeSinceOffline(serialNumber) == 3){
                if(offline){
                    QMessageBox::warning(nullptr, "",
                        QObject::tr("Device %1 is offline.<br/><br/>Try unlocking the device, then unplugging it and re-plugging it.<br/><br/>If this error persists, you may be able to find solutions <a href=\"%2\">here</a> (any adb commands mentioned there can be run in the command prompt after running <code>cd \"%3\"</code>).")
                        .arg(serialNumber, "https://stackoverflow.com/q/14993855/4284627", QCoreApplication::applicationDirPath())
                    );
                }
                else if(unauthorized){
                    QMessageBox::warning(nullptr, "",
                        QObject::tr("Device %1 is unauthorized.<br/><br/>Try unlocking your device. If it shows you a dialog asking if you want to allow this computer to access phone data, tap \"Allow\". If it doesn't show that dialog, disable and re-enable USB debugging as explained <a href=\"%2\">here</a>.<br/><br/>If it still isn't working, try unplugging and then re-plugging your device.")
                        .arg(serialNumber, "https://github.com/GustavLindberg99/AndroidDrive?tab=readme-ov-file#setup")
                    );
                }
            }
        }
        else{
            serialNumbers.push_back(serialNumber);
        }
    }

    //Update the model
    this->_model.updateDevices(serialNumbers, offlineSerialNumbers);

    //Start ADB again to continue updating
    this->_adb.start("adb.exe", {"devices"});
}

void DeviceListWindow::handleDokanError(AndroidDrive *drive, int status){
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
            drive->connectDrive(Settings().driveLetter(drive), this->_model.parentDevice(drive));    //Try connecting it again in case Dokan is finished installing
            return;
        }
        errorMessage = QObject::tr("Dokan doesn't seem to be installed.<br/><br/>Would you like to install it now?");
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
    if(QMessageBox::critical(nullptr, "", QObject::tr("Could not mount drive %1: %2").arg(drive->completeName(), errorMessage), buttons) == QMessageBox::Yes){
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
        errorMessage = QObject::tr("ADB failed to start.<br/><br/>Either the adb.exe file is missing, or you may have insufficient permissions to invoke the program.");
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
