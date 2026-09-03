#include "AppContext.h"

#include "src/common/Config.h"
#include "src/core/Job.h"
#include "src/providers/platform/WinFspDetector.h"
#include "src/providers/rclone/RCloneProviderWindows.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>

AppContext::AppContext(QObject* parent) : QObject(parent){
    m_filePath = QDir(QCoreApplication::applicationDirPath())
    .filePath(Config::SETTINGS_FILENAME);

    // TODO: pick these based on the host platform once non-Windows support
    // exists (e.g. a FUSE-based RCloneProvider/MountBackendDetector).
    m_rcloneProvider = std::make_unique<RCloneProviderWindows>();
    m_mountBackendDetector = std::make_unique<WinFspDetector>();
}
AppContext::~AppContext(){
    qDeleteAll(m_jobs);
    m_jobs.clear();
}

Job* AppContext::getJob(QString id){
    for(Job*& job : m_jobs){
        if(job->id() == id){
            return job;
        }
    }

    return nullptr;
}
void AppContext::addJob(Job* newJob){
    if (getJob(newJob->id())) {
        delete newJob;
        return;
    }

    m_jobs.append(newJob);

    emit added(newJob);
}
void AppContext::moveJob(const QString& id, int newIndex)
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
void AppContext::removeJob(QString id){
    for(int i = 0; i < m_jobs.size(); ++i){
        if(m_jobs[i]->id() == id){
            emit removed(m_jobs[i]->id());
            delete m_jobs[i];
            m_jobs.removeAt(i);
            break;
        }
    }
}
void AppContext::removeJob(Job* job){
    removeJob(job->id());
}

AppContext::LoadResult AppContext::load()
{
    QFile file(m_filePath);

    // create settings file on first run — expected/benign, not an error
    if (!file.exists()) {
        save();
        return LoadResult::FirstRun;
    }

    // read settings file to json object
    if (!file.open(QIODevice::ReadOnly)) {
        return LoadResult::LoadError;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return LoadResult::LoadError;
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
                Job* job = new Job(m_shared.get(), m_rcloneProvider.get());
                job->fromJson(v);
                if(job->autostart()) job->start();
                addJob(job);
            }
        }
    }

    return LoadResult::Loaded;
}

bool AppContext::save() const
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