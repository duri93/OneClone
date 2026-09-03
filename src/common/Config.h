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

    //logs
    inline constexpr int MAX_LOG_FILES = 20;
    inline constexpr int LOG_FLUSH_INTERVAL_MS = 1000; // batch per-line flush()es

    // rclone helper invocations (listremotes, lsd, config file) — bounded so
    // a hung/slow rclone process (e.g. waiting on network/credentials)
    // can't block indefinitely.
    inline constexpr int RCLONE_HELPER_TIMEOUT_MS = 15000;

    inline const QRegularExpression WARNING_REGEX{
        "NOTICE:.*failed|ERROR:"
    };

}