#pragma once

#include "Job.h"
#include "../model/SharedSettings.h"

#include <memory>
#include <QObject>

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject* parent = nullptr);
    ~Manager();

    // getters
    SharedSettings*           shared()         { return m_shared.get(); }
    const SharedSettings*     shared()   const { return m_shared.get(); }

    // job management
    Job* getJob(QString id);
    void addJob(Job* newJob);
    void removeJob(Job* job);
    void removeJob(QString id);

    // general
    bool load();
    bool save() const;
signals:
    void added(Job* newJob);
    void removed(const QString& jobId);

private:
    QString           m_filePath;
    std::unique_ptr<SharedSettings> m_shared = std::make_unique<SharedSettings>();
    QVector<Job*> m_jobs;
};
