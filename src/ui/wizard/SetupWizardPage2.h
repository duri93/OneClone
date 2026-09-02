#pragma once

#include "src/ui/wizard/ui_SetupWizardPage2.h"

#include <QWizardPage>

class AppContext;

// Page 2: shows configured rclone remotes, lets the user open the
// config console or the raw config file. The remotes list refreshes
// when the config console process exits, rather than on a timer.
class SetupWizardPage2 : public QWizardPage {
    Q_OBJECT
public:
    explicit SetupWizardPage2(AppContext* appContext, QWidget* parent = nullptr);

protected:
    void initializePage() override;

private:
    void refreshRemotes();

    Ui::SetupWizardPage2 ui;
    AppContext* m_appContext = nullptr;
};
