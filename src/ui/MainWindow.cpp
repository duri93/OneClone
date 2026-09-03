#include "MainWindow.h"

#include "src/common/Config.h"
#include "src/common/UpdateManager.h"
#include "src/ui/ui_MainWindow.h"
#include "src/ui/wizard/SetupWizard.h"

#include <QCloseEvent>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QString>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){

    // ---- Set up window ----
    ui->setupUi(this);
    setWindowTitle(QString(Config::APP_NAME) + " " + Config::APP_VERSION);

    QSettings settings(Config::APP_AUTHOR, Config::APP_NAME);
    if(settings.contains("geometry")){
        restoreGeometry(settings.value("geometry").toByteArray());
    }else{
        moveWindowToBottomRight();
    }

    // ---- Load settings (generates defaults on first run) ----
    bool dryRun = !m_appContext.load();

    if (dryRun) {
        Status::notify("Could not load settings file — using defaults.", Status::Level::Warning);
    }

    // ---- Setup wizard, errors, warnings and messages ----
    checkDependencies();
    if(dryRun) openSetupWizard();

    ui->updateFrame->hide();

    connect(ui->errorRcloneClose, &QPushButton::clicked, ui->errorRcloneFrame, &QFrame::hide);
    connect(ui->errorWinfspClose, &QPushButton::clicked, ui->errorWinfspFrame, &QFrame::hide);
    connect(ui->updateClose,      &QPushButton::clicked, ui->updateFrame,      &QFrame::hide);

    // ---- Settings tab ----
    m_settingsTab = new SettingsTabController(&m_appContext, this);
    ui->tabSettings->layout()->addWidget(m_settingsTab);
    connect(m_settingsTab, &SettingsTabController::wizardRequested, this, &MainWindow::openSetupWizard);
    connect(m_settingsTab, &SettingsTabController::settingsSaved, this, &MainWindow::checkDependencies);
    connect(m_settingsTab, &SettingsTabController::settingsCancel, this, [this](){
        ui->tabWidget->setCurrentWidget(ui->tabJobs);
    });

    // ---- List tab ----
    m_jobsTab = new JobsTabController(&m_appContext, this);
    ui->tabJobs->layout()->addWidget(m_jobsTab);
    connect(m_jobsTab, &JobsTabController::openDetailsRequested, this, &MainWindow::onJobsOpenDetailsRequested);

    // ---- Details tab ----
    m_detailsTab = new JobDetailsTabController(&m_appContext, this);
    ui->tabDetails->layout()->addWidget(m_detailsTab);
    connect(m_detailsTab, &JobDetailsTabController::detailsOpened, this, [this](){
        ui->tabWidget->setCurrentWidget(ui->tabDetails);
    });
    connect(m_detailsTab, &JobDetailsTabController::closeRequested, this, [this](){
        ui->tabWidget->setCurrentWidget(ui->tabJobs);
    });

    // ---- Start on the list tab ----
    ui->tabWidget->setCurrentWidget(ui->tabJobs);

    // ---- Setup tray ----
    m_trayController = new TrayController(this, m_jobsTab, this);

    // ---- Status bar
    connect(&Status::instance(), &Status::statusMessage, this, &MainWindow::onStatusMessage);

    // ---- Updater ----
    auto *updater = new UpdateManager(Config::APP_AUTHOR, Config::APP_NAME, Config::APP_VERSION, this);

    connect(updater, &UpdateManager::updateReady, this, [this](const QString &v) {
        ui->updateLabel->setText(tr("Version %1 will be installed on next restart.").arg(v));
        ui->updateFrame->show();
    });

    connect(updater, &UpdateManager::updateFailed, this, [this](const QString &reason) {
        ui->updateLabel->setText(tr("%1").arg(reason));
        ui->updateFrame->show();
    });

    updater->checkForUpdates();
}

MainWindow::~MainWindow(){
    delete ui;
}
void MainWindow::closeEvent(QCloseEvent* event){
    if(!this->isMaximized()){
        QSettings settings(Config::APP_AUTHOR, Config::APP_NAME);
        settings.setValue("geometry", saveGeometry());
    }

    hide();               // hide window, keep app running
    event->ignore();      // don't propagate the close
}

// ---------------------------------------------------------------------------
// Setup wizard / dependency checks
// ---------------------------------------------------------------------------
void MainWindow::openSetupWizard(){
    m_wizard = new SetupWizard(&m_appContext, this);
    connect(m_wizard, &SetupWizard::setupFinished, this, [this](){
        ui->errorRcloneFrame->setVisible(!m_appContext.rcloneProvider()->isAvailable(m_appContext.shared()->rclonePath()));
        ui->errorWinfspFrame->setVisible(!m_appContext.mountBackendDetector()->isAvailable());
    });
    m_wizard->show();

}
void MainWindow::checkDependencies(){
    ui->errorRcloneFrame->setVisible(!m_appContext.rcloneProvider()->isAvailable(m_appContext.shared()->rclonePath()));
    ui->errorWinfspFrame->setVisible(!m_appContext.mountBackendDetector()->isAvailable());
}

// ---------------------------------------------------------------------------
// Tab routing / window geometry
// ---------------------------------------------------------------------------
void MainWindow::onJobsOpenDetailsRequested(const QString& id){
    m_detailsTab->setJob(m_appContext.getJob(id));
}

void MainWindow::moveWindowToBottomRight(){
    // position window in bottom-right corner
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect available = screen->availableGeometry(); // excludes taskbar
    QSize  size     = frameSize().isEmpty() ? sizeHint() : frameSize();

    int x = available.right()  - size.width()  - 6;
    int y = available.bottom() - size.height() - 38;
    move(x, y);
}
void MainWindow::activate(){
    show();
    setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
}

// ---------------------------------------------------------------------------
// Status bar
// ---------------------------------------------------------------------------
void MainWindow::onStatusMessage(const QString& message, Status::Level level){
    // Optional: prefix/style by severity. Simplest version just forwards text.
    switch (level) {
    case Status::Level::Error:
        statusBar()->setStyleSheet("color: #c0392b;");
        break;
    case Status::Level::Warning:
        statusBar()->setStyleSheet("color: #e67e22;");
        break;
    default:
        statusBar()->setStyleSheet("");
        break;
    }
    statusBar()->showMessage(message, Config::STATUS_DURATION);
}

