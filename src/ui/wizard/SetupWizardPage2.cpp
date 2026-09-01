#include "SetupWizardPage2.h"
#include "src/core/Status.h"
#include "src/core/Manager.h"
#include <QProcess>

SetupWizardPage2::SetupWizardPage2(Manager* manager, QWidget* parent)
    : QWizardPage(parent), m_manager(manager)
{
    ui.setupUi(this);

    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/open.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    ui.configFileButton->setIcon(icon);
    ui.configConsoleButton->setIcon(icon);

    connect(ui.configFileButton, &QPushButton::clicked, this, [this](){
        if (!m_manager->openRcloneConfFile()) {
            Status::instance().notify(
                tr("Could not locate or open the rclone config file."),
                Status::Level::Error);
        }
    });

    connect(ui.configConsoleButton, &QPushButton::clicked, this, [this](){
        // Manager::openRcloneConfigProcess() hands back the running
        // QProcess (or nullptr if it failed to start) so we can react
        // when the user closes the config console, instead of polling.
        QProcess* process = m_manager->openRcloneConf();
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
    ui.remotes->setText(m_manager->listRCloneRemotes());
}
