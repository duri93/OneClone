# OneClone

A lightweight Windows tray application for managing [rclone](https://rclone.org/) jobs — mounts, syncs, and copies — without touching the command line.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Qt](https://img.shields.io/badge/Qt-6-green)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)

---

## Features

- **Mount, sync, and copy** — start and stop rclone jobs with a click
- **Persistent job definitions** — set up once, use everyday
- **System tray integration** — runs in the background, no need to keep windows open
- **Autostart support** — option to launch the app and any job automatically with windows

---

## Requirements

- **Windows 10 or later** (partial support for linux)
- **rclone** installed and accessible — [download here](https://rclone.org/downloads/)
- **WinFsp** for mounting remote folders locally — [download here](https://winfsp.dev/rel/).

Note: UOulu users can find rclone and WinFsp in Company Portal

---

## Usage

### First run

 :exclamation: RClone must be properly configured before using this app :exclamation:

1. Launch `OneClone.exe`.
2. Go to the **Settings** tab and check the path to `rclone.exe` is correct.
3. Check the box if you want OneClone to start automatically with Windows.
4. Optionally, access the advanced settings and adjust shared transfer settings as needed
5. Click **Save** (if you changed anything).

### Adding a job

1. Go to the **Jobs** tab and click **Add**.
2. Fill in the job name, type, local path, and remote.
3. Click **Save**. The job appears in the job list.

### Starting and stopping

- For **Mount** jobs, click "Mount" to connect the remote to the specified local mounting point.
- For **Sync** and **Copy** jobs, use the **▲** button to send local files to the remote, or the **▼** to download the remote files locally.
- Click **Stop** to gracefully shut down any running job (i.e. rclone process).
- Jobs can also be toggled directly from the system tray context menu.

Note: the difference between copy and sync is that the latter will also delete files in the destination if they are not present in the source.

### Edit job and see output log

Double-click any job in the list to open its detail view, which includes a live output log of everything rclone prints to stdout/stderr.

### Startup with Windows

Check **Launch on Windows start up** in the Settings tab to register the app to start automatically. Every time you login in Windows, the app will start minimized in the tray.

---

## Configuration

All settings are stored in `settings.json` next to the application executable. The file is created automatically on first launch with default values.

### Shared settings

| Setting               | Default                | Description                                         |
|-----------------------|------------------------|-----------------------------------------------------|
| `rclonePath`          | `C:\RClone\rclone.exe` | Path to the rclone executable                       |
| `cacheMode`           | `full`                 | VFS cache mode (`off`, `minimal`, `writes`, `full`) |
| `cacheMaxSize`        | `50` GB                | Maximum VFS cache size                              |
| `cacheMinFreeSpace`   | `50` GB                | Minimum free disk space before cache evicts         |
| `cacheMaxAge`         | `24` hours             | Maximum age of cached files                         |
| `readChunkSize`       | `128` MB               | Initial VFS read chunk size                         |
| `readChunkSizeLimit`  | `2048` MB              | Maximum VFS read chunk size                         |
| `bufferSize`          | `256` MB               | rclone in-memory buffer size                        |
| `transfers`           | `16`                   | Number of parallel file transfers                   |
| `checkers`            | `16`                   | Number of parallel checkers                         |
| `links`               | `true`                 | Follow symlinks (`--links` flag)                    |

### Per-job settings

| Field       | Description                                              |
|-------------|----------------------------------------------------------|
| `name`      | Display name shown in the UI                             |
| `type`      | Job type: `mount`, `sync`, or `copy`                     |
| `local`     | Local path (folder or mount point)                       |
| `remote`    | rclone remote path (e.g. `gdrive:backup`)                |
| `autostart` | Start this job automatically when the app launches       |
| `readOnly`  | Mount in read-only mode (mount jobs only)                |

## Known limitations

- **Windows only.** The graceful stop mechanism uses Win32 APIs (`AttachConsole`, `GenerateConsoleCtrlEvent`) that have no cross-platform equivalent.
- **rclone must be pre-configured.** OneClone does not manage rclone remotes — use `rclone config` in a terminal to set up remotes before using them here.
- **Mount jobs require WinFsp.** rclone FUSE mounts on Windows depend on [WinFsp](https://winfsp.dev/) being installed.
- **No validation on paths or remotes.** Incorrect paths will result in rclone failing at start time; check the output log for details.

---

## License

This is free software under the terms of the Creative Commons [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) license.