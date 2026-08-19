#include "Manager.h"
#include "Config.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSettings>

Manager::Manager(QObject* parent) : QObject(parent){
    m_filePath = QDir(QCoreApplication::applicationDirPath())
    .filePath(Config::SETTINGS_FILENAME);
}
Manager::~Manager(){
    qDeleteAll(m_jobs);
    m_jobs.clear();
}

Job* Manager::getJob(QString id){
    for(Job*& job : m_jobs){
        if(job->id() == id){
            return job;
        }
    }

    return nullptr;
}
void Manager::addJob(Job* newJob){
    if (getJob(newJob->id())) {
        delete newJob;
        return;
    }

    m_jobs.append(newJob);
    emit added(newJob);
}
void Manager::moveJob(const QString& id, int newIndex)
{
    int oldIndex = -1;
    for (int i = 0; i < m_jobs.size(); ++i) {
        if (m_jobs[i]->id() == id) { oldIndex = i; break; }
    }
    if (oldIndex == -1 || oldIndex == newIndex) return;

    Job* job = m_jobs.takeAt(oldIndex);
    newIndex = qBound(0, newIndex, m_jobs.size());
    m_jobs.insert(newIndex, job);
}
void Manager::removeJob(QString id){
    for(int i = 0; i < m_jobs.size(); ++i){
        if(m_jobs[i]->id() == id){
            emit removed(m_jobs[i]->id());
            delete m_jobs[i];
            m_jobs.removeAt(i);
            break;
        }
    }
}
void Manager::removeJob(Job* job){
    removeJob(job->id());
}

bool Manager::load()
{
    QFile file(m_filePath);

    // create settings file on first run
    if (!file.exists()) {
        save();
        return false;
    }

    // read settings file to json object
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();

    // import shared settings and jobs
    if (root.contains("shared") && root["shared"].isObject()) {
        this->m_shared = std::make_unique<SharedSettings>(root["shared"].toObject());
    }

    // remove old jobs
    for(Job*& job:m_jobs){
        emit removed(job->id());
        delete job;
    }
    m_jobs.clear();

    // add new jobs
    if (root.contains("jobs") && root["jobs"].isArray()) {
        for (const QJsonValue& v : root["jobs"].toArray()) {
            if (v.isObject()) {
                Job* job = new Job(m_shared.get());
                job->fromJson(v);
                if(job->autostart()) job->start();
                addJob(job);
            }
        }
    }

    return true;
}

bool Manager::save() const
{
    QJsonObject root;
    root["shared"] = m_shared->toJson();

    QJsonArray arr;
    for (Job* job : m_jobs) {
        arr.append(job->toJson());
    }
    root["jobs"] = arr;

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool Manager::isRcloneInstalled(){
    QString path = shared()->rclonePath();

    QFileInfo fi(path);
    return (fi.exists() && fi.isExecutable()) || !QStandardPaths::findExecutable(path).isEmpty();
}
bool Manager::isWinFspInstalled(){
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
bool Manager::openRcloneConf(){
    QString rclonePath = shared()->rclonePath();

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
