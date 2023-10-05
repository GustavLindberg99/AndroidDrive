#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QIcon>
#include <QProcess>
#include <QProgressBar>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryDir>
#include "updates.h"
#include "version.h"

//This file is shared between projects, which is why there is both Windows-specific and Linux-specific code regardless of whether or not the project is cross-platform.

void installUpdates(const QUrl &githubRepo, const QUrl &zipFileOrLinuxBinaryOrInstaller, const std::function<void()> &quit){
    #ifdef _WIN32
        if(!zipFileOrLinuxBinaryOrInstaller.toString().endsWith(".exe") && !QFile::exists("C:\\Windows\\system32\\tar.exe")){
            //tar.exe is needed to unzip the contents of the update when installing it on Windows, so if it doesn't exist then just open the GitHub page so that the user can install the update manually
            QDesktopServices::openUrl(githubRepo);
            return;
        }
    #endif

    QProgressBar *progressBar = new QProgressBar();
    progressBar->setWindowTitle(QObject::tr("Installing updates..."));
    progressBar->setWindowFlags(Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
    progressBar->setWindowIcon(QIcon(":/icon.ico"));
    progressBar->setFixedWidth(300);
    progressBar->setMaximum(0);
    progressBar->setTextVisible(false);
    progressBar->show();

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, manager, [=](QNetworkReply *reply){
        manager->deleteLater();

        if(reply->error()){
            QDesktopServices::openUrl(githubRepo);
            delete progressBar;
            return;
        }

        //Create a temporary file to put the newly downloaded files in
        QTemporaryDir temp;
        temp.setAutoRemove(false);

        //Create the ZIP file or Linux binary with the downloaded contents
        QFile downloadeFile(temp.filePath(zipFileOrLinuxBinaryOrInstaller.fileName()));
        if(!downloadeFile.open(QIODevice::WriteOnly)){
            QDesktopServices::openUrl(githubRepo);
            delete progressBar;
            return;
        }
        downloadeFile.write(reply->readAll());
        downloadeFile.close();

        #ifdef _WIN32
            //If we use the installer, just launch the installer
            if(zipFileOrLinuxBinaryOrInstaller.toString().endsWith(".exe")){
                QProcess::startDetached(downloadeFile.fileName());
                delete progressBar;
                quit();
                return;
            }

            //Unzip the ZIP file
            QProcess tar;
            tar.setWorkingDirectory(temp.path());
            tar.start("tar.exe", {"-xf", downloadeFile.fileName()});
            tar.waitForFinished();
            if(tar.exitCode() != 0){
                QDesktopServices::openUrl(githubRepo);
                delete progressBar;
                return;
            }
            downloadeFile.remove();
        #endif

        //Create a vbscript or bash file that copies the new version to the current folder. We have to do this in a seperate process otherwise the currently running executable file will be locked because it's in use.
        QFile vbsOrBashFile(temp.filePath("copyupdate"
            #ifdef _WIN32
                ".vbs"
            #else
                ".sh"
            #endif
        ));
        if(!vbsOrBashFile.open(QIODevice::WriteOnly | QIODevice::Text)){
            QDesktopServices::openUrl(githubRepo);
            delete progressBar;
            return;
        }
        const QString mainExecutable = QFileInfo(qApp->arguments()[0]).absoluteFilePath();
        const QString temporaryFolder = temp.path();
        #ifdef _WIN32
            const QString destinationFolder = QDir(QApplication::applicationDirPath()).absolutePath();
            const QString vbscript(
                //Test if the destination folder can be written to without admin rights
                "set fso = createObject(\"Scripting.FileSystemObject\")\n"
                "on error resume next\n"
                "testFolder = \"" + destinationFolder + "/permissiontest\"\n"
                "fso.CreateFolder testFolder\n"
                "if fso.folderExists(testFolder) then\n"
                    "fso.deleteFolder testFolder, true\n"
                "end if\n"

                //If it can't, restart the script with admin rights
                "if err.number <> 0 then\n"
                    "createObject(\"Shell.Application\").shellExecute wScript.fullName _\n"
                        ", \"\"\"\" & wScript.scriptFullName & \"\"\"\", \"\", \"runas\", 1\n"
                    "wScript.quit\n"
                "end if\n"

                //Delete the current .vbs file so that it doens't go in the installation folder with the other files
                "fso.deleteFile \"" + temporaryFolder + "/copyupdate.vbs\"\n"

                //As long as the program is still running, we can't do anything because the .exe file is locked
                "do\n"
                    "err.clear\n"
                    "fso.openTextFile \"" + mainExecutable + "\", 8, false\n"
                "loop while err.number <> 0\n"
                "err.clear\n"

                //Copy the new version from the temporary folder to the folder where the old version is installed
                "fso.copyFolder \"" + temporaryFolder + "\", \"" + destinationFolder + "\"\n"
                "set objShell = wScript.createObject(\"wScript.Shell\")\n"
                "if err.number <> 0 then\n"
                    //If there was an error, open GitHub so that the user can install the update manually
                    "objShell.run \"" + githubRepo.toString() + "\"\n"
                "else\n"
                    //Restart the program
                    "objShell.run \"\"\"" + qApp->arguments().join("\"\" \"\"") + "\"\"\""
                "end if\n"

                //We don't need the temporary folder anymore, so delete it
                "fso.deleteFolder \"" + temporaryFolder + "\"\n"
            );
            vbsOrBashFile.write(vbscript.toUtf8());
            vbsOrBashFile.close();

            QProcess::startDetached("wscript.exe", {vbsOrBashFile.fileName()});
        #else
            const QString bashScript(
                "mainExecutable=$(printf '%q\n' \"$1\")\n"
                "temporaryFolder=$(printf '%q\n' \"$2\")\n"
                "downloadedFileName=$(printf '%q\n' \"$3\")\n"
                "githubRepo=$(printf '%q\n' \"$4\")\n"
                "if mv \"$downloadedFileName\" \"$mainExecutable\"; then\n"
                    "\"$mainExecutable\"\n"
                "else\n"
                    "python -m webbrowser \"$githubRepo\"\n"
                "fi\n"
                "rm -rf \"$temporaryFolder\""
            );
            vbsOrBashFile.write(bashScript.toUtf8());
            vbsOrBashFile.setPermissions(static_cast<QFileDevice::Permission>(0x777));
            vbsOrBashFile.close();

            QProcess::startDetached("bash", {vbsOrBashFile.fileName(), mainExecutable, temporaryFolder, downloadeFile.fileName(), githubRepo.toString()});
        #endif

        delete progressBar;
        quit();
    });
    manager->get(QNetworkRequest(zipFileOrLinuxBinaryOrInstaller));
}

