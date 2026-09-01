#include "SetupWizard.h"
#include "SetupWizardPage0.h"
#include "SetupWizardPage1.h"
#include "SetupWizardPage2.h"
#include "SetupWizardPage3.h"
#include "src/core/Manager.h"
#include <QWidget>


SetupWizard::SetupWizard(Manager* manager, QWidget* parent)
    : QObject(nullptr), m_manager(manager)
{
    m_wizard = new QWizard(parent);
    m_wizard->setWizardStyle(QWizard::ModernStyle);
    m_wizard->setWindowTitle(tr("OneClone setup wizard"));

    m_page0 = new SetupWizardPage0(m_wizard);
    m_page1 = new SetupWizardPage1(m_manager, m_wizard);
    m_page2 = new SetupWizardPage2(m_manager, m_wizard);
    m_page3 = new SetupWizardPage3(m_wizard);

    m_wizard->addPage(m_page0);
    m_wizard->addPage(m_page1);
    m_wizard->addPage(m_page2);
    m_wizard->addPage(m_page3);

    // Whenever the wizard is actually destroyed (via WA_DeleteOnClose
    // above, or for any other reason), tear this wrapper down too.
    connect(m_wizard, &QWizard::finished, this, &SetupWizard::setupFinished);
    connect(m_wizard, &QWizard::finished, this, &QObject::deleteLater);
}

SetupWizard::~SetupWizard(){
    if (m_wizard) {
        delete m_wizard;
    }
}

void SetupWizard::show(){
    if (m_wizard) {
        m_wizard->show();
    }
}
