
# IMPLEMENTATION ROADMAP


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


