#pragma once
#include "ui_SetupWizardPage1.h"
#include <QWizardPage>

class Manager;
class QTimer;

// Page 1: checks for rclone / WinFsp, lets the user point at the
// rclone executable. Polls while visible; the poll stops as soon as
// both dependencies are found.
class SetupWizardPage1 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage1(Manager* manager, QWidget* parent = nullptr);

    bool isComplete() const override;

protected:
    void initializePage() override;
    void cleanupPage() override;

private:
    void refresh();

    Ui::SetupWizardPage1 ui;
    Manager* m_manager = nullptr;
    QTimer* m_timer = nullptr;
};
