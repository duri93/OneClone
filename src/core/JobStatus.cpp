#include "JobStatus.h"

QString jobStatusToString(JobStatus status){
    switch(status){
    case JobStatus::Stopped:  return QStringLiteral("Stopped");
    case JobStatus::Starting: return QStringLiteral("Starting");
    case JobStatus::Running:  return QStringLiteral("Running");
    case JobStatus::Errored:  return QStringLiteral("Errored");
    case JobStatus::Stopping: return QStringLiteral("Stopping");
    case JobStatus::Success:  return QStringLiteral("Success");
    }

    return QStringLiteral("Unknown");
}

JobStatus jobStatusFromString(const QString& str, JobStatus fallback){

    const QString normalized = str.trimmed().toLower();

    if (normalized == QStringLiteral("stopped"))  return JobStatus::Stopped;
    if (normalized == QStringLiteral("starting")) return JobStatus::Starting;
    if (normalized == QStringLiteral("running"))  return JobStatus::Running;
    if (normalized == QStringLiteral("errored"))  return JobStatus::Errored;
    if (normalized == QStringLiteral("stopping")) return JobStatus::Stopping;
    if (normalized == QStringLiteral("success"))  return JobStatus::Success;

    return fallback;
}