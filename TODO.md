# IMPLEMENTATION ROADMAP

TODO
- Page 2
  - QTimer to update remotes every second
  - List remotes to label
  - button click event for rclone conifg
  - button click event for allas config (show/hide frame)
  - button click event save allas config
  - link to allas guide
  - also move isRcloneInstalled and isWinFspInstalled in manager
- Page 3 (guide)
- QWizard
  - wizard itself
  - start routine
  - start button

## PRIORITY 1
Tests and bugs that should be fixed.

## PRIORITY 2
Additional small features.

- **guide.md** for general setup (not allas)
- **start guided setup** if no config file
- **guided setup** using QWizard
  - Check for rclone (show/edit exe path) and winfsp. Show download links or uoulu instructions if not found
  - List rclone remotes. Buttons for general rclone config or Allas rclone config. First opens cmd, latter shows page with name, key and secret input, link to allas setup guide, and appends to config.dat. !NOTE: Need a way to check it works!
  - Guide on how to create a job

- **Bandwidth limiting** (`--bwlimit`) and other advanced rclone flags not currently exposed in Settings.
- **Better progress number formatting** by rounding to significant digits (`Job::processLineProgress`)


## PRIORITY 3
Optional things to implement.

- **In-app rclone remote configuration editor**, rather than relying entirely on an external `rclone config` session.
- **Localization/i18n** — `tr()` is used throughout, but no translation files currently ship.









## LINUX COMPATIBILITY ISSUES TO FIX

| Location | Issue |
|---|---|
| `model/Job.cpp` — `Job::stop()` | Uses `AttachConsole` / `GenerateConsoleCtrlEvent(CTRL_C_EVENT)` / `SetConsoleCtrlHandler` / `FreeConsole` (all Win32 API, `#ifdef Q_OS_WIN`) to gracefully stop rclone. The `#else` branch just calls `m_process.kill()` (SIGKILL). This is not a "graceful stop" equivalent — on Linux this should send `SIGINT` (`kill(pid, SIGINT)` or `QProcess::terminate()`) first and only `kill()` as a fallback, especially for `mount` jobs where a SIGKILL can leave a stale FUSE mountpoint. |
| `view/MainWindow.cpp` — `loadSettingsToUi()` / `saveUiToSettings()` | Autostart-on-login is implemented purely via the Windows registry (`QSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", ...)`). This has no effect at all on Linux — the checkbox will silently do nothing. Needs a Linux implementation (e.g. writing/removing a `~/.config/autostart/oneclone.desktop` file). |
| `view/MainWindow.cpp` — `saveUiToSettings()` | `QCoreApplication::applicationFilePath().replace('/', '\\')` hardcodes Windows path separators when building the autostart command line. Needs to be conditional on platform (or removed/use `QDir::toNativeSeparators`). |
| `view/MainWindow.cpp` — `isWinFspInstalled()` | Entirely Windows-specific (registry check + `Program Files` paths). The `#else` branch just `return false;`, meaning on Linux the "WinFSP not installed" error banner is **always shown**, even though WinFsp is irrelevant on Linux. Needs a Linux-appropriate check (e.g. detect FUSE / `fusermount` availability) or the banner/check should be skipped entirely on non-Windows builds. |
| `model/SharedSettings.h` | Default `m_rclonePath = "C:\\RClone\\rclone.exe"` — a Windows-only default path. Should be platform-conditional (e.g. `"/usr/bin/rclone"` or simply `"rclone"` relying on `PATH` on Linux). |
| `view/MainWindow.cpp` — `onRcloneSelectClicked()` | File dialog filter is hardcoded to `"Executable (*.exe);;All files (*.*)"`. On Linux rclone binaries have no `.exe` extension, so the primary filter is useless there. |
| `CMakeLists.txt` / `main.rc` | `main.rc` (a Windows `.ico` resource script) is added unconditionally to `qt_add_executable(...)` sources. `.rc` compilation is a Windows/MSVC-toolchain-only concept; on Linux this file should be wrapped in `if(WIN32) ... endif()` or added conditionally, or the Linux build may fail/behave unpredictably depending on the generator. |
| `view/MainWindow.ui` | Tooltip text for the "local" field hardcodes Windows-style paths/examples (`C:\Users\Desktop\My Folder`, drive letters `E:\`, UNC paths `\\allas\folder`) with no Linux-path guidance (e.g. `/mnt/...`). Not a functional bug, but confusing to Linux users. |
| `view/MainWindow.ui` | Several rich-text labels hardcode the font family `Segoe UI`, which doesn't exist on Linux (will silently fall back, but is worth normalizing/removing for a cross-platform build). |

> Note: `model/UpdateManager.cpp` is already well cross-platform — it has real `Q_OS_WIN` / `Q_OS_LINUX` branches for both the update-script generation (`writeWindowsScript` / `writeLinuxScript`) and asset selection (`"win"` vs `"linux"` substring filtering), so it does **not** need rewriting, just verification.


