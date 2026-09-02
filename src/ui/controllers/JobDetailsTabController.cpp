#include "JobDetailsTabController.h"

#include "src/common/Config.h"
#include "src/common/LocalPathAutocompleter.h"
#include "src/core/Job.h"
#include "src/core/Status.h"
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

    // populate output log
    ui->command->setText(job->getCommand(false).join(' '));

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
}

void JobDetailsTabController::onTypeChanged(int index)
{
    ui->readOnly->setEnabled(index == ui->type->findText("mount"));
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

    ui->command->setText(job->getCommand(false).join(' '));

    if (m_appContext->save()) {
        Status::notify("Job saved.", Status::Level::Success);
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
