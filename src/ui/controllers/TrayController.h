#pragma once

#include <QAction>
#include <QObject>
#include <QSystemTrayIcon>

class AppContext;

// ---------------------------------------------------------------------------
// TrayController
// Owns the system tray icon and its menu (open/quit, plus a live per-job
// toggle entry for each job). Reads jobs straight from AppContext and
// renders their status icons via JobIcons, rather than reaching into the
// Jobs tab's widgets — the tray menu shouldn't need to know that widgets
// are how the jobs list happens to be displayed.
// ---------------------------------------------------------------------------
class TrayController : public QObject{
    Q_OBJECT

public:
    explicit TrayController(QWidget* window, AppContext* appContext, QObject* parent);

signals:

private slots:
    void onMenuAboutToShow();
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QWidget*         m_window;
    AppContext*      m_appContext;
    QSystemTrayIcon* m_trayIcon;
    QMenu*           m_trayMenu;

    QAction*         m_trayOpen;
    QAction*         m_trayClose;

};
