#pragma once

#include "src/ui/wizard/ui_SetupWizardPage1.h"

#include <QWizardPage>

class AppContext;
class QTimer;

// Page 1: checks for rclone / WinFsp, lets the user point at the
// rclone executable. Polls while visible; the poll stops as soon as
// both dependencies are found.
class SetupWizardPage1 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage1(AppContext* appContext, QWidget* parent = nullptr);

    bool isComplete() const override;

protected:
    void initializePage() override;
    void cleanupPage() override;

private:
    void refresh();

    Ui::SetupWizardPage1 ui;
    AppContext* m_appContext = nullptr;
    QTimer* m_timer = nullptr;
};
