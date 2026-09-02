#include "TrayController.h"

#include "src/common/Config.h"
#include "src/ui/controllers/JobsTabController.h"
#include "src/ui/widgets/JobWidget.h"
#include "src/core/Job.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QWidget>

TrayController::TrayController(QWidget* window, JobsTabController* jobsTab, QObject* parent)
    : QObject(parent)
    , m_window(window)
    , m_jobsTab(jobsTab)
{
    m_trayIcon = new QSystemTrayIcon(this);

    // Placeholder icon — replace with your actual app icon
    m_trayIcon->setIcon(QIcon(":/favicon.svg"));
    m_trayIcon->setToolTip(Config::APP_NAME);

    m_trayMenu  = new QMenu(m_window);
    m_trayOpen  = m_trayMenu->addAction("Open");
    m_trayMenu->addSeparator();
    // Per-job actions are inserted here dynamically (see onMenuAboutToShow)
    m_trayMenu->addSeparator();
    m_trayClose = m_trayMenu->addAction("Quit");

    connect(m_trayOpen,  &QAction::triggered,         m_window, &QWidget::show);
    connect(m_trayClose, &QAction::triggered,         qApp,     &QApplication::quit);
    connect(m_trayMenu,  &QMenu::aboutToShow,         this, &TrayController::onMenuAboutToShow);
    connect(m_trayIcon,  &QSystemTrayIcon::activated, this, &TrayController::onActivated);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
}

void TrayController::onMenuAboutToShow()
{
    // Remove all actions between the first separator and the last separator
    // (i.e. the dynamically added per-job actions from last time)
    QList<QAction*> actions = m_trayMenu->actions();
    QAction* lastSep  = nullptr;
    bool inside = false;
    for (QAction*& a : actions) {
        if (a->isSeparator()) {
            lastSep = a;
            inside = !inside;
        } else if (inside) {
            m_trayMenu->removeAction(a);
            delete a;
        }
    }

    // Re-insert current per-job actions before lastSep
    for (JobWidget* jw : m_jobsTab->jobWidgets()) {
        Job* job = jw->job();
        QPixmap icon = QPixmap(jw->getStatusIcon());

        QAction* act = new QAction(icon, job->name(), m_trayMenu);

        // Toggle on click
        connect(act, &QAction::triggered, this, [job]() {
            job->toggle();
        });

        m_trayMenu->insertAction(lastSep, act);
    }
}

void TrayController::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (m_window->isVisible()) {
            m_window->hide();
        } else {
            m_window->show();
            m_window->raise();
            m_window->activateWindow();
        }
    }
}
