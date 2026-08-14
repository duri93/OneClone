#include "src/view/ui_MainWindow.h"
#include "MainWindow.h"
#include "JobListWidget.h"

#include "../model/Config.h"
#include "../model/UpdateManager.h"

#include <QFileDialog>
#include <QCloseEvent>
#include <QUuid>
#include <QListView>
#include <QSettings>
#include <QPainter>
#include <QDir>
#include <QRect>
#include <QSize>
#include <QScreen>
#include <QStandardPaths>
#include <QProcess>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
    setWindowTitle(QString(Config::APP_NAME) + " " + Config::APP_VERSION);
    moveWindowToBottomRight();
    ui->updateFrame->hide();

    // ---- Load settings (generates defaults on first run) ----
    connect(&m_manager, &Manager::added, this, &MainWindow::onJobAdded);
    connect(&m_manager, &Manager::removed, this, &MainWindow::onJobRemoved);

    if (!m_manager.load()) {
        statusBar()->showMessage("Could not load settings file — using defaults.", Config::STATUS_DURATION);
    }

    // ---- Errors ----
    ui->errorRcloneFrame->setVisible(!isRcloneInstalled());
    ui->errorWinfspFrame->setVisible(!isWinFspInstalled());

    // ---- Settings tab ----
    ui->settingsAdvancedScrollarea->hide();
    loadSettingsToUi();
    connect(ui->settingsAdvanced,     &QCheckBox::checkStateChanged, this, &MainWindow::onSettingsAdvanced);
    connect(ui->settingsSave,         &QPushButton::clicked, this, &MainWindow::onSettingsSave);
    connect(ui->settingsRcloneButton, &QToolButton::clicked, this, &MainWindow::onRcloneSelectClicked);
    connect(ui->settingsRcloneConf,   &QPushButton::clicked, this, &MainWindow::onRcloneConfClicked);

    // ---- List tab ----
    // populated when adding jobs
    connect(ui->jobsAdd, &QPushButton::clicked, this, &MainWindow::onAddJobClicked);
    connect(ui->jobsList, &JobListWidget::jobMoved, this, &MainWindow::onJobMoved);

    // ---- Details tab ----
    ui->detailsOutput->document()->setMaximumBlockCount(Config::MAX_OUTPUT_LINES);

    connect(ui->detailsSave,   &QPushButton::clicked, this, &MainWindow::onDetailsSave);
    connect(ui->detailsDelete, &QPushButton::clicked, this, &MainWindow::onDetailsDelete);
    connect(ui->detailsLocalButton,    &QToolButton::clicked, this, &MainWindow::onLocalSelectClicked);

    // ---- Start on the list tab ----
    clearDetails();
    openDetails(nullptr);
    ui->tabWidget->setCurrentWidget(ui->tabJobs);

    // ---- setup tray ----
    setupTray();

    // ---- updater ----
    auto *updater = new UpdateManager(Config::APP_AUTHOR, Config::APP_NAME, Config::APP_VERSION, this);

    connect(updater, &UpdateManager::updateReady, this, [this](const QString &v) {
        ui->updateLabel->setText(tr("Version %1 will be installed on next restart.").arg(v));
        ui->updateFrame->show();
    });

    connect(updater, &UpdateManager::updateFailed, this, [this](const QString &reason) {
        ui->updateLabel->setText(tr("Update failed: ").arg(reason));
        ui->updateFrame->show();
    });

    updater->checkForUpdates();
}

MainWindow::~MainWindow(){
    delete ui;
}
void MainWindow::closeEvent(QCloseEvent* event){
    hide();               // hide window, keep app running
    event->ignore();      // don't propagate the close
}

