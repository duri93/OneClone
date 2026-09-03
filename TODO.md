- settings cancel button
- settings unsaved warning

# Unintuitive / odd behaviour (user viewpoint)

3. The Settings tab has no "unsaved changes" indicator (unlike the Job Details tab's ui->unsaved label via updateUnsavedIndicator), so navigating away after editing rclone path/VFS settings without clicking Save discards changes with no warning.

4. MainWindow::MainWindow treats a missing settings file on first run as a load failure (dryRun = !m_appContext.load()) and shows "Could not load settings file — using defaults." as a Warning status even though this is the expected, benign first-run state.

5. TrayController::onActivated uses m_window->isVisible() to decide whether clicking the tray icon should show or hide the window; a minimized window is still "visible" in Qt's sense, so clicking the tray icon while minimized will hide it instead of restoring it.

6. Starting a sync/copy job from the tray menu (TrayController::onMenuAboutToShow, job->toggle()) bypasses the confirmation dialog that JobWidget::buttonClicked/showConfirm shows in the main window, even though sync/copy always runs with the destructive --delete-before flag.

7. The generated rclone command always includes --delete-before for sync/copy jobs (RCloneProvider::buildCommand) with no UI option to disable it, so any sync/copy job is inherently destructive by default.

8. On Windows the auto-updater's install script never relaunches the application (the start "" "%APP_EXE%" line in UpdateManager::writeWindowsScript is commented out with ::), while the equivalent Linux script does relaunch it (nohup "$APP_EXE" ... in writeLinuxScript) — inconsistent post-update behaviour across platforms.

9. Any UpdateManager::updateFailed (including a simple offline/network hiccup on startup) pops up a visible ui->updateFrame banner with a raw technical error string, which is noisy/alarming for a routine, non-critical background check.


# Bugs

10. Manually stopping a job almost always ends up shown as "Errored" instead of "Stopped": Job::stop() (and its Windows CTRL+C fallback) ultimately calls m_process.kill(), which Qt guarantees emits errorOccurred(QProcess::Crashed) before finished(); Job::onProcessError unconditionally calls setStatus(JobStatus::Errored) with no check for the intentional Stopping state, and onProcessFinished's if (m_status == JobStatus::Errored) // leave as-is branch then locks in the wrong status instead of transitioning to Stopped.

11. JobListWidget::dropEvent calls qobject_cast<JobWidget*>(w)->job()->id() without null-checking the cast result, which would crash if the layout ever contains a non-JobWidget child.

12. JobListWidget::dropEvent has an unconditional targetIndex--; after the index-adjustment loop that looks like an unexplained off-by-one patch, making the drop-index math hard to trust/verify.

13. SingleInstanceGuard::listen() never checks the return value of m_server->listen(m_appId); if listening fails (e.g. a stale lock the removal didn't clear), subsequent app launches will silently start second instances instead of notifying the first.

14. AppContext::load() returns false both for "file didn't exist yet" and for real read/parse errors, and MainWindow reacts identically to both cases (generic "using defaults" warning), so a genuinely corrupt settings file is indistinguishable from a fresh install and the user isn't told their existing jobs/settings were just wiped.

15. In JobDetailsTabController::onSaveClicked, the Job setters (setName, setType, etc.) mutate the live Job object before AppContext::save() is even attempted, so if the save to disk fails the in-memory job has still changed — the "Error saving job" message doesn't reflect a rolled-back state.


# Failure points not properly handled/displayed to the user

16. RCloneProviderWindows::runRclone calls process.waitForFinished(-1) (infinite blocking wait) and is invoked synchronously on the UI thread from openConfigFile, listRemotes, and SetupWizardPage2::refreshRemotes/initializePage; a hung or slow rclone process (e.g. waiting on network/credentials) will freeze the whole UI with no way to cancel.

17. RCloneProviderWindows::openConfig launches cmd.exe /c start "rclone config" /wait rclonePath config and only checks waitForStarted(); if cmd.exe itself fails after that point or the inner rclone config command errors, there is no feedback to the user beyond a process that silently does nothing.

18. Job::start() never surfaces a failure of LogFile::open() beyond forwarding a generic warning via emit warning(...), so a logging failure and a real rclone warning line are shown through the exact same UI channel (the status-icon tooltip), making them hard to distinguish.

19. AutostartManager::setEnabled and AppContext::save() failures are reported through the transient status bar (Status::notify, 5s timeout via Config::STATUS_DURATION), which can easily be missed for a persistent/important failure like "your settings didn't save."

20. UpdateManager::isDirectoryWritable uses a QTemporaryFile probe whose failure could stem from many causes (permissions, disk full, path too long), but the user is only ever told "Install directory is not writable", losing the actual underlying reason from the OS.

21. Job::onProcessFinished distinguishes "already Errored" vs "Stopping" vs "nonzero exit" vs "success" but never surfaces why an errored exit happened beyond the generic "[ERROR] Process exited with code %1" line buried in the output stream — there's no structured summary of the failure for the user to act on.


# Odd / non-professional code logic, structure, organization

22. JobsTabController::onJobAdded and JobsTabController::findOrCreateJobWidget both construct a JobWidget, set the same openDetailsRequested connection, and follow nearly identical logic — this duplicated widget-creation code should be a single shared helper.

23. AppContext hardcodes concrete RCloneProviderWindows/WinFspDetector implementations in its constructor with a // TODO: pick these based on the host platform comment, even though the abstract RCloneProvider/MountBackendDetector interfaces exist specifically to support platform selection — the polymorphism is defined but never actually exploited via a factory.

24. Despite the above, CMakeLists.txt unconditionally compiles RCloneProviderWindows.cpp (which shells out to cmd.exe, defaults to C:\RClone\rclone.exe) into a build that also declares MACOSX_BUNDLE and links Windows-only resource files (resources/main.rc), and UpdateManager has real Linux install-script logic — the codebase is simultaneously structured for cross-platform support and hardwired to Windows-only behavior.

25. Job reaches directly into SharedSettings via a long list of individual getters (m_shared->cacheMode(), ->cacheMaxSize(), ... ten+ fields) in both Job::getCommand and JobDetailsTabController::updateCommandPreview, duplicating the same parameter-copying block in two places instead of having SharedSettings populate an RcloneCommandParams itself.

26. Job::stop() contains a #ifdef Q_OS_WIN block with raw WinAPI calls (AttachConsole, SetConsoleCtrlHandler, GenerateConsoleCtrlEvent) and a file-static std::atomic<int> s_ctrlSuppressCount living directly inside the platform-agnostic Job class, instead of being encapsulated behind the RCloneProvider/platform abstraction the rest of the app uses.

27. Status is implemented as a global Meyer's-singleton event bus (Status::instance()), which works but is a form of hidden global state/dependency that bypasses the otherwise fairly clean signal/slot wiring used elsewhere (e.g. AppContext::added/removed), making it harder to trace who is notifying whom.

28. JobWidget::onSpecChange/onStatusChange branch repeatedly on m_job->type() == "mount" / "copy" / "sync" string comparisons in several places (also duplicated in Job::processLine and RCloneProvider::buildCommand) rather than using the existing JobStatus-style enum for job type, risking typos and scattering the same string literals across the codebase.

29. SetupWizard.cpp's comment referencing "WA_DeleteOnClose above" doesn't match the actual code (that attribute is never set in the file), indicating stale/copy-pasted documentation that will mislead future maintainers about the wizard's cleanup mechanism.

30. LogFile::cleanupOldLogs and log writing happen synchronously on the main thread for every output line (m_file.flush() after every write), which is a minor but real per-line I/O cost that could be batched/buffered given jobs can emit lines rapidly during transfers.


# Other possible improvements (resilience/robustness without overcomplicating)

31. Move the blocking RCloneProviderWindows::runRclone calls (listRemotes, listDirs, openConfigFile) off the UI thread consistently — RemotesAutocompleter/RemotesLookupWorker already show the right pattern; SetupWizardPage2::refreshRemotes and openConfigFile should reuse it instead of blocking.

32. Add a bounded timeout to runRclone's waitForFinished(-1) so a hung rclone process can't wedge the calling thread indefinitely (even on the worker thread, this can leak/hang the app on shutdown).

33. Fix the stop/Errored status bug (item 10) by having Job::onProcessError ignore/short-circuit QProcess::Crashed while m_status == JobStatus::Stopping, so intentional stops are reliably reported as Stopped.

34. Introduce a small JobType enum (mount/sync/copy) instead of comparing raw QString literals throughout Job, JobWidget, and RCloneProvider, reducing risk of typos and centralizing the valid-type list.

35. Centralize "apply pending UI edits + call AppContext::save()" into a single method on AppContext or a small controller helper so every settings-changing surface (Job Details, Settings tab, Setup Wizard) behaves consistently instead of three different ad hoc save paths.

36. Have AppContext::load() distinguish "no file yet" from "parse/read error" (e.g. via an enum/error struct) so MainWindow can tell the user precisely when their existing config was unreadable/corrupt versus this being a fresh install.

37. Consider making --delete-before a configurable, opt-in setting per job (defaulting to off or clearly warned) rather than an unconditional hardcoded flag, to reduce the risk of unintended data loss on sync/copy jobs.

38. Add basic reconnection/backoff and a visible retry affordance for UpdateManager failures that are clearly transient (network errors) versus fatal ones (checksum mismatch, permissions), rather than routing all failures through the same generic banner.

39. Extract the repeated RcloneCommandParams population logic (Job::getCommand and JobDetailsTabController::updateCommandPreview) into a single SharedSettings::toCommandParams()-style helper to avoid the two call sites drifting out of sync as fields are added.


























# IMPLEMENTATION ROADMAP



## PRIORITY 1
Tests and bugs that should be fixed.
- check and fix tooltips everywhere
- rework includes
- merge branches
- version bump

## PRIORITY 2
Additional small features.

- **guide.md** for general setup (not allas)
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








------------------------------------------------------------------------------

1. Make AutostartManager work on Linux too, via an XDG ~/.config/autostart/*.desktop file. Choose if doing this directly in AutostartManager or creating two derived classes (Windows and Linux versions), depending on how deeply different the logic will be.
2. Implement RCloneProviderLinux: locate the rclone binary via PATH/standard install locations, and launch rclone config inside a detected terminal emulator (try gnome-terminal/konsole/xterm in sequence) since it's an interactive TUI, replacing the Windows cmd.exe /c start ... /wait trick.
3. Implement the Linux side of the mount-backend detector, checking for FUSE/fusermount availability instead of WinFsp.
4. Replace the Windows-only console-ctrl stop sequence in Job::stop() (AttachConsole/GenerateConsoleCtrlEvent) with a POSIX-appropriate graceful stop (e.g. SIGINT via QProcess::terminate()), keeping the same public stop() API.
5. Make binary-name/default-path assumptions platform-aware (rclone.exe filters and default paths in LocalPathAutocompleter/file dialogs) so Linux uses the extension-less rclone name.
6. Add a Linux build target and packaging (CMake/.pro conditionals, e.g. AppImage or .deb), and confirm UpdateManager's existing .tar.gz release handling actually gets exercised by a real Linux CI build.
7. Manual QA on at least one GTK-based (GNOME) and one Qt-based (KDE) desktop to confirm QSystemTrayIcon and native file dialogs behave as expected, since these vary more across Linux desktop environments than across Windows versions.

All these implementations with proper code and logic structure, but don't abstract the logic too much, try to keep the code simple. Ensure proper error handling takes place, and wire it through Status.


Implement one step at a time, then wait for me to say next. Only edit the specific code parts that pertain to the current step. Use TODO/FIXME comments in the code when relevant, if you know they will/should be addressed later. As output message, give a one sentence short description of what you did in the current step (like the description of a commit), then flag some details only if they are relevant (ideally none).
