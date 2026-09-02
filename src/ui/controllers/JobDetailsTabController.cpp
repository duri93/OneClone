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

    ui->detailsOutput->document()->setMaximumBlockCount(Config::MAX_OUTPUT_LINES);
    LocalPathAutocompleter::attach(ui->detailsLocal);

    connect(ui->detailsOpenLog,     &QPushButton::clicked, this, &JobDetailsTabController::onOpenLogClicked);
    connect(ui->detailsSave,        &QPushButton::clicked, this, &JobDetailsTabController::onSaveClicked);
    connect(ui->detailsDelete,      &QPushButton::clicked, this, &JobDetailsTabController::onDeleteClicked);
    connect(ui->detailsLocalButton, &QToolButton::clicked, this, &JobDetailsTabController::onLocalSelectClicked);

    // FIXME: onTypeChanged() is never wired to detailsType's signal in the
    // original code, so detailsReadOnly's enabled state never updates when
    // the job type changes. Carried over as-is; connect
    // ui->detailsType::currentIndexChanged here once that's confirmed to be
    // intentional to fix.

    clear();
}

JobDetailsTabController::~JobDetailsTabController()
{
    delete ui;
}

void JobDetailsTabController::setJob(Job* job)
{
    bool validJob = (bool) job;

    ui->detailsName     ->setEnabled(validJob);
    ui->detailsType     ->setEnabled(validJob);
    ui->detailsLocal    ->setEnabled(validJob);
    ui->detailsRemote   ->setEnabled(validJob);
    ui->detailsAutostart->setEnabled(validJob);
    ui->detailsReadOnly ->setEnabled(validJob);
    ui->detailsSave     ->setEnabled(validJob);
    ui->detailsDelete   ->setEnabled(validJob);
    ui->detailsOpenLog  ->setEnabled(validJob);

    if (!validJob) {
        clear();
        return;
    }

    m_currentJob = job;

    ui->detailsName->setText(job->name());
    ui->detailsType->setCurrentIndex(ui->detailsType->findText(job->type()));
    ui->detailsLocal->setText(job->local());
    ui->detailsRemote->setText(job->remote());
    ui->detailsAutostart->setChecked(job->autostart());
    ui->detailsReadOnly->setChecked(job->readOnly());

    // populate output log
    ui->detailsOutput->setText(job->getCommand(false).join(' '));

    // show autocomplete
    delete m_remotesAutocompleter;
    m_remotesAutocompleter = RemotesAutocompleter::attach(
        ui->detailsRemote, m_appContext->rcloneProvider(), m_appContext->shared()->rclonePath());

    emit detailsOpened();
}

void JobDetailsTabController::clear()
{
    m_currentJob = nullptr;

    ui->detailsName->clear();
    ui->detailsLocal->clear();
    ui->detailsRemote->clear();
    ui->detailsAutostart->setChecked(false);
    ui->detailsReadOnly->setChecked(false);
    ui->detailsOutput->clear();
}

void JobDetailsTabController::onTypeChanged(int index)
{
    ui->detailsReadOnly->setEnabled(index == ui->detailsType->findText("mount"));
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

    job->setName(ui->detailsName->text());
    job->setType(ui->detailsType->currentText());
    job->setLocal(ui->detailsLocal->text());
    job->setRemote(ui->detailsRemote->text());
    job->setAutostart(ui->detailsAutostart->isChecked());
    job->setReadOnly(ui->detailsReadOnly->isChecked());

    ui->detailsOutput->setText(job->getCommand(false).join(' '));

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
        this, "Select local folder or mount point", ui->detailsLocal->text());
    if (!path.isEmpty()) {
        ui->detailsLocal->setText(QDir::toNativeSeparators(path));
    }
}
