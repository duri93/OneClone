#pragma once
#include "ui_SetupWizardPage2.h"
#include <QWizardPage>

class Manager;

// Page 2: shows configured rclone remotes, lets the user open the
// config console or the raw config file. The remotes list refreshes
// when the config console process exits, rather than on a timer.
class SetupWizardPage2 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage2(Manager* manager, QWidget* parent = nullptr);

protected:
    void initializePage() override;

private:
    void refreshRemotes();

    Ui::SetupWizardPage2 ui;
    Manager* m_manager = nullptr;
};
