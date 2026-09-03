Duplicated / repeated logic

RCloneConfigWorker (added this session) and RemotesLookupWorker both move blocking RCloneProvider calls to a background QThread, but use two different, unrelated lifecycle patterns (one-shot thread vs. a persistent thread driven by queued slots) for essentially the same job. — Consolidate them into one reusable async-runner class that supports both usage styles, shared by RemotesAutocompleter and SetupWizardPage2.

SettingsTabController::onRcloneConfClicked re-implements the same "launch openConfig(), null-check, notify on failure" sequence already written in SetupWizardPage2. — Factor that sequence into one shared helper (free function or small class) used by both call sites.

JobsTabController::onJobRemoved and AppContext::removeJob(QString) each independently implement the same "linear-scan a list by job id and remove" loop. — Expose an AppContext::indexOfJob(id) helper that both can call instead of duplicating the scan.

Job type ("mount"/"copy"/"sync") is compared as raw string literals in at least four separate files (Job.cpp, JobWidget.cpp, JobDetailsTabController.cpp, RCloneProvider.cpp), so a typo in any one of them silently breaks that spot only. — Introduce a JobType enum with to/from-string conversion helpers, and use it everywhere instead of comparing QStrings.

Direct coupling that should go through signals/abstraction

TrayController reaches directly into JobsTabController::jobWidgets() and calls JobWidget::getStatusIcon()/Job::toggle(), coupling the tray menu to Jobs-tab widget internals it doesn't otherwise need. — Have TrayController iterate AppContext's Job list (or a lightweight summary of it) directly instead of depending on JobWidget.

Job (a core/model class) calls the global Status::notify() singleton directly for some conditions (log errors, "no local folder configured") while using its own warning/outputLine signals for others, mixing two different reporting channels inside one class. — Have Job only ever emit its own signals, and let a UI-layer listener translate those into Status::notify calls, keeping the model decoupled from the notification bus.

Status is a global singleton that low-level, non-UI classes (Job, AutostartManager, etc.) call into directly, creating hidden dependencies from the model/platform layer straight into a UI broadcast channel. — Route such notifications through signals emitted by the model layer, with a thin adapter at the UI boundary forwarding them to Status.

Inconsistencies / stale comments

SetupWizard's header comment claims the wizard cleans itself up "via WA_DeleteOnClose", but that attribute is never actually set anywhere in the code, so the comment describes behavior that doesn't exist. — Either set Qt::WA_DeleteOnClose on the QWizard to match the comment, or correct the comment to describe the actual finished → deleteLater wiring.

UpdateManager.cpp leaves several // handleFatalFailure(tr(...)) lines commented out directly above the plain, non-translatable string actually used, mixing translatable and non-translatable user-facing text inconsistently and leaving dead code in place. — Pick one approach (translatable via tr()) consistently and delete the commented-out alternatives.

JobsTabController::onJobMoved reports a save failure with the text "Warning: failed to save settings." but at Status::Level::Error, so the wording and severity disagree. — Make the message text match the actual level used.

Several classes (Job, SharedSettings) declare getters returning const bool/const int/const QString, which is non-idiomatic C++ — the top-level const on a by-value return has no real effect and blocks move semantics for QString. — Drop the top-level const on these return types.

Magic timing constants are inconsistently located — some centralized in Config.h (RCLONE_HELPER_TIMEOUT_MS, LOG_FLUSH_INTERVAL_MS), others left as raw literals inline (the 3000ms stop-fallback timer in Job::stop(), the 1000ms waits in destructors). — Move every tunable timing value into Config.h for one consistent source of truth.

Design/robustness gaps

RCloneProviderWindows::openConfig always launches through cmd.exe /c start ... /wait, whose exit code doesn't reliably reflect whether the wrapped rclone config process actually succeeded (already flagged as a TODO from the earlier task), making success/failure detection for a core feature structurally unreliable. — Launch rclone.exe config directly, without the cmd.exe/start wrapper, so the real exit code is available.

RcloneCommandParams's type/readOnly/deleteBefore fields are only loosely documented via comments ("mount only", "sync only") rather than enforced by the type system, so nothing prevents e.g. setting deleteBefore on a mount job. — Combine with the JobType enum fix (item 8) and consider grouping mount-only vs. sync/copy-only fields so invalid combinations aren't representable.

Job manages m_logfile as a raw pointer with manual new/delete spread across start() and onProcessFinished(), making its lifetime easy to get wrong in future edits (this is also what enables bug #1). — Hold m_logfile in a std::unique_ptr<LogFile> (or rely solely on Qt parent/child ownership) instead of manual delete calls.

JobWidget::getStatusIcon()/getJobIcon() re-load and re-render the relevant SVG from disk/resources on every single status/progress update, even though only a handful of icon variants ever exist. — Cache the rendered QPixmap per (type, status, active) combination instead of re-decoding the SVG every refresh.














# IMPLEMENTATION ROADMAP

---------
You are an expert developer in Qt c++. I want you to analyze this codebase and list:

    Unintuitive or odd behaviour (from the user viewpoint)
    Bugs
    Failure points that are not properly handled and displayed to the user
    Odd / non-professional code logic, structure or organization (e.g., repeated code blocks, or direct dependencies when a signal/slot or other system would make more sense, or other)
    Other possible code improvements, with the goal of making the code more resilient, less brittle, while avoiding overcomplication and keeping it simple to understand.

Create a numbered list of points you identify, using one sentence for each point and referring to specific classes/methods when applicable. Do not edit code yet.
---------

## PRIORITY 1
Tests and bugs that should be fixed.


## PRIORITY 2
Additional small features.

- split job class in subclasses: JobMount, JobCopy, JobSync
- uniformize slot names and event handlers names (list and ask me)

- tray icon menu, each job has sub-entry for sync up or down.
- **guide.md** for general setup (not allas)
- **Bandwidth limiting** (`--bwlimit`) and other advanced rclone flags not currently exposed in Settings.
- Linux port


## PRIORITY 3
Optional things to implement.

- **Better progress number formatting** by rounding to significant digits (`Job::processLineProgress`)
- **Localization/i18n** — `tr()` is used throughout, but no translation files currently ship.


















