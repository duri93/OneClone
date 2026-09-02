#include "SettingsTabController.h"

#include "src/common/Config.h"
#include "src/common/LocalPathAutocompleter.h"
#include "src/core/SharedSettings.h"
#include "src/core/Status.h"
#include "src/providers/platform/AutostartManager.h"
#include "src/ui/ui_SettingsTabController.h"

#include <QDir>
#include <QFileDialog>
#include <QProcess>

SettingsTabController::SettingsTabController(AppContext* appContext, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SettingsTabController)
    , m_appContext(appContext)
{
    ui->setupUi(this);

    ui->settingsAdvancedScrollarea->hide();
    LocalPathAutocompleter::attach(ui->settingsRclone,
                                   LocalPathAutocompleter::Mode::FoldersAndFiles,
                                   {"rclone.exe"});

    loadFromSettings();

    connect(ui->settingsAdvanced,     &QCheckBox::checkStateChanged, this, &SettingsTabController::onAdvancedToggled);
    connect(ui->settingsSave,         &QPushButton::clicked, this, &SettingsTabController::onSave);
    connect(ui->settingsRcloneButton, &QToolButton::clicked, this, &SettingsTabController::onRcloneSelectClicked);
    connect(ui->settingsRcloneConf,   &QPushButton::clicked, this, &SettingsTabController::onRcloneConfClicked);
    connect(ui->settingsWizard,       &QPushButton::clicked, this, &SettingsTabController::wizardRequested);
}

SettingsTabController::~SettingsTabController()
{
    delete ui;
}

void SettingsTabController::loadFromSettings()
{
    bool isRegistered = AutostartManager::isEnabled(Config::APP_ID);

    const SharedSettings* s = m_appContext->shared();
    ui->settingsRclone            ->setText   (s->rclonePath());
    ui->settingsAdvanced          ->setChecked(s->advanced());
    ui->settingsBufferSize        ->setValue  (s->bufferSize());
    ui->settingsCacheMaxSize      ->setValue  (s->cacheMaxSize());
    ui->settingsCacheMinFreeSpace ->setValue  (s->cacheMinFreeSpace());
    ui->settingsCacheMaxAge       ->setValue  (s->cacheMaxAge());
    ui->settingsReadChunkSize     ->setValue  (s->readChunkSize());
    ui->settingsReadChunkSizeLimit->setValue  (s->readChunkSizeLimit());
    ui->settingsTransfers         ->setValue  (s->transfers());
    ui->settingsCheckers          ->setValue  (s->checkers());
    ui->settingsLinks             ->setChecked(s->links());
    ui->settingsAutostart         ->setChecked(isRegistered);

    // settingsCacheMode combobox: find matching text
    int idx = ui->settingsCacheMode->findText(s->cacheMode());
    if (idx >= 0) ui->settingsCacheMode->setCurrentIndex(idx);
}

void SettingsTabController::saveToSettings()
{
    SharedSettings* s = m_appContext->shared();
    s->setRclonePath(ui->settingsRclone->text());
    s->setAdvanced(ui->settingsAdvanced->isChecked());
    s->setCacheMode(ui->settingsCacheMode->currentText());
    s->setCacheMaxSize(ui->settingsCacheMaxSize->value());
    s->setCacheMinFreeSpace(ui->settingsCacheMinFreeSpace->value());
    s->setCacheMaxAge(ui->settingsCacheMaxAge->value());
    s->setReadChunkSize(ui->settingsReadChunkSize->value());
    s->setReadChunkSizeLimit(ui->settingsReadChunkSizeLimit->value());
    s->setBufferSize(ui->settingsBufferSize->value());
    s->setTransfers(ui->settingsTransfers->value());
    s->setCheckers(ui->settingsCheckers->value());
    s->setLinks(ui->settingsLinks->isChecked());

    // register or unregister startup
    AutostartManager::setEnabled(Config::APP_ID, ui->settingsAutostart->isChecked());
}

void SettingsTabController::onSave()
{
    saveToSettings();

    if (m_appContext->save()) {
        Status::notify("Settings saved.", Status::Level::Success);
    } else {
        Status::notify("Error saving settings.", Status::Level::Error);
    }

    emit settingsSaved();
}

void SettingsTabController::onRcloneSelectClicked()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Select rclone.exe", ui->settingsRclone->text(),
        "Executable (*.exe);;All files (*.*)");
    if (!path.isEmpty()) {
        ui->settingsRclone->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsTabController::onAdvancedToggled()
{
    if (ui->settingsAdvanced->isChecked()) {
        ui->settingsAdvancedScrollarea->show();
    } else {
        ui->settingsAdvancedScrollarea->hide();
    }
}

void SettingsTabController::onRcloneConfClicked()
{
    QProcess* process = m_appContext->rcloneProvider()->openConfig(m_appContext->shared()->rclonePath(), m_appContext);
    if (!process) {
        Status::notify("Failed to run 'rclone config'.", Status::Level::Error);
    }
}
