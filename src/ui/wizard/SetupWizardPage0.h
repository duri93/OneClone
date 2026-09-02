#pragma once

#include "src/ui/wizard/ui_SetupWizardPage0.h"

#include <QWizardPage>

// Page 0: welcome/intro page with the app logo.
class SetupWizardPage0 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage0(QWidget* parent = nullptr);

private:
    Ui::SetupWizardPage0 ui;
};
