#include "SetupWizardPage0.h"

#include <QSvgRenderer>
#include <QSvgWidget>

SetupWizardPage0::SetupWizardPage0(QWidget* parent) : QWizardPage(parent){
    ui.setupUi(this);

    QSvgWidget *svgLogo = new QSvgWidget(":/logo.svg", this);
    svgLogo->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
    svgLogo->setMinimumHeight(160);
    svgLogo->setContentsMargins(64, 64, 64, 64);

    ui.verticalLayout->replaceWidget(ui.logo, svgLogo);
    delete ui.logo;
}
