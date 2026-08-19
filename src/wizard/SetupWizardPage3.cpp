#include "SetupWizardPage3.h"
#include "ui_SetupWizardPage3.h"

SetupWizardPage3::SetupWizardPage3(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::SetupWizardPage3)
{
    ui->setupUi(this);
    this->setTitle("3. Quick guide");
}

SetupWizardPage3::~SetupWizardPage3()
{
    delete ui;
}
