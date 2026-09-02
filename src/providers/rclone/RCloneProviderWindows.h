#pragma once

#include "src/providers/rclone/RCloneProvider.h"

#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QObject;
class QProcess;
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// RCloneProviderWindows
// Windows implementation of RCloneProvider: resolves rclone.exe, drives the
// interactive config tool via cmd.exe, and shells out to rclone for remote
// and directory listings.
// ---------------------------------------------------------------------------
class RCloneProviderWindows : public RCloneProvider
{
public:
    bool isAvailable(const QString& rclonePath) const override;
    QString resolveExecutable(const QString& rclonePath) const override;
    QProcess* openConfig(const QString& rclonePath, QObject* parent = nullptr) const override;
    bool openConfigFile(const QString& rclonePath) const override;
    QStringList listRemotes(const QString& rclonePath) const override;
    QStringList listDirs(const QString& rclonePath, const QString& remote) const override;

private:
    // Runs rclone synchronously and captures stdout. Returns false on
    // failure to start, non-zero exit, or abnormal termination.
    static bool runRclone(const QString& rclonePath, const QStringList& arguments, QString* stdoutText);
};