void checkForUpdates(const QUrl &githubRepo, const QUrl &versionHeader, const QUrl &zipFileOrLinuxBinary, const QUrl &windowsInstaller, const std::function<void()> &quit){
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, manager, [=](QNetworkReply *reply){
        manager->deleteLater();

        if(reply->error()){
            //If there's an error, it's probably because the user isn't connected to internet, in which case we can check for updates another time instead
            return;
        }

        const QString versionHeaderContents = QString::fromUtf8(reply->readAll());

        //clazy:excludeall=use-static-qregularexpression
        //Don't use static QRegularExpressions here because this function will only be executed once
        const QRegularExpressionMatch majorVersionMatch = QRegularExpression("#define\\s+MAJORVERSION\\s+([0-9]+)\\s*[\r\n]").match(versionHeaderContents);
        const QRegularExpressionMatch minorVersionMatch = QRegularExpression("#define\\s+MINORVERSION\\s+([0-9]+)\\s*[\r\n]").match(versionHeaderContents);
        const QRegularExpressionMatch patchVersionMatch = QRegularExpression("#define\\s+PATCHVERSION\\s+([0-9]+)\\s*[\r\n]").match(versionHeaderContents);
        if(!majorVersionMatch.hasMatch() || !minorVersionMatch.hasMatch() || !patchVersionMatch.hasMatch()){
            return;
        }
        const int mostRecentMajorVersion = majorVersionMatch.captured(1).toInt();
        const int mostRecentMinorVersion = minorVersionMatch.captured(1).toInt();
        const int mostRecentPatchVersion = patchVersionMatch.captured(1).toInt();

        const bool updateIsAvailable = mostRecentMajorVersion > MAJORVERSION || (mostRecentMajorVersion == MAJORVERSION && (
                                       mostRecentMinorVersion > MINORVERSION || (mostRecentMinorVersion == MINORVERSION &&
                                       mostRecentPatchVersion > PATCHVERSION)));

        if(updateIsAvailable && QMessageBox::question(nullptr, "", QObject::tr("An update is available. Do you want to install it now?")) == QMessageBox::Yes){
            #ifdef _WIN32
                const bool useInstaller = QSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", QSettings::NativeFormat).childGroups().contains(GUID "_is1", Qt::CaseInsensitive)
                          || QSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", QSettings::NativeFormat).childGroups().contains(GUID "_is1", Qt::CaseInsensitive);
            #else
                const bool useInstaller = false;
            #endif
            installUpdates(githubRepo, useInstaller ? windowsInstaller : zipFileOrLinuxBinary, quit);
        }
    });
    manager->get(QNetworkRequest(versionHeader));
}
