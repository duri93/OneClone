#pragma once

#include <QAction>
#include <QObject>
#include <QSystemTrayIcon>

class JobsTabController;

class TrayController : public QObject{
    Q_OBJECT

public:
    explicit TrayController(QWidget* window, JobsTabController* jobsTab, QObject* parent);

signals:

private slots:
    void onMenuAboutToShow();
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QWidget*           m_window;
    JobsTabController* m_jobsTab;
    QSystemTrayIcon*   m_trayIcon;
    QMenu*             m_trayMenu;

    QAction*           m_trayOpen;
    QAction*           m_trayClose;

};
