#pragma once

#include "src/core/AppContext.h"

#include <QVector>
#include <QWidget>

class JobWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class JobsTabController; }
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// JobsTabController
// Owns the Jobs list tab: the reorderable list of JobWidgets and the "Add"
// button, kept in sync with AppContext's job list.
// ---------------------------------------------------------------------------
class JobsTabController : public QWidget
{
    Q_OBJECT

public:
    explicit JobsTabController(AppContext* appContext, QWidget* parent = nullptr);
    ~JobsTabController() override;

signals:
    // Emitted when the user asks to open a job's details (add, click, or
    // double-click on a job widget).
    void openDetailsRequested(const QString& id);

private slots:
    void onAddClicked();
    void onJobMoved(const QString& id, int newIndex);
    void onJobAdded(Job* job);
    void onJobRemoved(const QString& jobId);

private:
    void populateJobList();
    JobWidget* findOrCreateJobWidget(Job* job);
    JobWidget* createJobWidget(Job* job);

    Ui::JobsTabController* ui;
    AppContext* m_appContext;
    QVector<JobWidget*> m_jobWidgets;
};
