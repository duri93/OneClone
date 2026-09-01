#include "RemotesLookupWorker.h"
#include <QProcess>
#include <QStringList>
#include <QRegularExpression>

RemotesLookupWorker::RemotesLookupWorker(QString rclonePath) : m_rclonePath(std::move(rclonePath)) {}

void RemotesLookupWorker::run(){
#ifdef Q_OS_WIN
    const QStringList remotes = listRemotes(m_rclonePath);

    if (!remotes.isEmpty()) {
        emit remotesReady(remotes);
    }

    for (const QString &remote : remotes) {
        const QStringList dirs = listDirs(m_rclonePath, remote);

        if (!dirs.isEmpty()){
            emit dirsReady(remote, dirs);
        }
    }
#else
    qDebug() << "RemotesAutocompleter is only implemented on Windows";
#endif
    emit finished();
}

bool RemotesLookupWorker::runRclone(const QString &rclonePath, const QStringList &arguments, QString *stdoutText){
#ifdef Q_OS_WIN
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
#else
    Q_UNUSED(rclonePath)
    Q_UNUSED(arguments)
    Q_UNUSED(stdoutText)
    return false;
#endif
}

QStringList RemotesLookupWorker::listRemotes(const QString &rclonePath){
    QString output;

    if (!runRclone(rclonePath, {"listremotes"}, &output)){
        return {};
    }

    QStringList remotes;

    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        QString remote = line.trimmed();

        if (remote.endsWith(':'))
            remote.chop(1);

        if (!remote.isEmpty()){
            remotes.append(remote);
        }

    }

    return remotes;
}

QStringList RemotesLookupWorker::listDirs(const QString &rclonePath, const QString &remote){
    QString output;

    if (!runRclone(rclonePath, {"lsd", remote + ":"}, &output))
        return {};

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

