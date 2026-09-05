#include "TrayController.h"

#include "src/common/Config.h"
#include "src/core/AppContext.h"
#include "src/core/Job.h"
#include "src/ui/JobIcons.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QWidget>

TrayController::TrayController(QWidget* window, AppContext* appContext, QObject* parent)
    : QObject(parent)
    , m_window(window)
    , m_appContext(appContext)
{
    m_trayIcon = new QSystemTrayIcon(this);

    // Placeholder icon — replace with your actual app icon
    m_trayIcon->setIcon(QIcon(":/favicon.svg"));
    m_trayIcon->setToolTip(Config::APP_NAME);

    m_trayMenu  = new QMenu(m_window);
    m_trayOpen  = m_trayMenu->addAction(tr("Open"));
    m_trayMenu->addSeparator();
    // Per-job actions are inserted here dynamically (see onMenuAboutToShow)
    m_trayMenu->addSeparator();
    m_trayClose = m_trayMenu->addAction(tr("Quit"));

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

    // Re-insert current per-job actions before lastSep. Built straight from
    // AppContext's job list — and JobIcons, the same cached icon rendering
    // JobWidget uses — rather than from the Jobs tab's JobWidgets, so the
    // tray doesn't need to know anything about how that tab displays jobs.
    for (Job* job : m_appContext->jobs()) {
        QPixmap icon = JobIcons::statusIcon(
            job->status(), !job->warnings().isEmpty(), Config::TRAY_JOB_ICON_SIZE);

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
        if (m_window->isVisible() && !m_window->isMinimized()) {
            m_window->hide();
        } else {
            m_window->show();
            m_window->setWindowState( (m_window->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            m_window->raise();
            m_window->activateWindow();
        }
    }
}
