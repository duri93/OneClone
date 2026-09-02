#include "JobDetailsTabController.h"

#include "src/common/Config.h"
#include "src/common/LocalPathAutocompleter.h"
#include "src/core/Job.h"
#include "src/core/Status.h"
#include "src/providers/rclone/RCloneProvider.h"
#include "src/ui/ui_JobDetailsTabController.h"

#include <QDir>
#include <QFileDialog>

JobDetailsTabController::JobDetailsTabController(AppContext* appContext, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::JobDetailsTabController)
    , m_appContext(appContext)
{
    ui->setupUi(this);

    ui->command->document()->setMaximumBlockCount(Config::MAX_OUTPUT_LINES);
    LocalPathAutocompleter::attach(ui->local);

    connect(ui->openLog,     &QPushButton::clicked,           this, &JobDetailsTabController::onOpenLogClicked);
    connect(ui->save,        &QPushButton::clicked,           this, &JobDetailsTabController::onSaveClicked);
    connect(ui->remove,      &QPushButton::clicked,           this, &JobDetailsTabController::onDeleteClicked);
    connect(ui->localButton, &QToolButton::clicked,           this, &JobDetailsTabController::onLocalSelectClicked);
    connect(ui->type,        &QComboBox::currentIndexChanged, this, &JobDetailsTabController::onTypeChanged);

    // Keep the generated command preview in sync with every field that
    // feeds into it, so it updates instantly as the user edits the job.
    connect(ui->type,     &QComboBox::currentIndexChanged, this, &JobDetailsTabController::updateCommandPreview);
    connect(ui->local,    &QLineEdit::textChanged,          this, &JobDetailsTabController::updateCommandPreview);
    connect(ui->remote,   &QLineEdit::textChanged,          this, &JobDetailsTabController::updateCommandPreview);
    connect(ui->readOnly, &QCheckBox::toggled,              this, &JobDetailsTabController::updateCommandPreview);

    // Keep the "unsaved changes" indicator in sync with every field the
    // user can edit, so it reflects whether the form still matches the
    // stored job.
    connect(ui->name,      &QLineEdit::textChanged,          this, &JobDetailsTabController::updateUnsavedIndicator);
    connect(ui->type,      &QComboBox::currentIndexChanged,  this, &JobDetailsTabController::updateUnsavedIndicator);
    connect(ui->local,     &QLineEdit::textChanged,          this, &JobDetailsTabController::updateUnsavedIndicator);
    connect(ui->remote,    &QLineEdit::textChanged,          this, &JobDetailsTabController::updateUnsavedIndicator);
    connect(ui->autostart, &QCheckBox::toggled,              this, &JobDetailsTabController::updateUnsavedIndicator);
    connect(ui->readOnly,  &QCheckBox::toggled,              this, &JobDetailsTabController::updateUnsavedIndicator);

    clear();
}

JobDetailsTabController::~JobDetailsTabController()
{
    delete ui;
}

void JobDetailsTabController::setJob(Job* job)
{
    bool validJob = (bool) job;

    ui->name       ->setEnabled(validJob);
    ui->type       ->setEnabled(validJob);
    ui->local      ->setEnabled(validJob);
    ui->localButton->setEnabled(validJob);
    ui->remote     ->setEnabled(validJob);
    ui->autostart  ->setEnabled(validJob);
    ui->readOnly   ->setEnabled(validJob);
    ui->save       ->setEnabled(validJob);
    ui->remove     ->setEnabled(validJob);
    ui->openLog    ->setEnabled(validJob);

    if (!validJob) {
        clear();
        return;
    }

    m_currentJob = job;

    ui->name->setText(job->name());
    ui->type->setCurrentIndex(ui->type->findText(job->type()));
    ui->local->setText(job->local());
    ui->remote->setText(job->remote());
    ui->autostart->setChecked(job->autostart());
    ui->readOnly->setChecked(job->readOnly());

    // populate command preview
    updateCommandPreview();

    // form now mirrors the stored job, so there's nothing unsaved yet
    updateUnsavedIndicator();

    // show autocomplete
    delete m_remotesAutocompleter;
    m_remotesAutocompleter = RemotesAutocompleter::attach(
        ui->remote, m_appContext->rcloneProvider(), m_appContext->shared()->rclonePath());

    emit detailsOpened();
}

