#pragma once

#include "src/providers/rclone/RCloneProvider.h"

#include <QObject>
#include <QString>
#include <QStringList>

// ---------------------------------------------------------------------------
// RemotesLookupWorker
// Runs on a background thread and asks an RCloneProvider for the configured
// remotes and their top-level directories, emitting results incrementally.
// A one-shot job: run() does the whole lookup and finishes once. See
// AsyncRunner, which handles moving this onto a background thread and
// stopping that thread once finished() fires.
// ---------------------------------------------------------------------------
class RemotesLookupWorker : public QObject
{
    Q_OBJECT

public:
    explicit RemotesLookupWorker(RCloneProvider* rcloneProvider, QString rclonePath);

public slots:
    void run();

signals:
    void remotesReady(const QStringList &remotes);
    void dirsReady(const QString &remote, const QStringList &dirs);
    void finished();

private:
    RCloneProvider* m_rcloneProvider;
    QString m_rclonePath;
};
