#pragma once

#include <QString>

enum class JobStatus{
    Stopped,
    Starting,
    Running,
    Success,
    Stopping,
    Errored
};

QString jobStatusToString(JobStatus status);

JobStatus jobStatusFromString(const QString& str, JobStatus fallback = JobStatus::Stopped);