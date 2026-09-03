#include "Job.h"

#include "src/common/Config.h"
#include "src/core/Status.h"

#include <atomic>
#include <QDesktopServices>
#include <QDir>
#include <QTimer>
#include <QUuid>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
static std::atomic<int> s_ctrlSuppressCount{0};

Job::Job(SharedSettings* shared, RCloneProvider* rcloneProvider, QObject* parent)
    : QObject(parent), m_shared(shared), m_rcloneProvider(rcloneProvider){

    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_name = "New job";

    // Wire up process signals
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &Job::onReadyRead);
    connect(&m_process, &QProcess::errorOccurred, this, &Job::onProcessError);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Job::onProcessFinished);

    // Merge stderr into stdout channel for unified output display
    m_process.setProcessChannelMode(QProcess::MergedChannels);
}
Job::~Job(){
    if (m_process.state() != QProcess::NotRunning) {
        m_status = JobStatus::Stopping;
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

const QString Job::statusString() const{
    switch(status()){
    case JobStatus::Stopped:
        return "Stopped";
    case JobStatus::Starting:
        return "Starting";
    case JobStatus::Running:
        return "Running";
    case JobStatus::Errored:
        return "Errored";
    case JobStatus::Stopping:
        return "Stopping";
    case JobStatus::Success:
        return "Success";
    }

    return "Unknown";
}

// ---------------------------------------------------------------------------
// start / stop
// ---------------------------------------------------------------------------
QStringList Job::getCommand(bool swapSides){
    RcloneCommandParams params;
    params.type         = m_type;
    params.local        = m_local;
    params.remote       = m_remote;
    params.readOnly     = m_readOnly;
    params.deleteBefore = m_deleteBefore;
    params.swapSides    = swapSides;

    params.cacheMode          = m_shared->cacheMode();
    params.cacheMaxSize       = m_shared->cacheMaxSize();
    params.cacheMinFreeSpace  = m_shared->cacheMinFreeSpace();
    params.cacheMaxAge        = m_shared->cacheMaxAge();
    params.readChunkSize      = m_shared->readChunkSize();
    params.readChunkSizeLimit = m_shared->readChunkSizeLimit();
    params.bufferSize         = m_shared->bufferSize();
    params.transfers          = m_shared->transfers();
    params.checkers           = m_shared->checkers();
    params.links              = m_shared->links();

    return m_rcloneProvider->buildCommand(params);
}

void Job::toggle(bool swapSides){
    if(active()){
        stop();
    }else{
        start(swapSides);
    }
}
void Job::start(bool swapSides){
    // don't start if already running
    if(active()) return;

    // generate rclone command
    QStringList args = this->getCommand(swapSides);

    // open log file
    delete m_logfile;
    m_logfile = new LogFile(m_name, this);
    connect(m_logfile, &LogFile::error, this, [this](const QString& message){
        emit warning(m_id, message);
    });
    m_logfile->write(args.join(' '));

    // clear job warnings
    m_warnings.clear();

    // start process
    setStatus(JobStatus::Starting);

    m_process.setProgram(m_rcloneProvider->resolveExecutable(m_shared->rclonePath()));
    m_process.setArguments(args);
    m_process.start();

    // QProcess::started is not connected here; we transition to Running on
    // first stdout output to avoid false positives while rclone initialises.
}
void Job::stop(){
    if (!active()) return;
    setStatus(JobStatus::Stopping);

#ifdef Q_OS_WIN
    if (AttachConsole(m_process.processId())) {
        if (s_ctrlSuppressCount.fetch_add(1) == 0){
            SetConsoleCtrlHandler(nullptr, TRUE);   // suppress it in our process
        }

        GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
        FreeConsole();

        QTimer::singleShot(3000, this, [this]() {
            if (m_process.state() != QProcess::NotRunning)
                m_process.kill();  // fallback
        });

    } else {
        m_process.kill();
    }
#else
    m_process.kill();
#endif
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------
void Job::onReadyRead(){
    // Read all available data line by line
    while (m_process.canReadLine()) {
        QString line = QString::fromUtf8(m_process.readLine()).trimmed();
        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}
void Job::onProcessError(QProcess::ProcessError error){
    Q_UNUSED(error)

    if(error == QProcess::FailedToStart){
        processLineOutput(QString("[ERROR] Failed to start: %1").arg(m_process.errorString()));
    }else{
        processLineOutput(QString("[ERROR] Process error: %1").arg(m_process.errorString()));
    }

    setStatus(JobStatus::Errored);
}
void Job::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus){
    Q_UNUSED(exitStatus)

#ifdef Q_OS_WIN
    if (s_ctrlSuppressCount.load() > 0) {
        if (--s_ctrlSuppressCount == 0)
            SetConsoleCtrlHandler(nullptr, FALSE);
    }
#endif

    // Drain any remaining output
    onReadyRead();

    if (m_status == JobStatus::Errored) {
        // already Errored from a matched error pattern — leave status as-is
    } else if(m_status == JobStatus::Stopping){
        setStatus(JobStatus::Stopped);
    } else if (exitCode != 0) {
        emit outputLine(m_id, QString("[ERROR] Process exited with code %1").arg(exitCode));
        setStatus(JobStatus::Errored);
    } else {
        setStatus(JobStatus::Success);
    }

    delete m_logfile;
    m_logfile = nullptr;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void Job::setStatus(JobStatus s){
    if (m_status == s) return;
    m_status = s;
    emit statusChanged(m_id, s);
}
void Job::processLine(const QString& line){
    // Transition from Starting -> Running on first real output
    if (m_status == JobStatus::Starting) {
        if (m_type == "mount") {
            // mount signals readiness with a specific string
            if (line.contains(Config::DEFAULT_START_STRING))
                setStatus(JobStatus::Running);
        } else {
            // sync/copy: any output means the process is alive and running
            setStatus(JobStatus::Running);
        }
    }

    // parse progress for sync/copy commands
    if(m_type == "mount"){
        processLineOutput(line);
    }else{
        static const QRegularExpression re(
            QStringLiteral("^(?:Transferred|Errors|Checks|Elapsed time|Transferring|\\*\\s)"),
            QRegularExpression::CaseInsensitiveOption
        );
        bool wasProgress = false;
        if(re.match(line).hasMatch()){
            wasProgress = processLineProgress(line);
        }
        if(!wasProgress){
            processLineOutput(line);
        }
    }

    // Check for error pattern
    if (Config::WARNING_REGEX.match(line).hasMatch()) {
        m_warnings.append(line);
        emit warning(m_id, line);
        return;
    }
}
void Job::processLineOutput(const QString &line){
    // save to log file
    m_logfile->write(line);

    // emit signal
    emit outputLine(m_id, line);
}
bool Job::processLineProgress(const QString& line){
    // Matches: "Transferred: 123.45 MiB / 1.23 GiB, 10%, 1.23 MiB/s, ETA 1m23s"
    static const QRegularExpression re(
        R"(Transferred:\s+([\d.]+ \S+ \/ [\d.]+ \S+),\s+(\d+)%,\s+([\S]+ \S+\/s),\s+ETA\s+(\S+))");

    QRegularExpressionMatch m = re.match(line);
    if (!m.hasMatch()) return false;

    // TODO: recognize as number and format number of significant digits?
    m_progress.bytes   = m.captured(1);   // e.g. "123.45 MiB"
    m_progress.percent = m.captured(2).toInt();
    m_progress.speed   = m.captured(3);   // e.g. "1.23 MiB/s"
    m_progress.eta     = m.captured(4);   // e.g. "1m23s"

    emit progressUpdated(m_id, m_progress);
    return true;
}

// ---------------------------------------------------------------------------
// Log helpers
// ---------------------------------------------------------------------------

bool Job::openLogFile(){
    if(!m_logfile) return false;

    const QFile& log = m_logfile->file();

    if(log.fileName().isEmpty() || !log.exists()){
        return false;
    }

    QDesktopServices::openUrl(
        QUrl::fromLocalFile(log.fileName())
    );

    return true;
}
bool Job::openLocalFolder(){
    if(m_local.isEmpty()){
        Status::notify(
            tr("Job \"%1\" has no local folder configured.").arg(m_name),
            Status::Level::Warning);
        return false;
    }

    if(!QDesktopServices::openUrl(QUrl::fromLocalFile(this->m_local))){
        Status::notify(
            tr("Could not open local folder for \"%1\".").arg(m_name),
            Status::Level::Error);
        return false;
    }

    return true;
}

void Job::fromJson(const QJsonValue& json){
    this->m_id           = json["id"]          .toString();
    this->m_name         = json["name"]        .toString();
    this->m_type         = json["type"]        .toString();
    this->m_local        = json["local"]       .toString();
    this->m_remote       = json["remote"]      .toString();
    this->m_autostart    = json["autostart"]   .toBool(false);
    this->m_readOnly     = json["readOnly"]    .toBool(false);
    this->m_deleteBefore = json["deleteBefore"].toBool(false);

    if (this->m_id.isEmpty()) {
        this->m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}
const QJsonObject Job::toJson() const{
    QJsonObject o;
    o["id"]           = this->m_id;
    o["name"]         = this->m_name;
    o["type"]         = this->m_type;
    o["local"]        = this->m_local;
    o["remote"]       = this->m_remote;
    o["autostart"]    = this->m_autostart;
    o["readOnly"]     = this->m_readOnly;
    o["deleteBefore"] = this->m_deleteBefore;
    return o;
}

