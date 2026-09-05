#include "JobStatus.h"

QString jobStatustoString(JobStatus status){
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
    if (normalized == QStringLiteral("Starting")) return JobStatus::Starting;
    if (normalized == QStringLiteral("Running"))  return JobStatus::Running;
    if (normalized == QStringLiteral("Errored"))  return JobStatus::Errored;
    if (normalized == QStringLiteral("Stopping")) return JobStatus::Stopping;
    if (normalized == QStringLiteral("Success"))  return JobStatus::Success;

    return fallback;
}