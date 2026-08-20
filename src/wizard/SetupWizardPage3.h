#pragma once

#include <QWizardPage>

#include "ui_SetupWizardPage3.h"

// Page 3: intentionally empty for now.
class SetupWizardPage3 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage3(QWidget* parent = nullptr);

private:
    Ui::SetupWizardPage3 ui;
};
