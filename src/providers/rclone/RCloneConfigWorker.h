#pragma once

#include "src/providers/rclone/RCloneProvider.h"

#include <QObject>
#include <QString>
#include <QStringList>

// ---------------------------------------------------------------------------
// RCloneConfigWorker
// Runs the blocking RCloneProvider calls used by the setup wizard's config
// page (listRemotes, openConfigFile) on a background thread, so callers
// never block the UI thread waiting on rclone. Intended to be moved to a
// QThread by the caller (see RemotesAutocompleter for the same pattern) and
// driven via queued slot invocations for as long as that thread lives.
// ---------------------------------------------------------------------------
class RCloneConfigWorker : public QObject
{
    Q_OBJECT

public:
    explicit RCloneConfigWorker(RCloneProvider* rcloneProvider, QObject* parent = nullptr);

public slots:
    void fetchRemotes(const QString& rclonePath);
    void openConfigFile(const QString& rclonePath);

signals:
    void remotesReady(const QStringList& remotes);
    void configFileOpened(bool ok);

private:
    RCloneProvider* m_rcloneProvider;
};
