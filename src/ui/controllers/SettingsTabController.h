#pragma once

#include "src/core/AppContext.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsTabController; }
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// SettingsTabController
// Owns the Settings tab: rclone path/autostart configuration, advanced VFS
// tuning, and the "open rclone config" / "run setup wizard" actions.
// ---------------------------------------------------------------------------
class SettingsTabController : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsTabController(AppContext* appContext, QWidget* parent = nullptr);
    ~SettingsTabController() override;

signals:
    // Emitted when the user asks to (re)run the setup wizard.
    void wizardRequested();

    // Emitted after settings are saved, so dependent UI (e.g. the
    // rclone/WinFsp error banners) can refresh.
    void settingsSaved();

private slots:
    void onSave();
    void onRcloneSelectClicked();
    void onAdvancedToggled();
    void onRcloneConfClicked();

private:
    void loadFromSettings();
    void saveToSettings();

    Ui::SettingsTabController* ui;
    AppContext* m_appContext;
};
