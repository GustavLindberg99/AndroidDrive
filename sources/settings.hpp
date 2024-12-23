#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class AndroidDrive;

class Settings final {
public:
    /**
     * Constructs a settings object containing the currently saved settings.
     */
    Settings() = default;

    /**
     * Gets the drive letter that the user prefers for the given drive.
     *
     * @param drive - The drive to get the drive letter for.
     *
     * @return The drive letter for the given drive.
     */
    char driveLetter(const AndroidDrive *drive) const;

    /**
     * Sets the drive letter that the user prefers for the given drive (does not change the drive object itself, only the settings).
     *
     * @param drive - The drive to set the drive letter for.
     * @param driveLetter - The drive letter to set.
     */
    void setDriveLetter(const AndroidDrive *drive, char driveLetter);

    /**
     * Gets the name that the user set for the given drive.
     *
     * @param drive - The drive to get the name of.
     *
     * @return The name of the given drive.
     */
    QString driveName(const AndroidDrive *drive) const;

    /**
     * Sets the name for the given drive (does not change the drive object itself, only the settings).
     *
     * @param drive - The drive to set the name of.
     * @param driveName - The name to set.
     */
    void setDriveName(const AndroidDrive *drive, const QString &driveName);

    /**
     * Gets whether the given drive should be automatically mounted when the device is connected.
     *
     * @param drive - The drive to get if it should be automatically mounted.
     *
     * @return True if it should be automatically mounted, false if it shouldn't.
     */
    bool autoConnect(const AndroidDrive *drive) const;

    /**
     * Aets whether the given drive should be automatically mounted when the device is connected (does not change the drive object itself, only the settings).
     *
     * @param drive - The drive to set if it should be automatically mounted.
     * @param autoConnect - True if it should be automatically mounted, false if it shouldn't.
     */
    void setAutoConnect(const AndroidDrive *drive, bool autoConnect);

    /**
     * Checks if newly connected drives should be opened in Windows Explorer.
     *
     * @return True if newly connected drives should be opened in Windows Explorer, false if they shouldn't.
     */
    bool openInExplorer() const;

    /**
     * Sets if newly connected drives should be opened in Windows Explorer.
     *
     * @param openInExplorer - True if newly connected drives should be opened in Windows Explorer, false if they shouldn't.
     */
    void setOpenInExplorer(bool openInExplorer);

    /**
     * Checks whether files beginning with a dot should be hidden.
     *
     * @return True if files beginning with a dot should be hidden, false if they shouldn't.
     */
    bool hideDotFiles() const;

    /**
     * Sets whether files beginning with a dot should be hidden.
     *
     * @return True if files beginning with a dot should be hidden, false if they shouldn't.
     */
    void setHideDotFiles(bool hideDotFiles);

    /**
     * Gets the language that the tray icon settings windows should be displayed in.
     *
     * @return The two-letter code of the language, or "auto" if the language should be automatically detected based on the system lanugage.
     */
    QString language() const;

    /**
     * Sets the language that the tray icon settings windows should be displayed in.
     *
     * @return The two-letter code of the language, or "auto" if the language should be automatically detected based on the system lanugage.
     */
    void setLanguage(const QString &language);

private:
    QSettings _settings = QSettings("Gustav Lindberg", "AndroidDrive");
};

#endif // SETTINGS_H
