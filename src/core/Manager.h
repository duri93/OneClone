#pragma once
#include "src/core/SharedSettings.h"
#include <memory>
#include <QObject>

class Job;
class QProcess;

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject* parent = nullptr);
    ~Manager();

    // getters
    SharedSettings*           shared()         { return m_shared.get(); }
    const SharedSettings*     shared()   const { return m_shared.get(); }
    const QVector<Job*>& jobs() const { return m_jobs; }

    // job management
    Job* getJob(QString id);
    void addJob(Job* newJob);
    void moveJob(const QString& id, int newIndex);
    void removeJob(Job* job);
    void removeJob(QString id);

    // general settings
    bool load();
    bool save() const;

    // prerequisite checks
    bool isRcloneInstalled();
    bool isWinFspInstalled();
    QProcess* openRcloneConf();
    bool openRcloneConfFile();
    QString listRCloneRemotes();
signals:
    void added(Job* newJob);
    void removed(const QString& jobId);

private:
    QString           m_filePath;
    std::unique_ptr<SharedSettings> m_shared = std::make_unique<SharedSettings>();
    QVector<Job*> m_jobs;
};