void JobDetailsTabController::clear()
{
    m_currentJob = nullptr;

    ui->name->clear();
    ui->local->clear();
    ui->remote->clear();
    ui->autostart->setChecked(false);
    ui->readOnly->setChecked(false);
    ui->command->clear();

    // no job loaded (or panel disabled) -> nothing to be unsaved
    updateUnsavedIndicator();
}

void JobDetailsTabController::onTypeChanged(int index)
{
    ui->readOnly->setEnabled(index == ui->type->findText("mount"));
}

void JobDetailsTabController::updateCommandPreview()
{
    if (!m_currentJob) {
        ui->command->clear();
        return;
    }

    const SharedSettings* shared = m_appContext->shared();

    RcloneCommandParams params;
    params.type      = ui->type->currentText();
    params.local     = ui->local->text();
    params.remote    = ui->remote->text();
    params.readOnly  = ui->readOnly->isChecked();
    params.swapSides = false;

    params.cacheMode          = shared->cacheMode();
    params.cacheMaxSize       = shared->cacheMaxSize();
    params.cacheMinFreeSpace  = shared->cacheMinFreeSpace();
    params.cacheMaxAge        = shared->cacheMaxAge();
    params.readChunkSize      = shared->readChunkSize();
    params.readChunkSizeLimit = shared->readChunkSizeLimit();
    params.bufferSize         = shared->bufferSize();
    params.transfers          = shared->transfers();
    params.checkers           = shared->checkers();
    params.links              = shared->links();

    QStringList command = m_appContext->rcloneProvider()->buildCommand(params);
    ui->command->setText(command.join(' '));
}

void JobDetailsTabController::updateUnsavedIndicator()
{
    if (!m_currentJob) {
        ui->unsaved->setVisible(false);
        return;
    }

    const bool dirty =
        ui->name->text()           != m_currentJob->name()      ||
        ui->type->currentText()    != m_currentJob->type()      ||
        ui->local->text()          != m_currentJob->local()     ||
        ui->remote->text()         != m_currentJob->remote()    ||
        ui->autostart->isChecked() != m_currentJob->autostart() ||
        ui->readOnly->isChecked()  != m_currentJob->readOnly();

    ui->unsaved->setVisible(dirty);
}

void JobDetailsTabController::onOpenLogClicked()
{
    if (!m_currentJob) return;

    if (!m_currentJob->openLogFile()) {
        Status::notify("Error opening job log file (run the job at least once first).", Status::Level::Error);
    }
}

void JobDetailsTabController::onSaveClicked()
{
    if (!m_currentJob) return;
    Job* job = m_currentJob;

    job->setName(ui->name->text());
    job->setType(ui->type->currentText());
    job->setLocal(ui->local->text());
    job->setRemote(ui->remote->text());
    job->setAutostart(ui->autostart->isChecked());
    job->setReadOnly(ui->readOnly->isChecked());

    updateCommandPreview();

    if (m_appContext->save()) {
        Status::notify("Job saved.", Status::Level::Success);
        updateUnsavedIndicator();
        emit closeRequested();
    } else {
        Status::notify("Error saving job.", Status::Level::Error);
    }
}

void JobDetailsTabController::onDeleteClicked()
{
    if (!m_currentJob) return;

    Job* toRemove = m_currentJob;
    m_currentJob = nullptr; // clear before deletion
    m_appContext->removeJob(toRemove);

    if (m_appContext->save()) {
        Status::notify("Job removed.", Status::Level::Success);
        setJob(nullptr);
        emit closeRequested();
    } else {
        Status::notify("Error removing job.", Status::Level::Error);
    }
}

void JobDetailsTabController::onLocalSelectClicked()
{
    QString path = QFileDialog::getExistingDirectory(
        this, "Select local folder or mount point", ui->local->text());
    if (!path.isEmpty()) {
        ui->local->setText(QDir::toNativeSeparators(path));
    }
}