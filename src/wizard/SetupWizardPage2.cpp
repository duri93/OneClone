#include "SetupWizardPage2.h"
#include "ui_SetupWizardPage2.h"

SetupWizardPage2::SetupWizardPage2(QWidget *parent) : QWizardPage(parent)
    , ui(new Ui::SetupWizardPage2)
{
    ui->setupUi(this);


    // remotes (check and update every second)
    m_timer = new QTimer();
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &SetupWizardPage2::listRemotes);
    m_timer->start();
}

SetupWizardPage2::~SetupWizardPage2()
{
    m_timer->stop();
    delete ui;
}

void SetupWizardPage2::listRemotes(){


}
