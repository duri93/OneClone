#pragma once

#include <QString>

// ---------------------------------------------------------------------------
// AutostartManager
// Registers or unregisters the application to launch at Windows login, via
// the HKCU "Run" registry key.
// ---------------------------------------------------------------------------
class AutostartManager
{
public:
    // Returns true if appId is currently registered to launch at login.
    static bool isEnabled(const QString& appId);

    // Registers (or unregisters) appId to launch at login. Returns false
    // if the registry key could not be written — callers are responsible
    // for reporting that to the user (e.g. via Status), since this is a
    // platform/model-layer class that doesn't reach into the UI itself.
    static bool setEnabled(const QString& appId, bool enabled);
};