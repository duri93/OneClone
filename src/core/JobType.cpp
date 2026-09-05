#include "JobType.h"

QString jobTypeToString(JobType type)
{
    switch (type) {
    case JobType::Mount: return QStringLiteral("mount");
    case JobType::Copy:  return QStringLiteral("copy");
    case JobType::Sync:  return QStringLiteral("sync");
    }
    return QStringLiteral("sync"); // unreachable, keeps compilers happy
}

JobType jobTypeFromString(const QString& str, JobType fallback)
{
    const QString normalized = str.trimmed().toLower();

    if (normalized == QStringLiteral("mount")) return JobType::Mount;
    if (normalized == QStringLiteral("copy"))  return JobType::Copy;
    if (normalized == QStringLiteral("sync"))  return JobType::Sync;

    return fallback;
}
