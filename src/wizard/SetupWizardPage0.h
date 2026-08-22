#pragma once
#include "ui_SetupWizardPage0.h"
#include <QWizardPage>

// Page 1: checks for rclone / WinFsp, lets the user point at the
// rclone executable. Polls while visible; the poll stops as soon as
// both dependencies are found.
class SetupWizardPage0 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage0(QWidget* parent = nullptr);

private:
    Ui::SetupWizardPage0 ui;
};
