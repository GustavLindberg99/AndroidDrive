#include "settingswindow.hpp"

#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

#include "androiddrive.hpp"

QSet<SettingsWindow*> SettingsWindow::_instances;
bool SettingsWindow::systemLanguageAvailable = true;
const QStringList SettingsWindow::_languageNames{"", "Deutsch", "English", "Français", "Magyar", "Italiano", "Svenska"};
const QStringList SettingsWindow::_languageAbbreviations{"auto", "de", "en", "fr", "hu", "it", "sv"};

SettingsWindow::SettingsWindow(const AndroidDrive *drive):
    _drive(drive)
{
    SettingsWindow::_instances.insert(this);

    this->setWindowTitle(QObject::tr("AndroidDrive - Settings"));
    this->setWindowIcon(QIcon(":/icon.svg"));
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, true);

    QGridLayout *layout = new QGridLayout(this);

    const auto enableApplyButton = [this](){this->_applyButton->setEnabled(true);};

    if(this->_drive != nullptr){
        QGroupBox *driveSettingsBox = new QGroupBox(QObject::tr("Drive settings for %1").arg(drive->completeName()), this);
        QFormLayout *driveSettingsLayout = new QFormLayout(driveSettingsBox);

        this->_driveLetter = new QComboBox(driveSettingsBox);
        QObject::connect(this->_driveLetter, &QComboBox::currentIndexChanged, this->_applyButton, enableApplyButton);
        this->_driveLetter->setWhatsThis(QObject::tr("Allows you to select a preferred drive letter for the selected Android drive.<br/><br/>If the preferred drive letter is unavailable when this drive is connected, it will use the next available drive letter in alphabetical order.<br/><br/>If you change the drive letter while this drive is connected, you will have to unmount it and re-mount it again for the changes to take effect."));
        driveSettingsLayout->addRow(QObject::tr("Drive &letter"), this->_driveLetter);

        QWidget *driveNameContainer = new QWidget(driveSettingsBox);
        QHBoxLayout *driveNameLayout = new QHBoxLayout(driveNameContainer);

        this->_driveName = new QLineEdit(driveNameContainer);
        QObject::connect(this->_driveName, &QLineEdit::textEdited, this->_applyButton, enableApplyButton);
        this->_driveName->setWhatsThis(QObject::tr("Allows you to select a name for the selected Android drive."));
        driveNameLayout->addWidget(this->_driveName);

        QPushButton *resetDriveName = new QPushButton(QObject::tr("&Reset"), driveNameContainer);
        QObject::connect(resetDriveName, &QPushButton::pressed, this->_driveName, [this](){
            const QString name = this->_drive->completeName();
            this->_driveName->setText(name);
            emit this->_driveName->textEdited(name);
        });
        resetDriveName->setWhatsThis(QObject::tr("Resets the name of the drive to the default value."));
        driveNameLayout->addWidget(resetDriveName);

        driveNameLayout->setContentsMargins(0, 0, 0, 0);
        driveNameContainer->setLayout(driveNameLayout);
        driveSettingsLayout->addRow(QObject::tr("Drive &name"), driveNameContainer);

        this->_autoConnect = new QCheckBox(QObject::tr("Automatically mount &drive"), driveSettingsBox);
        QObject::connect(this->_autoConnect, &QCheckBox::clicked, this->_applyButton, enableApplyButton);
        this->_autoConnect->setWhatsThis(QObject::tr("If this checkbox is checked, the selected drive will be automatically connected as a drive whenever you plug it into your computer.<br/><br/>Otherwise, you will have to mount it manually by going into Devices > Mount drive."));
        driveSettingsLayout->addRow(this->_autoConnect);

        driveSettingsBox->setWhatsThis(QObject::tr("These settings only affect the selected drive."));
        driveSettingsBox->setLayout(driveSettingsLayout);
        layout->addWidget(driveSettingsBox, 0, 0, 1, 3);
    }

    QGroupBox *globalSettingsBox = new QGroupBox(QObject::tr("Global settings"), this);
    QFormLayout *globalSettingsLayout = new QFormLayout(globalSettingsBox);

    this->_openInExplorer = new QCheckBox(QObject::tr("Open newly connected drives in &Explorer"), globalSettingsBox);
    QObject::connect(this->_openInExplorer, &QCheckBox::clicked, this->_applyButton, enableApplyButton);
    this->_openInExplorer->setWhatsThis(QObject::tr("If this checkbox is checked, whenever AndroidDrive is finished connecting a drive, it will open that drive in Windows Explorer."));
    globalSettingsLayout->addRow(this->_openInExplorer);

    this->_hideDotFiles = new QCheckBox(QObject::tr("&Hide files beginning with a dot"), globalSettingsBox);
    QObject::connect(this->_hideDotFiles, &QCheckBox::clicked, this->_applyButton, enableApplyButton);
    this->_hideDotFiles->setWhatsThis(QObject::tr("If this checkbox is checked, files that begin with a dot will be treated as hidden files, and will only be visible in Windows Explorer if Windows Explorer's \"Show hidden files\" option is activated."));
    globalSettingsLayout->addRow(this->_hideDotFiles);

    this->_language = new QComboBox(globalSettingsBox);
    QObject::connect(this->_language, &QComboBox::currentIndexChanged, this->_applyButton, enableApplyButton);
    for(const QString &language: SettingsWindow::_languageNames){
        if(language.isEmpty()){
            //QObject::tr("Use system language") can't go directly in _languageNames because otherwise it will be initialized before the translations are loaded
            this->_language->addItem(QObject::tr("Use system language"));
        }
        else{
            this->_language->addItem(language);
        }
    }
    this->_language->setWhatsThis(QObject::tr("Allows you to select which language AndroidDrive's GUI will be displayed in.<br/><br/>If you select \"Use system language\" but AndroidDrive isn't availiable in your system language, English will be used.<br/><br/>This setting has no effect on how the actual drive works.<br/><br/>You must restart AndroidDrive for this change to take effect."));
    globalSettingsLayout->addRow(QObject::tr("&Language"), this->_language);

    if(!systemLanguageAvailable){
        QLabel *contributeToTranslation = new QLabel(QObject::tr("AndroidDrive doesn't seem to be available in your system language.<br/><br/><a %1>Click here</a> if you would like to help translate it.").arg("href=\"https://github.com/GustavLindberg99/AndroidDrive/blob/main/sources/translations/contribute.md\""), this);
        contributeToTranslation->setWordWrap(true);
        contributeToTranslation->setTextFormat(Qt::RichText);
        contributeToTranslation->setTextInteractionFlags(Qt::TextBrowserInteraction);
        contributeToTranslation->setOpenExternalLinks(true);
        globalSettingsLayout->addRow(contributeToTranslation);
    }

    globalSettingsBox->setWhatsThis(QObject::tr("These settings affect all drives connected with AndroidDrive."));
    globalSettingsBox->setLayout(globalSettingsLayout);
    layout->addWidget(globalSettingsBox, drive != nullptr, 0, 1, 3);

    QPushButton *okButton = new QPushButton(QObject::tr("&OK"), this);
    QPushButton *cancelButton = new QPushButton(QObject::tr("&Cancel"), this);

    layout->addWidget(okButton, 1 + (drive != nullptr), 0);
    layout->addWidget(cancelButton, 1 + (drive != nullptr), 1);
    layout->addWidget(this->_applyButton, 1 + (drive != nullptr), 2);

    this->setLayout(layout);

    Settings() >> this;

    QObject::connect(okButton, &QPushButton::clicked, this, [this](){
        this->_applyButton->click();
        this->close();
    });
    QObject::connect(cancelButton, &QPushButton::clicked, this, [this](){
        Settings() >> this;
        this->close();
    });
    QObject::connect(this->_applyButton, &QPushButton::clicked, this, [this](){
        Settings settings;
        settings << this;
        for(SettingsWindow *settingsWindow: qAsConst(SettingsWindow::_instances)){
            settings >> settingsWindow;
        }
    });
}

