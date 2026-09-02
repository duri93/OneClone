#include "AutostartManager.h"

#include "src/core/Status.h"

#include <QCoreApplication>
#include <QSettings>

namespace {
    const char* const kRunKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
}

bool AutostartManager::isEnabled(const QString& appId)
{
    QSettings reg(kRunKey, QSettings::NativeFormat);
    return reg.contains(appId);
}

bool AutostartManager::setEnabled(const QString& appId, bool enabled)
{
    QSettings reg(kRunKey, QSettings::NativeFormat);

    if (enabled) {
        reg.setValue(appId, QCoreApplication::applicationFilePath().replace('/', '\\') + " --tray");
    } else {
        reg.remove(appId);
    }
    reg.sync();

    if (reg.status() != QSettings::NoError) {
        Status::notify("Could not update startup registry entry.", Status::Level::Error);
        return false;
    }
    return true;
}
