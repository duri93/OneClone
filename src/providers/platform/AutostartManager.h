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

    // Registers (or unregisters) appId to launch at login. Returns false and
    // notifies via Status if the registry key could not be written.
    static bool setEnabled(const QString& appId, bool enabled);
};