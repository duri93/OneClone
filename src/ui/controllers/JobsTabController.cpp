#include "JobsTabController.h"

#include "src/core/Job.h"
#include "src/core/Status.h"
#include "src/ui/ui_JobsTabController.h"
#include "src/ui/widgets/JobListWidget.h"
#include "src/ui/widgets/JobWidget.h"

JobsTabController::JobsTabController(AppContext* appContext, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::JobsTabController)
    , m_appContext(appContext)
{
    ui->setupUi(this);

    connect(ui->add, &QPushButton::clicked, this, &JobsTabController::onAddClicked);
    connect(ui->list, &JobListWidget::jobMoved, this, &JobsTabController::onJobMoved);

    connect(m_appContext, &AppContext::added, this, &JobsTabController::onJobAdded);
    connect(m_appContext, &AppContext::removed, this, &JobsTabController::onJobRemoved);

    populateJobList();
}

JobsTabController::~JobsTabController()
{
    delete ui;
}

void JobsTabController::onAddClicked()
{
    Job* job = new Job(m_appContext->shared(), m_appContext->rcloneProvider());
    m_appContext->addJob(job);
    m_appContext->save();
    emit openDetailsRequested(job->id());
}

void JobsTabController::onJobMoved(const QString& id, int newIndex)
{
    m_appContext->moveJob(id, newIndex);

    populateJobList();

    if (!m_appContext->save()) {
        Status::notify(tr("Warning: failed to save settings."), Status::Level::Error);
    }
}

void JobsTabController::populateJobList()
{

    // detach currently-shown widgets
    QLayout* l = ui->list->layout();
    for (JobWidget*& w : m_jobWidgets) {
        l->removeWidget(w);
        w->hide();
    }

    // recreate list
    QVector<JobWidget*> ordered;
    ordered.reserve(m_appContext->jobs().size());
    for (Job* job : m_appContext->jobs()) {
        JobWidget* w = findOrCreateJobWidget(job);
        l->addWidget(w);
        w->show();
        ordered.append(w);
    }

    // only replace once rebuilt, so lookups above still worked
    m_jobWidgets = ordered;
}
JobWidget* JobsTabController::createJobWidget(Job* job)
{
    JobWidget* w = new JobWidget(job);
    w->setProperty("jobId", job->id());
    connect(w, &JobWidget::openDetailsRequested, this, &JobsTabController::openDetailsRequested);

    // Job deliberately doesn't call into the app-wide Status bus itself
    // (it's a model class, decoupled from that UI-layer broadcast channel)
    // — it emits notification() instead. This is the UI-boundary listener
    // that forwards those through to Status::notify(), for every job that
    // gets a widget (i.e. every job in the app).
    connect(job, &Job::notification, this, [](const QString& message, Status::Level level) {
        Status::notify(message, level);
    });

    return w;
}

JobWidget* JobsTabController::findOrCreateJobWidget(Job* job)
{
    for (JobWidget*& w : m_jobWidgets)
        if (w->job() == job) return w;

    // not found — create fresh
    return createJobWidget(job);
}

void JobsTabController::onJobAdded(Job* job)
{
    JobWidget* w = createJobWidget(job);

    m_jobWidgets.append(w);
    ui->list->layout()->addWidget(w);
}

void JobsTabController::onJobRemoved(const QString& jobId)
{
    // removeJob() emits removed() just before actually erasing the job, so
    // AppContext still has it (and thus a valid index for it) here — and
    // m_jobWidgets is always kept in the same order as m_appContext->jobs(),
    // so that index tells us exactly what to remove from our own list too.
    const int index = m_appContext->indexOfJob(jobId);
    if (index < 0 || index >= m_jobWidgets.size()) return;

    delete m_jobWidgets[index];
    m_jobWidgets.removeAt(index);
}
