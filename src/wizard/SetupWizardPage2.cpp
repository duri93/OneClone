#include "SetupWizardPage2.h"
#include "ui_SetupWizardPage2.h"

SetupWizardPage2::SetupWizardPage2(Manager* manager, QWidget *parent) : QWizardPage(parent)
    , ui(new Ui::SetupWizardPage2)
{
    ui->setupUi(this);
    this->setTitle("2. Setup RClone remote connection");

    m_manager = manager;

    // buttons
    connect(ui->configConsoleButton, &QPushButton::clicked, this, [this](){m_manager->openRcloneConf();});
    connect(ui->configFileButton, &QPushButton::clicked, this, [this](){m_manager->openRcloneConfFile();});

    // remotes (check and update every second)
    m_timer = new QTimer();
    m_timer->setInterval(2000);
    connect(m_timer, &QTimer::timeout, this, &SetupWizardPage2::listRemotes);
    m_timer->start();
}

SetupWizardPage2::~SetupWizardPage2()
{
    m_timer->stop();
    delete ui;
}

void SetupWizardPage2::listRemotes(){
    ui->remotes->setText(m_manager->listRCloneRemotes());
}
