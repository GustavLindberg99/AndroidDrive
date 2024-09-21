#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QSettings>

#include "settings.h"

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    /**
     * Constructs a settings window for a drive (without opening the window).
     *
     * @param drive - A non-owning pointer to the drive that these settings are for (the settings window should be owned by the drive to ensure that it's closed when the drive is destroyed). Can be null to show only general settings.
     */
    SettingsWindow(const AndroidDrive *drive);

    /**
     * Destructor.
     */
    virtual ~SettingsWindow();

    /**
     * Saves the settings specified in a settings window to a settings object.
     *
     * @param settings - The settings object to save the settings to.
     * @param settingsWindow - The settings window to get the settings from.
     *
     * @return The settings object to enable chaining.
     */
    friend Settings &operator<<(Settings &settings, const SettingsWindow *settingsWindow);

    /**
     * Displays the settings from a settings object in a settings window.
     *
     * @param settings - The settings object to load the settings from.
     * @param settingsWindow - The settings window to display the settings in.
     *
     * @return The settings object to enable chaining.
     */
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

#endif // SETTINGSWINDOW_H
