#pragma once

#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QObject;
class QProcess;
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// RcloneCommandParams
// Every parameter needed to build the rclone command line for a job,
// decoupled from the Job/SharedSettings classes so the command can be
// generated from arbitrary (e.g. not-yet-saved) UI input as well.
// ---------------------------------------------------------------------------
struct RcloneCommandParams
{
    // job-specific
    QString type;              // "mount", "sync" or "copy"
    QString local;
    QString remote;
    bool    readOnly     = false; // mount only
    bool    deleteBefore = false; // sync only
    bool    swapSides    = false; // sync/copy only

    // shared / vfs settings
    QString cacheMode;
    int     cacheMaxSize       = 0;   // GB
    int     cacheMinFreeSpace  = 0;   // GB
    int     cacheMaxAge        = 0;   // hours
    int     readChunkSize      = 0;   // MB
    int     readChunkSizeLimit = 0;   // MB
    int     bufferSize         = 0;   // MB
    int     transfers          = 0;
    int     checkers           = 0;
    bool    links               = false;
};

// ---------------------------------------------------------------------------
// RCloneProvider
// Abstract interface for every touchpoint the app has with the rclone
// binary: availability checks, the interactive config tool, remote/dir
// listing, and resolving the executable used to launch jobs. Platform
// implementations (e.g. RCloneProviderWindows) provide the concrete
// behavior.
// ---------------------------------------------------------------------------
class RCloneProvider
{
public:
    virtual ~RCloneProvider() = default;

    // Builds the rclone command-line arguments (excluding the executable
    // itself) for the given parameters. Pure string-building, so it is not
    // platform-specific and is shared by every RCloneProvider implementation.
    QStringList buildCommand(const RcloneCommandParams& params) const;

    // Returns true if rclonePath points to a usable rclone executable
    // (either directly, or found on the system PATH).
    virtual bool isAvailable(const QString& rclonePath) const = 0;

    // Resolves rclonePath to the actual command used to launch rclone jobs
    // (e.g. resolving a bare "rclone" through the system PATH).
    virtual QString resolveExecutable(const QString& rclonePath) const = 0;

    // Launches the interactive "rclone config" tool. Returns the running
    // process (parented to `parent`), or nullptr if it failed to start.
    virtual QProcess* openConfig(const QString& rclonePath, QObject* parent = nullptr) const = 0;

    // Opens the active rclone.conf file in the OS file browser.
    virtual bool openConfigFile(const QString& rclonePath) const = 0;

    // Returns the names of all remotes configured in rclone.conf.
    virtual QStringList listRemotes(const QString& rclonePath) const = 0;

    // Returns the top-level directory names inside the given remote.
    virtual QStringList listDirs(const QString& rclonePath, const QString& remote) const = 0;

    // Attempts a graceful stop of a running job process (e.g. sending
    // CTRL+C on Windows instead of a hard kill). Returns true if a stop
    // signal was actually sent, in which case the caller should still
    // arrange a kill() fallback in case the process doesn't exit in time.
    // The default implementation does nothing and returns false, so
    // platforms without a graceful mechanism just kill() immediately.
    virtual bool requestGracefulStop(QProcess& process) const { Q_UNUSED(process); return false; }

    // Called once a process is finished, to let a provider clean up any
    // platform-specific state left behind by requestGracefulStop() (e.g.
    // Windows' console control handler suppression). Safe to call even if
    // requestGracefulStop() was never called or returned false.
    virtual void notifyProcessFinished(QProcess& process) const { Q_UNUSED(process); }

    // Resolves a job's configured local path to wherever it's actually
    // reachable on disk right now, for opening it in the OS file browser.
    // For an ordinary path this is just the path itself. But a mount's
    // local path can be something the OS only resolves indirectly — e.g.
    // on Windows, WinFsp mounts configured with a UNC-style path such as
    // \\rclone\media are actually backed by a drive letter chosen at mount
    // time, and only Windows' network-connection table can say which one.
    // The default implementation returns `local` unchanged; platforms with
    // that kind of indirection should override it.
    virtual QString resolveLocalPath(const QString& local) const { return local; }
};