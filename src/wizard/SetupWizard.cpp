#include "SetupWizard.h"

#include "SetupWizardPage1.h"
#include "SetupWizardPage2.h"
#include "SetupWizardPage3.h"

SetupWizard::SetupWizard(Manager* manager, QObject *parent)
    : QWizard{}
{

    QWizard wizard;
    wizard.addPage(new SetupWizardPage1(manager));
    wizard.addPage(new SetupWizardPage2(manager));
    wizard.addPage(new SetupWizardPage3());

    wizard.setWindowTitle("First setup wizard");
    wizard.show();
}
