#include "SetupWizard.h"

#include "src/core/AppContext.h"
#include "src/ui/wizard/SetupWizardPage0.h"
#include "src/ui/wizard/SetupWizardPage1.h"
#include "src/ui/wizard/SetupWizardPage2.h"
#include "src/ui/wizard/SetupWizardPage3.h"

#include <QWidget>

SetupWizard::SetupWizard(AppContext* appContext, QWidget* parent)
    : QObject(nullptr), m_appContext(appContext)
{
    m_wizard = new QWizard(parent);
    m_wizard->setWizardStyle(QWizard::ModernStyle);
    m_wizard->setWindowTitle(tr("OneClone setup wizard"));

    m_page0 = new SetupWizardPage0(m_wizard);
    m_page1 = new SetupWizardPage1(m_appContext, m_wizard);
    m_page2 = new SetupWizardPage2(m_appContext, m_wizard);
    m_page3 = new SetupWizardPage3(m_wizard);

    m_wizard->addPage(m_page0);
    m_wizard->addPage(m_page1);
    m_wizard->addPage(m_page2);
    m_wizard->addPage(m_page3);

    // When the wizard finishes, tear down this wrapper too: deleteLater()
    // destroys the SetupWizard object, whose destructor then deletes
    // m_wizard itself.
    connect(m_wizard, &QWizard::finished, this, &SetupWizard::setupFinished);
    connect(m_wizard, &QWizard::finished, this, &QObject::deleteLater);
    connect(m_wizard, &QWizard::finished, this, [this]() {m_appContext->save(); });
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
