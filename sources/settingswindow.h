#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include "androiddrive.h"

class AndroidDevice;

class SettingsWindow : public QDialog{
    Q_OBJECT

public:
    friend class Settings;

    SettingsWindow(const AndroidDrive *drive);
    virtual ~SettingsWindow();

    friend Settings &operator<<(Settings &settings, const SettingsWindow *settingsWindow);
    friend const Settings &operator>>(const Settings &settings, SettingsWindow *settingsWindow);

    static bool systemLanguageAvailable;

private:
    static QSet<SettingsWindow*> _instances;
    static const QStringList _languageNames, _languageAbbreviations;

    const AndroidDrive *const _drive;

    QComboBox *_driveLetter = nullptr;
    QLineEdit *_driveName = nullptr;
    QCheckBox *_autoConnect = nullptr;

    QCheckBox *_openInExplorer;
    QCheckBox *_hideDotFiles;
    QComboBox *_language;

    QPushButton *const _applyButton = new QPushButton(QObject::tr("&Apply"), this);
};

class Settings: public QSettings{
public:
    Settings();

    char driveLetter(const AndroidDrive *drive) const;
    QString driveName(const AndroidDrive *drive) const;
    bool autoConnect(const AndroidDrive *drive) const;
    bool openInExplorer() const;
    bool hideDotFiles() const;
    QString language() const;
};

#endif // SETTINGSWINDOW_H
