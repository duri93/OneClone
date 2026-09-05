#pragma once

#include "src/core/JobStatus.h"
#include "src/core/JobType.h"
#include "src/core/LogFile.h"
#include "src/core/SharedSettings.h"
#include "src/core/Status.h"
#include "src/providers/rclone/RCloneProvider.h"

#include <memory>
#include <QObject>
#include <QProcess>
#include <QString>

struct JobProgress{
    QString bytes = "";
    QString speed = "";
    int percent = 0;
    QString eta = "";
};

// ---------------------------------------------------------------------------
// Job
// A single rclone operation (mount/copy/sync) with its settings, live
// status/progress, and the QProcess that runs it.
// ---------------------------------------------------------------------------
class Job : public QObject
{
    Q_OBJECT

public:
    explicit Job(SharedSettings* shared, RCloneProvider* rcloneProvider, QObject* parent = nullptr);
    ~Job() override;

    // property accessors
    QString id()           const { return m_id;           };
    QString name()         const { return m_name;         };
    JobType type()         const { return m_type;         };
    QString local()        const { return m_local;        };
    QString remote()       const { return m_remote;       };
    bool    autostart()    const { return m_autostart;    };
    bool    readOnly()     const { return m_readOnly;     };
    bool    deleteBefore() const { return m_deleteBefore; };

    void setId           (QString newId        ){ m_id           = std::move(newId);     emit specsChanged(); }
    void setName         (QString newName      ){ m_name         = std::move(newName);   emit specsChanged(); }
    void setType         (JobType newType      ){ m_type         = newType;              emit specsChanged(); }
    void setLocal        (QString newLocal     ){ m_local        = std::move(newLocal);  emit specsChanged(); }
    void setRemote       (QString newRemote    ){ m_remote       = std::move(newRemote); emit specsChanged(); }
    void setAutostart    (bool newAutostart    ){ m_autostart    = newAutostart;         emit specsChanged(); }
    void setReadOnly     (bool newReaOnly      ){ m_readOnly     = newReaOnly;           emit specsChanged(); }
    void setDeleteBefore (bool newDeleteBefore ){ m_deleteBefore = newDeleteBefore;      emit specsChanged(); }

    // import / export
    QJsonObject toJson() const;
    void fromJson(const QJsonValue& json);

    // Status accessors
    JobStatus        status()   const { return m_status; }
    bool             active()   const { return m_status == JobStatus::Starting || m_status == JobStatus::Running; }
    JobProgress      progress() const { return m_progress; }
    QVector<QString> warnings() const { return m_warnings; }

    // Process handling
    QStringList getCommand(bool swapSides = false);

    void start(bool swapSides = false);
    void stop();

    bool openLogFile();
    bool openLocalFolder();

public slots:
    void toggle(bool swapSides = false);

signals:
    void specsChanged();
    void statusChanged(const QString& serviceId, const JobStatus& newStatus);
    void warning(const QString& serviceId, const QString& line);
    void progressUpdated(const QString& serviceId, const JobProgress& newProgress);
    void outputLine(const QString& serviceId, const QString& line);

    // Anything Job would otherwise have reported straight to the app-wide
    // Status bus (log-write failures, "no local folder configured", etc.)
    // is emitted here instead, keeping this model class decoupled from
    // that UI-layer broadcast channel. A listener at the UI boundary (see
    // JobsTabController) forwards these to Status::notify().
    void notification(const QString& message, Status::Level level);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setStatus(JobStatus s);
    void processLine(const QString& line);
    void processLineOutput(const QString & line);
    bool processLineProgress(const QString& line);

    QString m_id;       // internal UUID, not shown in UI
    QString m_name    = "New job";
    JobType m_type    = JobType::Sync;
    QString m_local   = "";
    QString m_remote  = "";
    bool m_autostart  = false;
    bool m_readOnly   = false;
    bool m_deleteBefore = false;


    SharedSettings*  m_shared;
    RCloneProvider*  m_rcloneProvider;
    JobStatus        m_status = JobStatus::Stopped;
    QVector<QString> m_warnings;
    JobProgress      m_progress;
    QProcess         m_process;
    std::unique_ptr<LogFile> m_logfile;
};
