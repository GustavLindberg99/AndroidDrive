#ifndef ANDROIDDEVICE_H
#define ANDROIDDEVICE_H

#include <QString>
#include <QList>
#include <QTemporaryDir>
#include <QFileSystemWatcher>
#include <QTimer>
#include <functional>

class AndroidDevice{
public:
    AndroidDevice(const QString &id);

    bool operator==(const AndroidDevice &other) const;
    bool operator!=(const AndroidDevice &other) const;

    QString id() const;
    QString name() const;
    QString storageLocation() const;
    bool isValid() const;

    void setDriveLetter(char letter);
    char driveLetter() const;

    void connectDrive(const std::function<void(const QString&)> &callback);
    void autoconnectDrive();
    void disconnectDrive(bool disconnectInSettings = false) const;
    bool isConnected() const;

    static QList<AndroidDevice> allDevices();

private:
    const QString _id;
    char _driveLetter;

    //These need to be static for performance reasons (otherwise ADB would be called each time the AndroidDevice constructor is called which would be bad for performance)
    static QMap<QString, QString> _name;
    static QMap<QString, QString> _storageLocation;

    //These need to be static so that they can be saved across multiple AndroidDevice instances representing the same device
    static QTemporaryDir _temp;
    static QMap<QString, QFileSystemWatcher*> _fileSystemWatchers;
    static QMap<QString, QTimer*> _pullTimers;

    QDir temporaryDir() const;
    void checkDriveLetter();
};

#endif // ANDROIDDEVICE_H
