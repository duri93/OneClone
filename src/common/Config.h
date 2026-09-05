#pragma once

#include <QRegularExpression>

namespace Config {

    // Application info
    inline constexpr char APP_NAME[]    = "OneClone";
    inline constexpr char APP_VERSION[] = "1.8";
    inline constexpr char APP_ID[]      = "tk.duri.oneclone";
    inline constexpr char APP_AUTHOR[]  = "duri93";

    // Settings file (resolved at runtime relative to exe)
    inline constexpr char SETTINGS_FILENAME[] = "settings.json";

    // Status detection: regex applied against rclone stdout
    inline constexpr char DEFAULT_START_STRING[] = "The service rclone has been started.";

    // display
    inline constexpr int STATUS_DURATION = 5000;
    inline constexpr int SMALL_FONT_SIZE = 8;
    inline constexpr int TRAY_JOB_ICON_SIZE = 16; // per-job status icon size in the tray menu

    //logs
    inline constexpr int MAX_LOG_FILES = 20;
    inline constexpr int LOG_FLUSH_INTERVAL_MS = 1000; // batch per-line flush()es

    // rclone helper invocations (listremotes, lsd, config file) — bounded so
    // a hung/slow rclone process (e.g. waiting on network/credentials)
    // can't block indefinitely.
    inline constexpr int RCLONE_HELPER_TIMEOUT_MS = 15000;

    // process lifecycle timings
    inline constexpr int PROCESS_KILL_WAIT_MS = 1000;          // how long to wait for a killed process to actually exit
    inline constexpr int JOB_STOP_GRACEFUL_TIMEOUT_MS = 3000;  // grace period after a graceful-stop request before falling back to kill()

    // UI polling / debounce timings
    inline constexpr int SETUP_WIZARD_PREREQ_REFRESH_INTERVAL_MS = 2000; // how often SetupWizardPage1 re-checks rclone/WinFsp availability
    inline constexpr int SINGLE_INSTANCE_ACK_TIMEOUT_MS = 200;           // how long a second launch waits for the running instance to ack before giving up

    inline const QRegularExpression WARNING_REGEX{
        "NOTICE:.*failed|ERROR:"
    };

}