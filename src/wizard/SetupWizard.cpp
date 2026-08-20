#include "SetupWizard.h"
#include "SetupWizardPage1.h"
#include "SetupWizardPage2.h"
#include "SetupWizardPage3.h"
#include "../model/Manager.h"
#include <QWidget>


SetupWizard::SetupWizard(Manager* manager, QWidget* parent)
    : QObject(nullptr), m_manager(manager)
{
    m_wizard = new QWizard(parent);
    m_wizard->setWizardStyle(QWizard::ModernStyle);
    m_wizard->setWindowTitle(tr("OneClone setup wizard"));

    // Closing the wizard (Finish, Cancel, or [x]) will now actually
    // destroy it instead of just hiding it.
    m_wizard->setAttribute(Qt::WA_DeleteOnClose);

    m_page1 = new SetupWizardPage1(m_manager, m_wizard);
    m_page2 = new SetupWizardPage2(m_manager, m_wizard);
    m_page3 = new SetupWizardPage3(m_wizard);

    m_wizard->addPage(m_page1);
    m_wizard->addPage(m_page2);
    m_wizard->addPage(m_page3);

    // Whenever the wizard is actually destroyed (via WA_DeleteOnClose
    // above, or for any other reason), tear this wrapper down too.
    connect(m_wizard, &QObject::destroyed, this, &QObject::deleteLater);
}

SetupWizard::~SetupWizard(){
    // In the normal flow, the wizard has already deleted itself
    // (WA_DeleteOnClose) by the time we get here, so m_wizard is null.
    // This is just a safety net for the (unusual) case where the
    // SetupWizard wrapper is destroyed some other way while the
    // window is still open.
    if (m_wizard) {
        delete m_wizard;
    }
}

void SetupWizard::show(){
    if (m_wizard) {
        m_wizard->show();
    }
}
