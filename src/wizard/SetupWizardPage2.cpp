#include "SetupWizardPage2.h"

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
        m_manager->openRcloneConfFile();
    });

    connect(ui.configConsoleButton, &QPushButton::clicked, this, [this](){
        // Manager::openRcloneConfigProcess() hands back the running
        // QProcess (or nullptr if it failed to start) so we can react
        // when the user closes the config console, instead of polling.
        QProcess* process = m_manager->openRcloneConf();
        if (!process) {
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
