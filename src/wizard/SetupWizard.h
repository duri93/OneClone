#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include "..\model\Manager.h"
#include <QWizard>

class SetupWizard : public QWizard{
    Q_OBJECT
public:
    explicit SetupWizard(Manager *manager, QObject *parent = nullptr);

signals:
};

#endif // SETUPWIZARD_H