// ---------------------------------------------------------------------------
// Settings tab
// ---------------------------------------------------------------------------
void MainWindow::loadSettingsToUi(){
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    bool isRegistered = reg.contains(Config::APP_ID);

    const SharedSettings* s = m_manager.shared();
    ui->settingsRclone            ->setText   (s->rclonePath());
    ui->settingsAdvanced          ->setChecked(s->advanced());
    ui->settingsBufferSize        ->setValue  (s->bufferSize());
    ui->settingsCacheMaxSize      ->setValue  (s->cacheMaxSize());
    ui->settingsCacheMinFreeSpace ->setValue  (s->cacheMinFreeSpace());
    ui->settingsCacheMaxAge       ->setValue  (s->cacheMaxAge());
    ui->settingsReadChunkSize     ->setValue  (s->readChunkSize());
    ui->settingsReadChunkSizeLimit->setValue  (s->readChunkSizeLimit());
    ui->settingsTransfers         ->setValue  (s->transfers());
    ui->settingsCheckers          ->setValue  (s->checkers());
    ui->settingsLinks             ->setChecked(s->links());
    ui->settingsAutostart         ->setChecked(isRegistered);

    // settingsCacheMode combobox: find matching text
    int idx = ui->settingsCacheMode->findText(s->cacheMode());
    if (idx >= 0) ui->settingsCacheMode->setCurrentIndex(idx);
}
void MainWindow::saveUiToSettings(){
    SharedSettings* s = m_manager.shared();
    s->setRclonePath(ui->settingsRclone->text());
    s->setAdvanced(ui->settingsAdvanced->isChecked());
    s->setCacheMode(ui->settingsCacheMode->currentText());
    s->setCacheMaxSize(ui->settingsCacheMaxSize->value());
    s->setCacheMinFreeSpace(ui->settingsCacheMinFreeSpace->value());
    s->setCacheMaxAge(ui->settingsCacheMaxAge->value());
    s->setReadChunkSize(ui->settingsReadChunkSize->value());
    s->setReadChunkSizeLimit(ui->settingsReadChunkSizeLimit->value());
    s->setBufferSize(ui->settingsBufferSize->value());
    s->setTransfers(ui->settingsTransfers->value());
    s->setCheckers(ui->settingsCheckers->value());
    s->setLinks(ui->settingsLinks->isChecked());

    // register or unregister startup
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if(ui->settingsAutostart->isChecked()){
        reg.setValue(Config::APP_ID, QCoreApplication::applicationFilePath().replace('/', '\\') + " --tray");
    }else{
        reg.remove(Config::APP_ID);
    }

    // check rclone again
    ui->errorRcloneFrame->setVisible(!isRcloneInstalled());
}

void MainWindow::onSettingsSave(){
    saveUiToSettings();

    if(m_manager.save()){
        statusBar()->showMessage("Settings saved.", Config::STATUS_DURATION);
    }else{
        statusBar()->showMessage("Error saving settings.", Config::STATUS_DURATION);
    }
}
void MainWindow::onRcloneSelectClicked(){
    QString path = QFileDialog::getOpenFileName(
        this, "Select rclone.exe", ui->settingsRclone->text(),
        "Executable (*.exe);;All files (*.*)");
    if (!path.isEmpty()) {
        ui->settingsRclone->setText(QDir::toNativeSeparators(path));
    }
}
void MainWindow::onSettingsAdvanced(){
    if(ui->settingsAdvanced->isChecked()){
        ui->settingsAdvancedScrollarea->show();
    }else{
        ui->settingsAdvancedScrollarea->hide();
    }
}

bool startDetachedClean(const QString &program, const QStringList &arguments){
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // Strip Qt Creator / Qt kit injected paths that cause ABI mismatches
    // in system-installed Qt apps like konsole.
    env.remove("LD_LIBRARY_PATH");
    env.remove("LD_PRELOAD");
    env.remove("QT_PLUGIN_PATH");
    env.remove("QML2_IMPORT_PATH");
    env.remove("QML_IMPORT_PATH");

    process.setProgram(program);
    process.setArguments(arguments);
    process.setProcessEnvironment(env);

    qint64 pid = 0;
    return process.startDetached(&pid);
}
bool MainWindow::onRcloneConfClicked(){
    QString rclonePath = m_manager.shared()->rclonePath();

    #if defined(Q_OS_WIN)
        QString program = "cmd.exe";
        QStringList arguments;
        arguments << "/c" << "start" << "rclone config" << "/wait" << rclonePath << "config";
        return QProcess::startDetached(program, arguments);
    #elif defined(Q_OS_LINUX)
        // set up terminals lookup table
        QString shellCmd = QString("'%1' config; exit").arg(rclonePath);

        struct TerminalCmd { QString exe; QStringList args; };
        const QList<TerminalCmd> terminals = {
            { "x-terminal-emulator", { "-e", "bash", "-c", shellCmd } }, // Debian/Ubuntu default alias
            { "gnome-terminal",      { "--", "bash", "-c", shellCmd } },
            { "konsole",             { "-e", "bash", "-c", shellCmd } },
            { "xfce4-terminal",      { "-x", "bash", "-c", shellCmd } },
            { "xterm",               { "-e", "bash", "-c", shellCmd } },
        };

        // start forst found terminal and run rclone config
        for (const auto &term : terminals) {
            if (!QStandardPaths::findExecutable(term.exe).isEmpty())
                return startDetachedClean(term.exe, term.args);
        }

        statusBar()->showMessage("No suitable terminal emulator found on this system.", Config::STATUS_DURATION);
        return false;
    #else
        statusBar()->showMessage("No suitable terminal emulator found on this system.", Config::STATUS_DURATION);
        return false;
    #endif
}


