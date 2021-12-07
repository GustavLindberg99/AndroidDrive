#include <QApplication>
#include <QtWidgets>
#include <winerror.h>
#include "devicelistwindow.h"
#include "programinfo.h"

DeviceListWindow *deviceListWindow = nullptr;

int main(int argc, char **argv){
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    //Check if this program is already running
    QSharedMemory sharedMemory;
    sharedMemory.setKey("AndroidDrive");
    if(!sharedMemory.create(1)){
        QMessageBox::information(nullptr, "", QObject::tr("AndroidDrive is already running."));
        return ERROR_SERVICE_ALREADY_RUNNING;
    }

    //Clean up drives that may still be connected because this program crashed (put this in its own scope so that it doesn't stay in memory during the whole program execution)
    {
        QProcess subst;
        subst.start("C:\\Windows\\System32\\subst.exe", QStringList());
        subst.waitForFinished(-1);
        const QStringList &drives = QString::fromUtf8(subst.readAllStandardOutput()).split(QRegularExpression("[^\\S\r\n]*[\r\n]+[^\\S\r\n]*"));
        for(const QString &driveInfo: drives){
            const QRegularExpressionMatch &match = QRegularExpression("^([A-Za-z]:)\\\\:\\s=>\\s[^\r\n]+[\\\\/]sdcard[\\\\/]?$").match(driveInfo);
            if(match.hasMatch()){
                QProcess::startDetached("C:\\Windows\\System32\\subst.exe", {match.captured(1), "/d"});
            }
        }
    }

    //Create the tray icon
    QSystemTrayIcon trayIcon;
    trayIcon.setToolTip(QObject::tr("AndroidDrive"));

    QMenu contextMenu;

    QAction *deviceListAction = contextMenu.addAction(QObject::tr("&Devices"));
    deviceListWindow = new DeviceListWindow();
    QObject::connect(deviceListAction, &QAction::triggered, deviceListWindow, &DeviceListWindow::show);
    QObject::connect(deviceListAction, &QAction::triggered, deviceListWindow, &DeviceListWindow::updateDevices);

    QAction *aboutAction = contextMenu.addAction(QObject::tr("&About"));
    QObject::connect(aboutAction, &QAction::triggered, [](){
        QMessageBox msg;
        msg.setIconPixmap(QPixmap(":/icon.ico"));
        msg.setWindowIcon(QIcon(":/icon.ico"));
        msg.setWindowTitle(QObject::tr("About"));
        msg.setText(QObject::tr("AndroidDrive version %1 by Gustav Lindberg.").arg(PROGRAMVERSION) + "<br><br>" + QObject::tr("Icons made by %3 and %4 from %1 are licensed by %2.").arg("<a href=\"https://www.iconfinder.com/\">www.iconfinder.com</a>", "<a href=\"http://creativecommons.org/licenses/by/3.0/\">CC 3.0 BY</a>", "<a href=\"https://www.iconfinder.com/pocike\">Alpár-Etele Méder</a>", "<a href=\"https://www.iconfinder.com/iconsets/tango-icon-library\">Tango</a>") + "<br><br>" + QObject::tr("This program uses %1 by Google and %2 by temblast.com.").arg("<a href=\"https://android.googlesource.com/platform/packages/modules/adb/\">ADB</a>", "<a href=\"http://www.temblast.com/adbsync.htm\">AdbSync</a>"));
        msg.exec();
    });

    QAction *exitAction = contextMenu.addAction(QObject::tr("E&xit"));
    QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);
    trayIcon.setContextMenu(&contextMenu);

    trayIcon.setIcon(QIcon(":/icon.ico"));
    trayIcon.show();

    //Automatically connect new drives
    QList<AndroidDevice> connectedDevices;
    QTimer checkDevicesTimer;
    QObject::connect(&checkDevicesTimer, &QTimer::timeout, [&](){
        const QList<AndroidDevice> &allDevices = AndroidDevice::allDevices();
        if(connectedDevices != allDevices){
            for(AndroidDevice device: allDevices){
                if(!connectedDevices.contains(device)){
                    device.autoconnectDrive();
                }
            }
        }
        connectedDevices = allDevices;
    });
    checkDevicesTimer.setInterval(5000);
    checkDevicesTimer.start();

    //Run the program
    const int exitCode = app.exec();

    //Clean up
    delete deviceListWindow;
    deviceListWindow = nullptr;
    const QList<AndroidDevice> &allDevices = AndroidDevice::allDevices();
    for(const AndroidDevice &device: allDevices){
        device.disconnectDrive();
    }
    return exitCode;
}
