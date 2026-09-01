#pragma once

#include <QObject>

class RemotesLookupWorker : public QObject
{
    Q_OBJECT

public:
    explicit RemotesLookupWorker(QString rclonePath);

public slots:
    void run();

signals:
    void remotesReady(const QStringList &remotes);
    void dirsReady(const QString &remote, const QStringList &dirs);
    void finished();

private:
    static bool runRclone(const QString &rclonePath, const QStringList &arguments, QString *stdoutText);
    static QStringList listRemotes(const QString &rclonePath);

    static QStringList listDirs(const QString &rclonePath, const QString &remote);

    QString m_rclonePath;
};
