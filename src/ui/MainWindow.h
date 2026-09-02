#pragma once

#include "src/core/AppContext.h"
#include "src/core/Status.h"
#include "src/ui/controllers/JobDetailsTabController.h"
#include "src/ui/controllers/JobsTabController.h"
#include "src/ui/controllers/SettingsTabController.h"
#include "src/ui/controllers/TrayController.h"

#include <QMainWindow>

class QString;
class SetupWizard;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// MainWindow
// Top-level window: hosts the tab controllers, tray icon, and setup wizard,
// and wires them to the shared AppContext.
// Follows a simple signals-&-slots architecture — no extra MVP layer.
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void activate();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Jobs list tab
    void onJobsOpenDetailsRequested(const QString& id);

    // Status bar
    void onStatusMessage(const QString& message, Status::Level level);

private:
    // Settings
    void openSetupWizard();
    void checkDependencies();

    // Window management
    void moveWindowToBottomRight();

    // Properties
    Ui::MainWindow*  ui;
    AppContext  m_appContext;

    SettingsTabController*    m_settingsTab = nullptr;
    JobsTabController*        m_jobsTab     = nullptr;
    JobDetailsTabController*  m_detailsTab  = nullptr;
    TrayController*           m_trayController = nullptr;

private:
    // setup wizard
    SetupWizard* m_wizard = nullptr;
};
