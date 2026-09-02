#pragma once

#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QObject;
class QProcess;
QT_END_NAMESPACE

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
};