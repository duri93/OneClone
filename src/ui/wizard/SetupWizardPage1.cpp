#include "SetupWizardPage1.h"

#include "src/common/LocalPathAutocompleter.h"
#include "src/core/AppContext.h"

#include <QDir>
#include <QFileDialog>
#include <QTimer>

SetupWizardPage1::SetupWizardPage1(AppContext* appContext, QWidget* parent)
    : QWizardPage(parent), m_appContext(appContext), m_timer(new QTimer(this))
{
    ui.setupUi(this);
    ui.rclonePath->setText(m_appContext->shared()->rclonePath());

    LocalPathAutocompleter::attach(ui.rclonePath,
                                   LocalPathAutocompleter::Mode::FoldersAndFiles,
                                   {"rclone.exe"});

    connect(ui.rclonePathButton, &QToolButton::clicked, this, [this](){
        QString path = QFileDialog::getOpenFileName(
            this, "Select rclone.exe", ui.rclonePath->text(),
            "Executable (*.exe);;All files (*.*)");

        if (!path.isEmpty()) {
            ui.rclonePath->setText(QDir::toNativeSeparators(path));
            m_appContext->shared()->setRclonePath(QDir::toNativeSeparators(path));
        }
    });
    connect(ui.rclonePath, &QLineEdit::editingFinished, this, [this](){
        m_appContext->shared()->setRclonePath(QDir::toNativeSeparators(ui.rclonePath->text()));
    });

    m_timer->setInterval(2000);
    connect(m_timer, &QTimer::timeout, this, &SetupWizardPage1::refresh);
}

void SetupWizardPage1::initializePage(){
    refresh();
    // no point polling if we already know both dependencies are there
    if (!isComplete()) {
        m_timer->start();
    }
}

void SetupWizardPage1::cleanupPage(){
    // stop polling as soon as the user navigates away from this page
    m_timer->stop();
}

bool SetupWizardPage1::isComplete() const {
    return m_appContext->rcloneProvider()->isAvailable(m_appContext->shared()->rclonePath()) && m_appContext->mountBackendDetector()->isAvailable();
}

void SetupWizardPage1::refresh(){
    static const QPixmap okPixmap(QString::fromUtf8(":/icons/success.svg"));
    static const QPixmap noPixmap(QString::fromUtf8(":/icons/errored.svg"));

    bool rcloneOk = m_appContext->rcloneProvider()->isAvailable(m_appContext->shared()->rclonePath());
    ui.rcloneStatus->setText(rcloneOk ? tr("Found") : tr("Not found"));
    ui.rcloneIcon->setPixmap(rcloneOk ? okPixmap : noPixmap);

    bool winfspOk = m_appContext->mountBackendDetector()->isAvailable();
    ui.winfspStatus->setText(winfspOk ? tr("Found") : tr("Not found"));
    ui.winfspIcon->setPixmap(winfspOk ? okPixmap : noPixmap);

    if (rcloneOk && winfspOk) {
        m_timer->stop();
    }

    // lets QWizard re-evaluate isComplete() and enable/disable Next
    emit completeChanged();
}
