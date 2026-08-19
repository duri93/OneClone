#pragma once

#include "../model/Manager.h"

#include <QWizardPage>
#include <QTimer>

namespace Ui {
class SetupWizardPage1;
}

class SetupWizardPage1 : public QWizardPage
{
    Q_OBJECT

public:
    explicit SetupWizardPage1(Manager* manager, QWidget *parent = nullptr);
    ~SetupWizardPage1();

    void setup();

private:
    Ui::SetupWizardPage1 *ui;

    void onRcloneSelectClicked();
    bool isComplete() const;

    QTimer* m_timer;
    static Manager* m_manager;
    bool completed = false;
};
