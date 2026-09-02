#pragma once

#include "src/core/AppContext.h"
#include "src/providers/rclone/RemotesAutocompleter.h"

#include <QWidget>

class Job;

QT_BEGIN_NAMESPACE
namespace Ui { class JobDetailsTabController; }
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// JobDetailsTabController
// Owns the Job details tab: viewing/editing a single job's settings,
// opening its log file, saving, and deleting it.
// ---------------------------------------------------------------------------
class JobDetailsTabController : public QWidget
{
    Q_OBJECT

public:
    explicit JobDetailsTabController(AppContext* appContext, QWidget* parent = nullptr);
    ~JobDetailsTabController() override;

    // Shows job's details, or clears/disables the panel if job is nullptr.
    void setJob(Job* job);

signals:
    // Emitted once a job's details are populated and ready to be shown.
    void detailsOpened();

    // Emitted once the tab is done with this job (saved or deleted) and the
    // view should switch back to the jobs list.
    void closeRequested();

private slots:
    void onTypeChanged(int index);
    void onOpenLogClicked();
    void onSaveClicked();
    void onDeleteClicked();
    void onLocalSelectClicked();
    void updateCommandPreview();
    void updateUnsavedIndicator();

private:
    void clear();

    Ui::JobDetailsTabController* ui;
    AppContext* m_appContext;
    Job*     m_currentJob = nullptr;

    RemotesAutocompleter* m_remotesAutocompleter = nullptr;
};
