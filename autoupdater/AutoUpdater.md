# GitHubUpdater — Usage Guide

A single-class (`githubupdater.h` / `githubupdater.cpp`) auto-updater for Qt
desktop apps distributed as GitHub Releases. Works on Windows and Linux.
Fully asynchronous, no third-party dependencies beyond `QtCore` + `QtNetwork`.

This file was verified to compile, link and run cleanly against Qt 5.15
(`-Wall -Wextra -Wpedantic`, zero warnings) before being handed to you.

---

## 1. How it works

1. `checkForUpdates()` calls the GitHub API (`/repos/{owner}/{repo}/releases/latest`)
   and compares the release tag against your current version.
2. If newer, it downloads the release asset that matches your platform into
   a private temp directory. HTTP redirects (GitHub always redirects asset
   downloads to S3) are followed automatically via `NoLessSafeRedirectPolicy`.
3. The download is verified:
   - **Always**: byte-for-byte size match against the size GitHub reports
     for the asset (catches truncated/interrupted downloads).
   - **If you publish a checksum file** alongside the release (any asset
     whose name contains `sha256` or `checksum`), its SHA-256 is verified
     too — strong tamper/corruption detection.
   - If corrupted, the file is deleted and re-downloaded, up to
     `setMaxDownloadAttempts()` times (default 3).
4. Once verified, `updateReady(version)` is emitted — show your "update
   available, will install on restart" message here.
5. The update is applied **automatically the next time the app quits**
   (hooked into `QCoreApplication::aboutToQuit`), or immediately if you call
   `applyUpdateAndRestart()`. A small script (batch on Windows, `sh` on
   Linux) is generated fresh, launched detached, and:
   - waits for your process to actually exit,
   - extracts the archive,
   - copies the new files over the install directory,
   - relaunches the app,
   - deletes all temporary files (including itself).

---

## 2. Required release asset layout

Because there's no bundled unzip library, the updater relies on the `tar`
binary that ships with both target OSes (Windows 10 1803+ includes
`tar.exe`/bsdtar in `System32`; every Linux distro has `tar`):

| Platform | Asset must be     | Contents                                   |
|----------|--------------------|---------------------------------------------|
| Windows  | a `.zip`           | the full application folder (exe + DLLs + resources), no wrapping top-level folder |
| Linux    | a `.tar.gz`        | the full application folder                  |

Recommended GitHub release asset names, e.g. for tag `v2.1.0`:
```
myapp-2.1.0-windows.zip
myapp-2.1.0-linux.tar.gz
myapp-2.1.0-linux.tar.gz.sha256   # optional, strongly recommended
```
A `.sha256` file can contain just the hex digest, or the standard
`sha256sum` format (`<hash>  <filename>`), one line per asset.

`setAssetNameContains()` controls which asset is picked (default: `"win"`
on Windows builds, `"linux"` on Linux builds) — make sure it's a unique
substring of your asset's file name.

---

## 3. Integration

### Build files
Add `githubupdater.h` and `githubupdater.cpp` to your project. `Q_OBJECT` is
used, so make sure moc runs on it (automatic with CMake's `AUTOMOC` or
qmake).

**CMake:**
```cmake
find_package(Qt6 COMPONENTS Core Network REQUIRED) # or Qt5
target_sources(myapp PRIVATE githubupdater.h githubupdater.cpp)
target_link_libraries(myapp PRIVATE Qt6::Core Qt6::Network)
```

### Code
```cpp
#include "githubupdater.h"

// Somewhere with app-lifetime scope (e.g. MainWindow member, or parented to qApp)
auto *updater = new GitHubUpdater("your-org", "your-repo",
                                   QCoreApplication::applicationVersion(),
                                   this);

// Optional configuration:
// updater->setAssetNameContains("linux-x64");
// updater->setMaxDownloadAttempts(3);         // in-memory only, see below
// updater->setInstallDirectory("/opt/myapp"); // defaults to applicationDirPath()
// updater->setIncludePreReleases(false);

connect(updater, &GitHubUpdater::updateAvailable, this, [](const QString &v) {
    qInfo() << "Downloading update" << v << "...";
});

connect(updater, &GitHubUpdater::downloadProgress, this,
        [](qint64 got, qint64 total) { /* update a progress bar */ });

connect(updater, &GitHubUpdater::updateReady, this, [this](const QString &v) {
    // Downloaded + verified. It will be installed automatically the next
    // time the app quits. Tell the user, e.g.:
    auto reply = QMessageBox::information(this, tr("Update ready"),
        tr("Version %1 has been downloaded and will be installed the next "
           "time you close the app. Restart now?").arg(v),
        QMessageBox::Yes | QMessageBox::Later);
    if (reply == QMessageBox::Yes) {
        updater->applyUpdateAndRestart(); // quits the app and installs
    }
});

connect(updater, &GitHubUpdater::insufficientPermissions, this, [this](const QString &dir) {
    QMessageBox::warning(this, tr("Update"),
        tr("Cannot update: no write permission to %1.\n"
           "Try installing the app to a user-writable location, or run it "
           "with elevated privileges once to update.").arg(dir));
});

connect(updater, &GitHubUpdater::updateFailed, this, [](const QString &reason) {
    qWarning() << "Update failed:" << reason;
});

connect(updater, &GitHubUpdater::noUpdateAvailable, this, [](const QString &current) {
    qInfo() << "Already on latest version" << current;
});

// Kick off a check, e.g. on startup or from a "Check for updates" menu item:
updater->checkForUpdates();
```

That's the entire integration surface — one object, six signals, two slots.

---



## 5. Signals reference

| Signal | When |
|---|---|
| `checkStarted()` | `checkForUpdates()` began |
| `noUpdateAvailable(currentVersion)` | already up to date |
| `updateAvailable(newVersion)` | newer release found, download starting |
| `downloadProgress(received, total)` | forwarded from the network reply |
| `updateReady(newVersion)` | downloaded + verified, will install on next quit |
| `updateFailed(reason)` | check, download (after retries), or verification failed |
| `insufficientPermissions(dir)` | install dir not writable; precedes `updateFailed` |

## 6. Slots reference

| Slot | Effect |
|---|---|
| `checkForUpdates()` | starts an async check; no-op if one is already running |
| `applyUpdateAndRestart()` | quits the app now and installs; returns `false` if nothing is ready |

## 7. Known limitations (kept in, on purpose, for simplicity)

- Version comparison splits on `.` and compares numeric components; it's
  correct for plain versions like `1.2.10` (with or without a leading `v`),
  but doesn't implement full semver precedence for pre-release suffixes
  (`1.0.0-beta` isn't ranked below `1.0.0`). Tag your releases with plain
  `MAJOR.MINOR.PATCH` for predictable behavior.
- No silent privilege elevation (see above) — by design.
- Windows extraction relies on the `tar.exe` that ships with Windows 10
  1803+ / Windows 11. If you must support older Windows, ship your own
  `tar`/`unzip` next to the app and adjust the one `tar -xf` line in
  `writeWindowsScript()`.
- `setMaxDownloadAttempts()` is intentionally in-memory only — there is no
  `QSettings` call anywhere in this class, so it can't leak into persisted
  state and always resets to the constructor default (3) on next launch.