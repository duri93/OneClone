#include "WinFspDetector.h"

#include <QDir>
#include <QSettings>

bool WinFspDetector::isAvailable() const
{
#ifdef Q_OS_WIN
    // Method 1: check the registry
    QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\WinFsp", QSettings::NativeFormat);
    if (!reg.allKeys().isEmpty()) {
        return true;
    }

    // Method 2: check default install paths
    const QStringList paths = {
        "C:/Program Files (x86)/WinFsp",
        "C:/Program Files/WinFsp"
    };
    for (const QString& path : paths) {
        if (QDir(path).exists()) {
            return true;
        }
    }
    return false;
#else
    return false; // WinFsp is Windows-only
#endif
}
