#include "Job.h"

#include "src/common/Config.h"
#include "src/core/Status.h"

#include <QDesktopServices>
#include <QDir>
#include <QTimer>
#include <QUuid>

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
        m_process.waitForFinished(Config::PROCESS_KILL_WAIT_MS);
    }
}

QString Job::statusString() const{
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
    RcloneCommandParams params = m_shared->toCommandParams();
    params.type         = jobTypeToString(m_type);
    params.local        = m_local;
    params.remote       = m_remote;
    params.readOnly     = m_readOnly;
    params.deleteBefore = m_deleteBefore;
    params.swapSides    = swapSides;

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

    // open log file — replacing any previous run's file for this job
    m_logfile = std::make_unique<LogFile>(m_name);
    connect(m_logfile.get(), &LogFile::error, this, [this](const QString& message){
        // Deliberately NOT emit warning(m_id, message): that signal/its UI
        // (the status-icon tooltip) is for warnings parsed from rclone's
        // own output. A logging failure is a different kind of problem —
        // route it through the app-wide status channel instead, so the two
        // aren't shown through the same indistinguishable UI element.
        emit notification(
            tr("Job \"%1\": %2").arg(m_name, message),
            Status::Level::Warning);
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

    if (m_rcloneProvider->requestGracefulStop(m_process)) {
        // Graceful stop signal sent — give the process a moment to exit on
        // its own, then fall back to a hard kill if it hasn't.
        QTimer::singleShot(Config::JOB_STOP_GRACEFUL_TIMEOUT_MS, this, [this]() {
            if (m_process.state() != QProcess::NotRunning)
                m_process.kill();  // fallback
        });
    } else {
        m_process.kill();
    }
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
    // Job::stop() -> m_process.kill() always emits errorOccurred(Crashed)
    // before finished(). That's expected/intentional here, not a real
    // error, so don't let it clobber the Stopping status; onProcessFinished
    // will transition Stopping -> Stopped once the process actually exits.
    if (error == QProcess::Crashed && m_status == JobStatus::Stopping) {
        return;
    }

    if(error == QProcess::FailedToStart){
        processLineOutput(QString("[ERROR] Failed to start: %1").arg(m_process.errorString()));
    }else{
        processLineOutput(QString("[ERROR] Process error: %1").arg(m_process.errorString()));
    }

    setStatus(JobStatus::Errored);
}
void Job::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus){
    Q_UNUSED(exitStatus)

    // Let the provider clean up any platform-specific state left behind by
    // requestGracefulStop() (e.g. Windows' console control handler
    // suppression). No-op if a graceful stop was never requested.
    m_rcloneProvider->notifyProcessFinished(m_process);

    // Drain any remaining output
    onReadyRead();

    if (m_status == JobStatus::Errored) {
        // already Errored from a matched error pattern — leave status as-is.
        // (Stopping is handled below and is no longer forced into Errored by
        // the Crashed error kill() emits — see onProcessError.)
    } else if(m_status == JobStatus::Stopping){
        setStatus(JobStatus::Stopped);
    } else if (exitCode != 0) {
        emit outputLine(m_id, QString("[ERROR] Process exited with code %1").arg(exitCode));
        setStatus(JobStatus::Errored);
    } else {
        setStatus(JobStatus::Success);
    }
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
        if (m_type == JobType::Mount) {
            // mount signals readiness with a specific string
            if (line.contains(Config::DEFAULT_START_STRING))
                setStatus(JobStatus::Running);
        } else {
            // sync/copy: any output means the process is alive and running
            setStatus(JobStatus::Running);
        }
    }

    // parse progress for sync/copy commands
    if(m_type == JobType::Mount){
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
        emit notification(
            tr("Job \"%1\" has no local folder configured.").arg(m_name),
            Status::Level::Warning);
        return false;
    }

    // For most jobs this is just m_local itself. But a mount's local path
    // can be configured as something the OS only resolves indirectly (see
    // RCloneProvider::resolveLocalPath) — e.g. a UNC-style WinFsp
    // mountpoint that's actually backed by whatever drive letter got
    // picked at mount time.
    const QString path = m_rcloneProvider->resolveLocalPath(m_local);

    if(!QDesktopServices::openUrl(QUrl::fromLocalFile(path))){
        emit notification(
            tr("Could not open local folder for \"%1\".").arg(m_name),
            Status::Level::Error);
        return false;
    }

    return true;
}

void Job::fromJson(const QJsonValue& json){
    this->m_id           = json["id"]          .toString();
    this->m_name         = json["name"]        .toString();
    this->m_type         = jobTypeFromString(json["type"].toString());
    this->m_local        = json["local"]       .toString();
    this->m_remote       = json["remote"]      .toString();
    this->m_autostart    = json["autostart"]   .toBool(false);
    this->m_readOnly     = json["readOnly"]    .toBool(false);
    this->m_deleteBefore = json["deleteBefore"].toBool(false);

    if (this->m_id.isEmpty()) {
        this->m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}
QJsonObject Job::toJson() const{
    QJsonObject o;
    o["id"]           = this->m_id;
    o["name"]         = this->m_name;
    o["type"]         = jobTypeToString(this->m_type);
    o["local"]        = this->m_local;
    o["remote"]       = this->m_remote;
    o["autostart"]    = this->m_autostart;
    o["readOnly"]     = this->m_readOnly;
    o["deleteBefore"] = this->m_deleteBefore;
    return o;
}

