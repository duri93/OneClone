#pragma once

#include "src/core/SharedSettings.h"
#include "src/providers/platform/MountBackendDetector.h"
#include "src/providers/rclone/RCloneProvider.h"

#include <memory>
#include <QObject>

class Job;

// ---------------------------------------------------------------------------
// AppContext
// Application-wide shared state: persisted settings, the job list, and the
// platform capability providers (rclone, mount backend) that jobs are built
// with. Owns the settings file's load/save lifecycle.
// ---------------------------------------------------------------------------
class AppContext : public QObject
{
    Q_OBJECT
public:
    explicit AppContext(QObject* parent = nullptr);
    ~AppContext();

    // getters
    SharedSettings*           shared()         { return m_shared.get(); }
    const SharedSettings*     shared()   const { return m_shared.get(); }
    const QVector<Job*>& jobs() const { return m_jobs; }
    RCloneProvider*           rcloneProvider()       const { return m_rcloneProvider.get(); }
    MountBackendDetector*     mountBackendDetector() const { return m_mountBackendDetector.get(); }

    // job management
    Job* getJob(QString id);
    void addJob(Job* newJob);
    void moveJob(const QString& id, int newIndex);
    void removeJob(Job* job);
    void removeJob(QString id);

    // general settings
    enum class LoadResult {
        Loaded,     // existing settings file was read successfully
        FirstRun,   // no settings file existed yet — defaults were created (benign)
        LoadError,  // a settings file existed but could not be read/parsed
    };
    LoadResult load();
    bool save() const;
signals:
    void added(Job* newJob);
    void removed(const QString& jobId);

private:
    QString           m_filePath;
    std::unique_ptr<SharedSettings>       m_shared = std::make_unique<SharedSettings>();
    std::unique_ptr<RCloneProvider>       m_rcloneProvider;
    std::unique_ptr<MountBackendDetector> m_mountBackendDetector;
    QVector<Job*> m_jobs;
};
