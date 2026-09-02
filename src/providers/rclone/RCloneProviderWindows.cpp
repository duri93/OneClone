#include "RCloneProviderWindows.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

bool RCloneProviderWindows::isAvailable(const QString& rclonePath) const
{
    QFileInfo fi(rclonePath);
    return (fi.exists() && fi.isExecutable()) || !QStandardPaths::findExecutable(rclonePath).isEmpty();
}

QString RCloneProviderWindows::resolveExecutable(const QString& rclonePath) const
{
    QFileInfo fi(rclonePath);
    if (fi.exists() && fi.isExecutable()) {
        return rclonePath;
    }

    QString found = QStandardPaths::findExecutable(rclonePath);
    if (!found.isEmpty()) {
        return found;
    }

    // Nothing better found — return as-is and let the caller fail to start.
    return rclonePath;
}

QProcess* RCloneProviderWindows::openConfig(const QString& rclonePath, QObject* parent) const
{
    auto* process = new QProcess(parent);
    // auto-cleanup once the process is done, regardless of who else is
    // listening to ::finished()
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                      process, &QProcess::deleteLater);

    const QString program = "cmd.exe";
    const QStringList arguments = {"/c", "start", "rclone config", "/wait", rclonePath, "config"};

    process->start(program, arguments);
    if (!process->waitForStarted()) {
        process->deleteLater();
        return nullptr;
    }
    return process;
}

bool RCloneProviderWindows::openConfigFile(const QString& rclonePath) const
{
    QString output;
    if (!runRclone(rclonePath, {"config", "file"}, &output)) {
        return false;
    }

    // rclone outputs something like:
    // Configuration file is stored at:
    // C:\Users\User\AppData\Roaming\rclone\rclone.conf
    //
    // Extract the last non-empty line.
    const QStringList lines = output.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        return false;
    }

    const QString configPath = lines.last().trimmed();
    return QDesktopServices::openUrl(QUrl::fromLocalFile(configPath));
}

QStringList RCloneProviderWindows::listRemotes(const QString& rclonePath) const
{
    QString output;
    if (!runRclone(rclonePath, {"listremotes"}, &output)) {
        return {};
    }

    QStringList remotes;
    for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
        QString remote = line.trimmed();
        if (remote.endsWith(':')) {
            remote.chop(1);
        }
        if (!remote.isEmpty()) {
            remotes.append(remote);
        }
    }
    return remotes;
}

QStringList RCloneProviderWindows::listDirs(const QString& rclonePath, const QString& remote) const
{
    QString output;
    if (!runRclone(rclonePath, {"lsd", remote + ":"}, &output)) {
        return {};
    }

    QStringList dirs;
    static const QRegularExpression re(R"(^\s*\S+\s+\S+\s+\S+\s+\S+\s+(.*?)\s*$)");

    for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
        const QRegularExpressionMatch match = re.match(line.trimmed());
        if (!match.hasMatch()) {
            continue;
        }

        const QString dir = match.captured(1).trimmed();
        if (!dir.isEmpty()) {
            dirs.append(dir);
        }
    }
    return dirs;
}

bool RCloneProviderWindows::runRclone(const QString& rclonePath, const QStringList& arguments, QString* stdoutText)
{
    QProcess process;
    process.start(rclonePath, arguments);

    if (!process.waitForStarted()) {
        return false;
    }
    if (!process.waitForFinished(-1)) {
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return false;
    }

    *stdoutText = QString::fromLocal8Bit(process.readAllStandardOutput());
    return true;
}
