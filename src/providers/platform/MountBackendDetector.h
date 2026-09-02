#pragma once

// ---------------------------------------------------------------------------
// MountBackendDetector
// Abstract interface for checking whether the OS-level filesystem backend
// needed to mount rclone remotes is installed (e.g. WinFsp on Windows,
// FUSE on Linux).
// ---------------------------------------------------------------------------
class MountBackendDetector
{
public:
    virtual ~MountBackendDetector() = default;

    // Returns true if the mount backend is installed and usable.
    virtual bool isAvailable() const = 0;
};
