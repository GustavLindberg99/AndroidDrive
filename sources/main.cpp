#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTranslator>

#include "androiddevice.hpp"
#include "debuglogger.hpp"
#include "devicelistwindow.hpp"
#include "settingswindow.hpp"
#include "updates.hpp"
#include "version.h"

int main(int argc, char **argv){
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    DokanInit();


    //Load translations
    QTranslator translator, baseTranslator;
    QString language = Settings().language();
    if(language == "auto"){
        language = QLocale::system().name().section('_', 0, 0);
    }
    SettingsWindow::systemLanguageAvailable = translator.load(":/translations/androiddrive_" + language) || language == "en";
    (void) baseTranslator.load(":/translations/qtbase_" + language);
    app.installTranslator(&translator);
    app.installTranslator(&baseTranslator);


    //Create the tray icon and the windows
    auto trayIcon = std::make_unique<QSystemTrayIcon>(QIcon(":/icon.svg"));
    auto deviceListWindow = std::make_unique<DeviceListWindow>();
    auto settingsWindow = std::make_unique<SettingsWindow>(nullptr);
    const auto quit = [&trayIcon, &deviceListWindow, &settingsWindow](){
        AndroidDevice::quitOnLastDeletedDevice();
        trayIcon.reset();
        deviceListWindow.reset();
        settingsWindow.reset();
    };
    checkForUpdates(
        QUrl("https://github.com/GustavLindberg99/AndroidDrive"),
        QUrl("https://raw.githubusercontent.com/GustavLindberg99/AndroidDrive/main/sources/version.h"),
        QUrl("https://raw.githubusercontent.com/GustavLindberg99/AndroidDrive/main/AndroidDrive-portable.zip"),
        QUrl("https://raw.githubusercontent.com/GustavLindberg99/AndroidDrive/main/AndroidDrive-install.exe"),
        quit
    );


    //Initialize the device list window
    QObject::connect(deviceListWindow.get(), &DeviceListWindow::encounteredFatalError, quit);


    //Initialize the tray icon
    QMenu contextMenu;

    QAction *deviceListAction = contextMenu.addAction(QObject::tr("&Devices"));
    QObject::connect(deviceListAction, &QAction::triggered, deviceListWindow.get(), &QWidget::show);

    QAction *settingsAction = contextMenu.addAction(QObject::tr("&Settings"));
    QObject::connect(settingsAction, &QAction::triggered, settingsWindow.get(), &QWidget::show);

    QAction *debugAction = contextMenu.addAction(QObject::tr("Record Debug &Logs"));
    QObject::connect(debugAction, &QAction::triggered, [debugAction](){
        DebugLogger &logger = DebugLogger::getInstance();
        if(logger.isRecording()){
            logger.stop();
            debugAction->setText(QObject::tr("Record Debug &Logs"));
        }
        else{
            if(logger.start()){
                debugAction->setText(QObject::tr("Finish Recording Debug &Logs"));
                QMessageBox::information(nullptr, "", QObject::tr("Recording of debug logs has started.<br/><br/>You will be able to find the log file in %1.<br/><br/>If you're planning on attaching the log file to a bug report, keep in mind that the log file will contain the names of the files on your phone, so make sure that the filenames don't contain any sensitive information (The debug logs will only contain the file names, they won't contain the contents of any file).").arg(DebugLogger::getInstance().logFilePath()));
            }
            else{
                QMessageBox::critical(nullptr, "", QObject::tr("Failed to create log file."));
            }
        }
    });

    QAction *aboutAction = contextMenu.addAction(QObject::tr("&About AndroidDrive"));
    QObject::connect(aboutAction, &QAction::triggered, aboutAction, [](){
        QMessageBox msg;
        msg.setIconPixmap(QPixmap(":/icon.svg"));
        msg.setWindowIcon(QIcon(":/icon.svg"));
        msg.setWindowTitle(QObject::tr("About AndroidDrive"));
        msg.setText(QObject::tr("AndroidDrive version %1 by Gustav Lindberg.").arg(PROGRAMVERSION) + "<br/><br/>" + QObject::tr("Icons made by %3 and %4 from %1 are licensed by %2.").arg("<a href=\"https://www.iconfinder.com/\">www.iconfinder.com</a>", "<a href=\"http://creativecommons.org/licenses/by/3.0/\">CC 3.0 BY</a>", "<a href=\"https://www.iconfinder.com/pocike\">Alpár-Etele Méder</a>", "<a href=\"https://www.iconfinder.com/iconsets/tango-icon-library\">Tango</a>") + "<br/><br/>" + QObject::tr("This program uses %1 and %2.").arg("<a href=\"https://android.googlesource.com/platform/packages/modules/adb/\">ADB</a>", "<a href=\"https://dokan-dev.github.io/\">Dokan</a>"));
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