SettingsWindow::~SettingsWindow(){
    SettingsWindow::_instances.remove(this);
}

Settings &operator<<(Settings &settings, const SettingsWindow *settingsWindow){
    if(settingsWindow->_drive != nullptr){
        settings.setDriveLetter(settingsWindow->_drive, settingsWindow->_driveLetter->currentText().at(0).toLatin1());
        settings.setDriveName(settingsWindow->_drive, settingsWindow->_driveName->text());
        settings.setAutoConnect(settingsWindow->_drive, settingsWindow->_autoConnect->isChecked());
    }
    settings.setOpenInExplorer(settingsWindow->_openInExplorer->isChecked());
    settings.setHideDotFiles(settingsWindow->_hideDotFiles->isChecked());
    settings.setLanguage(SettingsWindow::_languageAbbreviations[settingsWindow->_language->currentIndex()]);
    settingsWindow->_applyButton->setEnabled(false);
    return settings;
}

const Settings &operator>>(const Settings &settings, SettingsWindow *settingsWindow){
    if(settingsWindow->_drive != nullptr){
        settingsWindow->_driveLetter->clear();
        for(char letter = 'A'; letter <= 'Z'; letter++){
            settingsWindow->_driveLetter->addItem(letter + QString(":"));
            if(letter == settings.driveLetter(settingsWindow->_drive)){
                settingsWindow->_driveLetter->setCurrentIndex(settingsWindow->_driveLetter->count() - 1);
            }
        }
        settingsWindow->_driveName->setText(settings.driveName(settingsWindow->_drive));
        settingsWindow->_autoConnect->setChecked(settings.autoConnect(settingsWindow->_drive));
    }
    settingsWindow->_openInExplorer->setChecked(settings.openInExplorer());
    settingsWindow->_hideDotFiles->setChecked(settings.hideDotFiles());
    settingsWindow->_language->setCurrentIndex(SettingsWindow::_languageAbbreviations.indexOf(settings.language()));
    settingsWindow->_applyButton->setEnabled(false);
    return settings;
}
