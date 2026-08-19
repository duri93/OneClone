#include "SetupWizardPage1.h"
#include "ui_SetupWizardPage1.h"

#include <QFileDialog>
#include <QDialog>
#include <QDir>
#include <QLineEdit>
#include <QPushButton>

SetupWizardPage1::SetupWizardPage1(Manager* manager, QWidget *parent):QWizardPage(parent)
    , ui(new Ui::SetupWizardPage1){
    ui->setupUi(this);
    ui->rclonePath->setText(manager->shared()->rclonePath());

    m_manager = manager;
}

SetupWizardPage1::~SetupWizardPage1()
{
    m_timer->stop();
    delete ui;
}

void SetupWizardPage1::setup(){
    // rclone path
    ui->rclonePath->setText(m_manager->shared()->rclonePath());
    connect(ui->rclonePathButton, &QPushButton::clicked, this, &SetupWizardPage1::onRcloneSelectClicked);
    connect(ui->rclonePath, &QLineEdit::editingFinished, this, [this]() {
        m_manager->shared()->setRclonePath(QDir::toNativeSeparators(ui->rclonePath->text()));
    });

    // icons (check and update every second)
    m_timer = new QTimer();
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &SetupWizardPage1::isComplete);
    m_timer->start();
}
bool SetupWizardPage1::isComplete() const{
    // check
    bool rcloneOk = m_manager->isRcloneInstalled();
    bool winfspOk = m_manager->isWinFspInstalled();

    // update icons
    QPixmap okPixmap (":/resources/success.png");
    QPixmap noPixmap (":/resources/errored.png");

    ui->rcloneIcon->setPixmap(rcloneOk ? okPixmap : noPixmap);
    ui->winfspIcon->setPixmap(winfspOk ? okPixmap : noPixmap);

    // enable next button
    wizard()->button(QWizard::NextButton)->setEnabled(rcloneOk && winfspOk);
    return rcloneOk && winfspOk;
}

void SetupWizardPage1::onRcloneSelectClicked(){
    QString path = QFileDialog::getOpenFileName(
        this, "Select rclone.exe", ui->rclonePath->text(),
        "Executable (*.exe);;All files (*.*)");

    if (!path.isEmpty()) {
        ui->rclonePath->setText(QDir::toNativeSeparators(path));
        m_manager->shared()->setRclonePath(QDir::toNativeSeparators(path));
    }
}
