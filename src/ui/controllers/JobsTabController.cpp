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
    emit openDetailsRequested(job->id());
}

void JobsTabController::onJobMoved(const QString& id, int newIndex)
{
    m_appContext->moveJob(id, newIndex);

    populateJobList();

    if (!m_appContext->save()) {
        Status::notify("Warning: failed to save settings.", Status::Level::Error);
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
JobWidget* JobsTabController::findOrCreateJobWidget(Job* job)
{
    for (JobWidget*& w : m_jobWidgets)
        if (w->job() == job) return w;

    // not found — create fresh (same as onJobAdded)
    JobWidget* w = new JobWidget(job);
    w->setProperty("jobId", job->id());
    connect(w, &JobWidget::openDetailsRequested, this, &JobsTabController::openDetailsRequested);
    return w;
}

void JobsTabController::onJobAdded(Job* job)
{
    JobWidget* w = new JobWidget(job);

    connect(w, &JobWidget::openDetailsRequested, this, &JobsTabController::openDetailsRequested);

    m_jobWidgets.append(w);
    ui->list->layout()->addWidget(w);
}

void JobsTabController::onJobRemoved(const QString& jobId)
{
    for (int i = 0; i < m_jobWidgets.size(); ++i) {
        if (m_jobWidgets[i]->job()->id() == jobId) {
            delete m_jobWidgets[i];
            m_jobWidgets.removeAt(i);
            break;
        }
    }
}
