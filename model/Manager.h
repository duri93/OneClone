#pragma once

#include "Job.h"
#include "../model/SharedSettings.h"

#include <QObject>

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject* parent = nullptr);
    ~Manager();

    // getters
    SharedSettings*           shared()         { return m_shared; }
    const SharedSettings*     shared()   const { return m_shared; }

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
    void removed(Job* job);

private:
    QString           m_filePath;
    SharedSettings*   m_shared = new SharedSettings();
    QVector<Job*> m_jobs;
};
