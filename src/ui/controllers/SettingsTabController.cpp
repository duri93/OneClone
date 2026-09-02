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

    ui->advancedScrollarea->hide();
    LocalPathAutocompleter::attach(ui->rclone, LocalPathAutocompleter::Mode::FoldersAndFiles, {"rclone.exe"});

    loadFromSettings();

    connect(ui->advanced,       &QCheckBox::checkStateChanged, this, &SettingsTabController::onAdvancedToggled);
    connect(ui->save,           &QPushButton::clicked, this, &SettingsTabController::onSave);
    connect(ui->rcloneButton,   &QToolButton::clicked, this, &SettingsTabController::onRcloneSelectClicked);
    connect(ui->openRcloneConf, &QPushButton::clicked, this, &SettingsTabController::onRcloneConfClicked);
    connect(ui->openWizard,     &QPushButton::clicked, this, &SettingsTabController::wizardRequested);
}

SettingsTabController::~SettingsTabController()
{
    delete ui;
}

void SettingsTabController::loadFromSettings()
{
    bool isRegistered = AutostartManager::isEnabled(Config::APP_ID);

    const SharedSettings* s = m_appContext->shared();
    ui->rclone            ->setText   (s->rclonePath());
    ui->advanced          ->setChecked(s->advanced());
    ui->bufferSize        ->setValue  (s->bufferSize());
    ui->cacheMaxSize      ->setValue  (s->cacheMaxSize());
    ui->cacheMinFreeSpace ->setValue  (s->cacheMinFreeSpace());
    ui->cacheMaxAge       ->setValue  (s->cacheMaxAge());
    ui->readChunkSize     ->setValue  (s->readChunkSize());
    ui->readChunkSizeLimit->setValue  (s->readChunkSizeLimit());
    ui->transfers         ->setValue  (s->transfers());
    ui->checkers          ->setValue  (s->checkers());
    ui->symlinks          ->setChecked(s->links());
    ui->autostart         ->setChecked(isRegistered);

    // settingsCacheMode combobox: find matching text
    int idx = ui->cacheMode->findText(s->cacheMode());
    if (idx >= 0) ui->cacheMode->setCurrentIndex(idx);
}

void SettingsTabController::saveToSettings()
{
    SharedSettings* s = m_appContext->shared();
    s->setRclonePath(ui->rclone->text());
    s->setAdvanced(ui->advanced->isChecked());
    s->setCacheMode(ui->cacheMode->currentText());
    s->setCacheMaxSize(ui->cacheMaxSize->value());
    s->setCacheMinFreeSpace(ui->cacheMinFreeSpace->value());
    s->setCacheMaxAge(ui->cacheMaxAge->value());
    s->setReadChunkSize(ui->readChunkSize->value());
    s->setReadChunkSizeLimit(ui->readChunkSizeLimit->value());
    s->setBufferSize(ui->bufferSize->value());
    s->setTransfers(ui->transfers->value());
    s->setCheckers(ui->checkers->value());
    s->setLinks(ui->symlinks->isChecked());

    // register or unregister startup
    AutostartManager::setEnabled(Config::APP_ID, ui->autostart->isChecked());
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
        this, "Select rclone.exe", ui->rclone->text(),
        "Executable (*.exe);;All files (*.*)");
    if (!path.isEmpty()) {
        ui->rclone->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsTabController::onAdvancedToggled()
{
    if (ui->advanced->isChecked()) {
        ui->advancedScrollarea->show();
    } else {
        ui->advancedScrollarea->hide();
    }
}

void SettingsTabController::onRcloneConfClicked()
{
    QProcess* process = m_appContext->rcloneProvider()->openConfig(m_appContext->shared()->rclonePath(), m_appContext);
    if (!process) {
        Status::notify("Failed to run 'rclone config'.", Status::Level::Error);
    }
}
