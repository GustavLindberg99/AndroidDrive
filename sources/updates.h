#ifndef UPDATES_H
#define UPDATES_H

#include <QApplication>
#include <QUrl>
#include <functional>

void checkForUpdates(const QUrl &githubRepo, const QUrl &versionHeader, const QUrl &zipFileOrLinuxBinary, const QUrl &windowsInstaller, const std::function<void()> &quit = [](){qApp->quit();});

#endif // UPDATES_H
