#include "Manager.h"
#include "../model/Config.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>

Manager::Manager(QObject* parent) : QObject(parent){
    m_filePath = QDir(QCoreApplication::applicationDirPath())
    .filePath(Config::SETTINGS_FILENAME);
}
Manager::~Manager(){
    delete m_shared;
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
    Job* job = getJob(newJob->id());
    if(job) return;

    m_jobs.append(newJob);

    emit added(newJob);
}

void Manager::removeJob(QString id){
    for(int i = 0; i < m_jobs.size(); ++i){
        if(m_jobs[i]->id() == id){
            emit removed(m_jobs[i]);
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
        delete this->m_shared;
        this->m_shared = new SharedSettings(root["shared"].toObject());
    }

    // remove old jobs
    for(Job*& job:m_jobs){
        emit removed(job);
        delete job;
    }
    m_jobs.clear();

    // add new jobs
    if (root.contains("jobs") && root["jobs"].isArray()) {
        for (const QJsonValue& v : root["jobs"].toArray()) {
            if (v.isObject()) {
                Job* job = new Job(m_shared);
                job->fromJson(v);
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