// ---------------------------------------------------------------------------
// Jobs list tab
// ---------------------------------------------------------------------------
void MainWindow::onAddJobClicked(){
    Job* job = new Job(m_manager.shared());
    m_manager.addJob(job);
    openDetails(job);
}
void MainWindow::onJobMoved(const QString& id, int newIndex){
    m_manager.moveJob(id, newIndex);

    // rebuild the visual order from m_jobs, which is now authoritative
    QLayout* l = ui->jobsList->layout();
    for (JobWidget*& w : m_jobWidgets){
        l->removeWidget(w);
        w->hide();
    }

    m_jobWidgets.clear();
    for (Job* job : m_manager.jobs()) {   // see note below
        JobWidget* w = findOrCreateJobWidget(job);
        l->addWidget(w);
        w->show();
        m_jobWidgets.append(w);
    }

    if (!m_manager.save())
        statusBar()->showMessage("Warning: failed to save settings.", Config::STATUS_DURATION);
}
JobWidget* MainWindow::findOrCreateJobWidget(Job* job){
    for (JobWidget*& w : m_jobWidgets)
        if (w->job() == job) return w;

    // not found — create fresh (same as onJobAdded)
    JobWidget* w = new JobWidget(job);
    w->setProperty("jobId", job->id());
    connect(w, &JobWidget::openDetailsRequested, this, [this](const QString& id) {
        openDetails(m_manager.getJob(id));
    });
    return w;
}

// ---------------------------------------------------------------------------
// Job details tab
// ---------------------------------------------------------------------------
void MainWindow::openDetails(Job* job){
    // enable / disable panel
    bool validJob = (bool) job;

    ui->detailsName     ->setEnabled(validJob);
    ui->detailsType     ->setEnabled(validJob);
    ui->detailsLocal    ->setEnabled(validJob);
    ui->detailsRemote   ->setEnabled(validJob);
    ui->detailsAutostart->setEnabled(validJob);
    ui->detailsReadOnly ->setEnabled(validJob);
    ui->detailsSave     ->setEnabled(validJob);
    ui->detailsDelete   ->setEnabled(validJob);

    if(!validJob) return;

    // save current job
    m_currentJobDetails = job;

    // update ui
    ui->detailsName->setText(job->name());
    ui->detailsType->setCurrentIndex(ui->detailsType->findText(job->type()));
    ui->detailsLocal->setText(job->local());
    ui->detailsRemote->setText(job->remote());
    ui->detailsAutostart->setChecked(job->autostart());
    ui->detailsReadOnly->setChecked(job->readOnly());

    // populate output log
    ui->detailsOutput->setText(job->getCommand(false).join(' '));

    // show details tab
    ui->tabWidget->setCurrentWidget(ui->tabDetails);
}
void MainWindow::clearDetails(){
    m_currentJobDetails = nullptr;

    ui->detailsName->clear();
    ui->detailsLocal->clear();
    ui->detailsRemote->clear();
    ui->detailsAutostart->setChecked(false);
    ui->detailsReadOnly->setChecked(false);
    ui->detailsOutput->clear();
}

void MainWindow::onDetailsSave(){
    if(!m_currentJobDetails) return;
    Job* job = m_currentJobDetails;

    job->setName(ui->detailsName->text());
    job->setType(ui->detailsType->currentText());
    job->setLocal(ui->detailsLocal->text());
    job->setRemote(ui->detailsRemote->text());
    job->setAutostart(ui->detailsAutostart->isChecked());
    job->setReadOnly(ui->detailsReadOnly->isChecked());

    ui->detailsOutput->setText(job->getCommand(false).join(' '));

    if(m_manager.save()){
        statusBar()->showMessage("Job saved.", Config::STATUS_DURATION);
        ui->tabWidget->setCurrentWidget(ui->tabJobs);
    }else{
        statusBar()->showMessage("Error saving job.", Config::STATUS_DURATION);
    }
}
void MainWindow::onDetailsDelete(){
    if (!m_currentJobDetails) return;

    Job* toRemove = m_currentJobDetails;
    m_currentJobDetails = nullptr;   // clear before deletion
    m_manager.removeJob(toRemove);

    if(m_manager.save()){
        statusBar()->showMessage("Job removed.", Config::STATUS_DURATION);

        clearDetails();
        openDetails(nullptr);
        ui->tabWidget->setCurrentWidget(ui->tabJobs);
    }else{
        statusBar()->showMessage("Error removing job.", Config::STATUS_DURATION);
    }

}
void MainWindow::onLocalSelectClicked(){
    QString path = QFileDialog::getExistingDirectory(
        this, "Select local folder or mount point", ui->detailsLocal->text());
    if (!path.isEmpty()) {
        ui->detailsLocal->setText(QDir::toNativeSeparators(path));
    }
}

