#include "SetupWizardPage2.h"

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

    connect(ui.configFileButton, &QPushButton::clicked, this, [this](){
        if (!m_appContext->rcloneProvider()->openConfigFile(m_appContext->shared()->rclonePath())) {
            Status::instance().notify(
                tr("Could not locate or open the rclone config file."),
                Status::Level::Error);
        }
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
                this, &SetupWizardPage2::refreshRemotes);
    });
}

void SetupWizardPage2::initializePage(){
    refreshRemotes();
}

void SetupWizardPage2::refreshRemotes(){
    QStringList remotes = m_appContext->rcloneProvider()->listRemotes(m_appContext->shared()->rclonePath());
    for (QString& remote : remotes) {
        remote += ':';
    }
    ui.remotes->setText(remotes.join('\n'));
}
