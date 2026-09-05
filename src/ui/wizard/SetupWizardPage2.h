#pragma once

#include "src/providers/rclone/RCloneConfigWorker.h"
#include "src/ui/wizard/ui_SetupWizardPage2.h"

#include <QWizardPage>

class AppContext;
class AsyncRunner;

// Page 2: shows configured rclone remotes, lets the user open the
// config console or the raw config file. The remotes list refreshes
// when the config console process exits, rather than on a timer.
// All rclone calls (listing remotes, opening the config file) run on a
// background thread via RCloneConfigWorker (kept alive for as long as this
// page exists via AsyncRunner) so the UI never blocks on a slow/hung
// rclone process.
class SetupWizardPage2 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage2(AppContext* appContext, QWidget* parent = nullptr);
    ~SetupWizardPage2() override = default;

protected:
    void initializePage() override;

signals:
    // Internal cross-thread requests to m_worker — connecting across
    // threads via signal/slot marshals the call onto the worker thread.
    void requestRemotes(const QString& rclonePath);
    void requestOpenConfigFile(const QString& rclonePath);

private:
    void refreshRemotes();

    Ui::SetupWizardPage2 ui;
    AppContext* m_appContext = nullptr;

    // m_runner owns the background thread m_worker lives on, and is
    // parented to `this` so it (and the thread) is torn down automatically
    // when this page is destroyed — see AsyncRunner.
    AsyncRunner* m_runner = nullptr;
    RCloneConfigWorker* m_worker = nullptr;
};
