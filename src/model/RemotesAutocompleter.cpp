// RemotesList.cpp

#include "RemotesAutocompleter.h"

#include <QProcess>
#include <QRegularExpression>
#include <QStringList>

bool RemotesAutocompleter::runRclone(const QString &rclonePath, const QStringList &arguments, QString *stdoutText){
#ifdef Q_OS_WIN
    QProcess process;
    process.start(rclonePath, arguments);

    if (!process.waitForStarted())
        return false;

    if (!process.waitForFinished(-1))
        return false;

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0){
        return false;
    }

    *stdoutText = QString::fromLocal8Bit(process.readAllStandardOutput());
    return true;
#else
    Q_UNUSED(rclonePath)
    Q_UNUSED(arguments)
    Q_UNUSED(stdoutText)

    return false;
#endif
}

QStringList RemotesAutocompleter::listRemotes(const QString &rclonePath){
    QString output;

    if (!runRclone(rclonePath, {"listremotes"}, &output))
        return {};

    QStringList remotes;

    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        QString remote = line.trimmed();

        if (remote.endsWith(':'))
            remote.chop(1);

        if (!remote.isEmpty())
            remotes.append(remote);
    }

    return remotes;
}

QStringList RemotesAutocompleter::listDirs(const QString &rclonePath, const QString &remote){
    QString output;

    if (!runRclone(rclonePath, {"lsd", remote + ":"}, &output)) return {};

    QStringList dirs;

    static const QRegularExpression re(R"(^\s*\S+\s+\S+\s+\S+\s+\S+\s+(.*?)\s*$)");

    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        const QString trimmed = line.trimmed();

        QRegularExpressionMatch match = re.match(trimmed);

        if (!match.hasMatch())
            continue;

        const QString dir = match.captured(1).trimmed();

        if (!dir.isEmpty())
            dirs.append(dir);
    }

    return dirs;
}

QStringList RemotesAutocompleter::buildEntries(const QString &rclonePath){
    QStringList entries;

    const QStringList remotes = listRemotes(rclonePath);

    for (const QString &remote : remotes) {
        const QStringList dirs = listDirs(rclonePath, remote);

        for (const QString &dir : dirs) {
            entries.append(remote + ":" + dir);
        }
    }

    entries.removeDuplicates();
    entries.sort(Qt::CaseInsensitive);

    return entries;
}

QCompleter *RemotesAutocompleter::setup(QLineEdit *lineEdit, const QString &rclonePath){
#ifdef Q_OS_WIN

    if (!lineEdit)
        return nullptr;

    const QStringList entries = buildEntries(rclonePath);

    auto *completer = new QCompleter(entries, lineEdit);

    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchStartsWith);
    completer->setCompletionMode(QCompleter::PopupCompletion);

    lineEdit->setCompleter(completer);

    return completer;

#else

    Q_UNUSED(lineEdit)
    Q_UNUSED(rclonePath)

    return nullptr;

#endif
}