#pragma once

#include <QWizardPage>
#include <QTimer>

namespace Ui {
class SetupWizardPage2;
}

class SetupWizardPage2 : public QWizardPage
{
    Q_OBJECT

public:
    explicit SetupWizardPage2(QWidget *parent = nullptr);
    ~SetupWizardPage2();

private:
    Ui::SetupWizardPage2 *ui;

    void listRemotes();

    QTimer* m_timer;
};