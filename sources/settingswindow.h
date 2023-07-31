#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include "androiddevice.h"

class AndroidDevice;

class SettingsWindow : public QDialog{
    Q_OBJECT

public:
    friend class Settings;

    SettingsWindow(const AndroidDevice *device);
    virtual ~SettingsWindow();

    friend Settings &operator<<(Settings &settings, const SettingsWindow *settingsWindow);
    friend const Settings &operator>>(const Settings &settings, SettingsWindow *settingsWindow);

private:
    static QSet<SettingsWindow*> _instances;

    const AndroidDevice *const _device;

    QGridLayout _layout;
    QGroupBox _deviceSettingsBox, _globalSettingsBox;
    QFormLayout _deviceSettingsLayout, _globalSettingsLayout;

    QComboBox _driveLetter;
    QCheckBox _autoConnect;
    QCheckBox _openInExplorer, _hideDotFiles;

    mutable QPushButton _okButton, _cancelButton, _applyButton;
};

class Settings: public QSettings{
public:
    Settings();

    char driveLetter(const AndroidDevice *device) const;
    bool autoConnect(const AndroidDevice *device) const;
    bool openInExplorer() const;
    bool hideDotFiles() const;
};

#endif // SETTINGSWINDOW_H
