#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include "androiddevice.h"
#include "devicelistwindow.h"
#include "programinfo.h"
#include "settingswindow.h"

int main(int argc, char **argv){
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    DokanInit();


    //Create the tray icon and the windows
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(QIcon(":/icon.ico"));
    DeviceListWindow *deviceListWindow = new DeviceListWindow();
    SettingsWindow *settingsWindow = new SettingsWindow(nullptr);
    const auto quit = [&app, &trayIcon, &deviceListWindow, &settingsWindow](){
        delete trayIcon;
        trayIcon = nullptr;
        deviceListWindow->deleteLater();
        deviceListWindow = nullptr;
        delete settingsWindow;
        settingsWindow = nullptr;
        AndroidDevice::shutdownAllDevices([&app](){
            app.quit();
        });
    };


    //Initialize the device list window
    QObject::connect(deviceListWindow, &DeviceListWindow::encounteredFatalError, quit);


    //Initialize the tray icon
    QMenu contextMenu;

    QAction *deviceListAction = contextMenu.addAction(QObject::tr("&Devices"));
    QObject::connect(deviceListAction, &QAction::triggered, deviceListWindow, &QWidget::show);

    QAction *settingsAction = contextMenu.addAction(QObject::tr("&Settings"));
    QObject::connect(settingsAction, &QAction::triggered, settingsWindow, &QWidget::show);

    QAction *aboutAction = contextMenu.addAction(QObject::tr("&About AndroidDrive"));
    QObject::connect(aboutAction, &QAction::triggered, aboutAction, [](){
        QMessageBox msg;
        msg.setIconPixmap(QPixmap(":/icon.ico"));
        msg.setWindowIcon(QIcon(":/icon.ico"));
        msg.setWindowTitle(QObject::tr("About AndroidDrive"));
        msg.setText(QObject::tr("AndroidDrive version %1 by Gustav Lindberg.").arg(PROGRAMVERSION) + "<br><br>" + QObject::tr("Icons made by %3 and %4 from %1 are licensed by %2.").arg("<a href=\"https://www.iconfinder.com/\">www.iconfinder.com</a>", "<a href=\"http://creativecommons.org/licenses/by/3.0/\">CC 3.0 BY</a>", "<a href=\"https://www.iconfinder.com/pocike\">Alpár-Etele Méder</a>", "<a href=\"https://www.iconfinder.com/iconsets/tango-icon-library\">Tango</a>") + "<br><br>" + QObject::tr("This program uses %1 and %2.").arg("<a href=\"https://android.googlesource.com/platform/packages/modules/adb/\">ADB</a>", "<a href=\"https://dokan-dev.github.io/\">Dokan</a>"));
        msg.exec();
    });

    QAction *aboutQtAction = contextMenu.addAction(QObject::tr("About &Qt"));
    QObject::connect(aboutQtAction, &QAction::triggered, [](){
        QMessageBox::aboutQt(nullptr);
    });

    QAction *exitAction = contextMenu.addAction(QObject::tr("E&xit"));
    QObject::connect(exitAction, &QAction::triggered, quit);
    trayIcon->setContextMenu(&contextMenu);

    trayIcon->setToolTip("AndroidDrive");
    trayIcon->show();


    //Run the program
    const int status = app.exec();
    DokanShutdown();
    return status;
}
