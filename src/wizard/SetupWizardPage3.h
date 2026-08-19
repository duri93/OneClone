#ifndef SETUPWIZARDPAGE3_H
#define SETUPWIZARDPAGE3_H

#include <QWizardPage>

namespace Ui {
class SetupWizardPage3;
}

class SetupWizardPage3 : public QWizardPage
{
    Q_OBJECT

public:
    explicit SetupWizardPage3(QWidget *parent = nullptr);
    ~SetupWizardPage3();

private:
    Ui::SetupWizardPage3 *ui;
};

#endif // SETUPWIZARDPAGE3_H
