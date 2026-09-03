#pragma once

#include "src/providers/rclone/RCloneConfigWorker.h"
#include "src/ui/wizard/ui_SetupWizardPage2.h"

#include <QThread>
#include <QWizardPage>

class AppContext;

// Page 2: shows configured rclone remotes, lets the user open the
// config console or the raw config file. The remotes list refreshes
// when the config console process exits, rather than on a timer.
// All rclone calls (listing remotes, opening the config file) run on a
// background thread via RCloneConfigWorker so the UI never blocks on a
// slow/hung rclone process.
class SetupWizardPage2 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage2(AppContext* appContext, QWidget* parent = nullptr);
    ~SetupWizardPage2() override;

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

    QThread m_workerThread;
    RCloneConfigWorker* m_worker = nullptr;
};
