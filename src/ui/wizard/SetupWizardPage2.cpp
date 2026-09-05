#include "SetupWizardPage2.h"

#include "src/common/AsyncRunner.h"
#include "src/core/AppContext.h"
#include "src/core/Status.h"

#include <QProcess>

SetupWizardPage2::SetupWizardPage2(AppContext* appContext, QWidget* parent)
    : QWizardPage(parent), m_appContext(appContext)
{
    ui.setupUi(this);

    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/open.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    ui.configFileButton->setIcon(icon);
    ui.configConsoleButton->setIcon(icon);

    // All rclone calls run on a background thread via RCloneConfigWorker,
    // driven for as long as this page exists (unlike RemotesLookupWorker,
    // which is a one-shot per-thread job) so refreshRemotes()/the config
    // file button can be triggered repeatedly without blocking the UI.
    // AsyncRunner moves the worker onto that thread and starts it; it's
    // parented to `this` so it (and the thread) is cleaned up automatically
    // when this page is destroyed.
    m_worker = new RCloneConfigWorker(m_appContext->rcloneProvider());
    m_runner = new AsyncRunner(m_worker, this);

    connect(this, &SetupWizardPage2::requestRemotes,        m_worker, &RCloneConfigWorker::fetchRemotes);
    connect(this, &SetupWizardPage2::requestOpenConfigFile, m_worker, &RCloneConfigWorker::openConfigFile);

    connect(m_worker, &RCloneConfigWorker::remotesReady, this, [this](const QStringList& remotesIn){
        QStringList remotes = remotesIn;
        for (QString& remote : remotes) {
            remote += ':';
        }
        ui.remotes->setText(remotes.join('\n'));
    });

    connect(m_worker, &RCloneConfigWorker::configFileOpened, this, [this](bool ok){
        if (!ok) {
            Status::instance().notify(
                tr("Could not locate or open the rclone config file."),
                Status::Level::Error);
        }
    });

    connect(ui.configFileButton, &QPushButton::clicked, this, [this](){
        emit requestOpenConfigFile(m_appContext->shared()->rclonePath());
    });

    connect(ui.configConsoleButton, &QPushButton::clicked, this, [this](){
        // RCloneProvider::openConfig() hands back the running QProcess (or
        // nullptr if it failed to start) so we can react when the user
        // closes the config console, instead of polling.
        QProcess* process = m_appContext->rcloneProvider()->openConfig(m_appContext->shared()->rclonePath(), m_appContext);
        if (!process) {
            Status::instance().notify(
                tr("Failed to launch 'rclone config'."), Status::Level::Error);
            return;
        }
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus exitStatus){
            // rclone config is now launched directly (no cmd.exe/start
            // wrapper), so this exit code genuinely reflects whether it
            // succeeded.
            if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                Status::instance().notify(
                    tr("'rclone config' did not complete successfully."), Status::Level::Warning);
            }
            refreshRemotes();
        });
    });
}

void SetupWizardPage2::initializePage(){
    refreshRemotes();
}

void SetupWizardPage2::refreshRemotes(){
    emit requestRemotes(m_appContext->shared()->rclonePath());
}
