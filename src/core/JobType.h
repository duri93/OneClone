#pragma once

#include <QString>

// ---------------------------------------------------------------------------
// JobType
// The kind of rclone operation a Job represents. Used everywhere in place
// of comparing/storing raw "mount"/"copy"/"sync" string literals, so a typo
// in one spot can't silently fail to match the others.
// ---------------------------------------------------------------------------
enum class JobType {
    Mount,
    Copy,
    Sync
};

// Converts to the lowercase string used in the UI, on the rclone command
// line, and in the JSON settings file (e.g. JobType::Mount -> "mount").
QString jobTypeToString(JobType type);

// Parses the strings produced by jobTypeToString(). Unrecognized input
// (e.g. missing/corrupt JSON) falls back to `fallback`.
JobType jobTypeFromString(const QString& str, JobType fallback = JobType::Sync);