// ---------------------------------------------------------------------------
// Model events
// ---------------------------------------------------------------------------
void MainWindow::onJobOutputLine(const QString& id, const QString& line){
    // Only append to the output widget if the right service is in the details tab
    if (!m_currentJobDetails || id != m_currentJobDetails->id()) return;

    ui->detailsOutput->append(line);

}
void MainWindow::onJobAdded(Job* job){
    JobWidget* w = new JobWidget(job);

    connect(w, &JobWidget::openDetailsRequested, this, [this](const QString& id) {
        openDetails(m_manager.getJob(id));
    });

    connect(job, &Job::outputLine,    this, &MainWindow::onJobOutputLine);
    m_jobWidgets.append(w);
    ui->jobsList->layout()->addWidget(w);

    connect(job, &Job::logError, this, [this](const QString& id){
        statusBar()->showMessage("Error opening job log file.", Config::STATUS_DURATION);
    });
}
void MainWindow::onJobRemoved(const QString& jobId){
    for (int i = 0; i < m_jobWidgets.size(); ++i) {
        if (m_jobWidgets[i]->job()->id() == jobId) {
            delete m_jobWidgets[i];
            m_jobWidgets.removeAt(i);
            break;
        }
    }
};

// ---------------------------------------------------------------------------
// Tray icon
// ---------------------------------------------------------------------------
void MainWindow::setupTray(){
    m_trayIcon = new QSystemTrayIcon(this);

    // Placeholder icon — replace with your actual app icon
    m_trayIcon->setIcon(QIcon(":/favicon.svg"));
    m_trayIcon->setToolTip(Config::APP_NAME);

    m_trayMenu  = new QMenu(this);
    m_trayOpen  = m_trayMenu->addAction("Open");
    m_trayMenu->addSeparator();
    // Service actions are inserted here dynamically (see onTrayMenuAboutToShow)
    m_trayMenu->addSeparator();
    m_trayClose = m_trayMenu->addAction("Quit");

    connect(m_trayOpen,  &QAction::triggered,         this, &MainWindow::show);
    connect(m_trayClose, &QAction::triggered,         qApp, &QApplication::quit);
    connect(m_trayMenu,  &QMenu::aboutToShow,         this, &MainWindow::onTrayMenuAboutToShow);
    connect(m_trayIcon,  &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
}
void MainWindow::onTrayMenuAboutToShow(){
    // Remove all actions between the first separator and the last separator
    // (i.e. the dynamically added service actions from last time)
    QList<QAction*> actions = m_trayMenu->actions();
    QAction* lastSep  = nullptr;
    bool inside = false;
    for (QAction*& a : actions) {
        if (a->isSeparator()) {
            lastSep = a;
            inside = !inside;
        }else if (inside){
            m_trayMenu->removeAction(a);
            delete a;
        }
    }

    // Re-insert current service actions before lastSep
    for(JobWidget*& jw : m_jobWidgets){
        Job* job = jw->job();
        QPixmap icon = QPixmap(jw->getStatusIcon());

        QAction* act = new QAction(icon, job->name(), m_trayMenu);

        // Toggle on click
        connect(act, &QAction::triggered, this, [job]() {
            job->toggle();
        });

        m_trayMenu->insertAction(lastSep, act);
    }
}
void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason){
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
    }
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
// General error messages
// ---------------------------------------------------------------------------
bool MainWindow::isRcloneInstalled(){
    QString path = m_manager.shared()->rclonePath();
    QFileInfo fi(path);
    return (fi.exists() && fi.isExecutable()) || !QStandardPaths::findExecutable(path).isEmpty();
}
bool MainWindow::isWinFspInstalled(){
#ifdef Q_OS_WIN
    // Method 1: Check registry
    QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\WinFsp", QSettings::NativeFormat);
    if (!reg.allKeys().isEmpty()){
        return true;
    }

    // Method 2: Check default install path
    QStringList paths = {
        "C:/Program Files (x86)/WinFsp",
        "C:/Program Files/WinFsp"
    };
    for (const QString &path : paths) {
        if (QDir(path).exists())
            return true;
    }
    return false;
#else
    return false; // WinFsp is Windows-only
#endif
}

