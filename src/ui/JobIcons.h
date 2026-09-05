#pragma once

#include "src/core/Job.h"
#include "src/core/JobType.h"

#include <QPixmap>

// ---------------------------------------------------------------------------
// JobIcons
// Renders the small SVG icons used to represent a job's type and status.
// Only a handful of distinct variants ever exist (a few job types x
// active/inactive, a few statuses x warning/no-warning), so each rendered
// QPixmap is cached the first time it's needed rather than re-decoded from
// its SVG resource on every status/progress update.
//
// Shared by JobWidget (the jobs list) and TrayController (the tray menu),
// so the tray can build the same icons directly from a Job's type/status
// without depending on JobWidget for them.
// ---------------------------------------------------------------------------
namespace JobIcons {

    // Icon for a job's type (mount/copy/sync), reflecting whether it's
    // currently active, scaled to `size` pixels square.
    QPixmap jobIcon(JobType type, bool active, int size);

    // Status icon (Stopped/Starting/Running/...), with a small warning
    // badge overlaid if `hasWarning` is set, scaled to `size` pixels
    // square.
    QPixmap statusIcon(JobStatus status, bool hasWarning, int size);

}
