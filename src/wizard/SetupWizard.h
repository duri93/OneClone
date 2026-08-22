#pragma once
#include <QObject>
#include <QWizard>
#include <QPointer>

class Manager;
class QWizard;
class SetupWizardPage0;
class SetupWizardPage1;
class SetupWizardPage2;
class SetupWizardPage3;

// Owns and drives a QWizard, without being one itself. This lets us
// manage the wizard's lifetime explicitly (see the .cpp) instead of
// relying on the wizard object outliving whoever created it.
class SetupWizard : public QObject {
    Q_OBJECT
public:
    explicit SetupWizard(Manager* manager, QWidget* parent = nullptr);
    ~SetupWizard();

    void show();

private:
    // QPointer so we can safely tell, in the destructor, whether the
    // wizard has already deleted itself (WA_DeleteOnClose) or not.
    QPointer<QWizard> m_wizard;

    // Owned by m_wizard via QWizard::addPage(); kept here only for
    // reference, never deleted manually.
    SetupWizardPage0* m_page0 = nullptr;
    SetupWizardPage1* m_page1 = nullptr;
    SetupWizardPage2* m_page2 = nullptr;
    SetupWizardPage3* m_page3 = nullptr;

    Manager* m_manager = nullptr;
};
