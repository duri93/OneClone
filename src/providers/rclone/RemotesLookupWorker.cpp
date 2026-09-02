#include "RemotesLookupWorker.h"

RemotesLookupWorker::RemotesLookupWorker(RCloneProvider* rcloneProvider, QString rclonePath)
    : m_rcloneProvider(rcloneProvider), m_rclonePath(std::move(rclonePath)) {}

void RemotesLookupWorker::run(){
    const QStringList remotes = m_rcloneProvider->listRemotes(m_rclonePath);

    if (!remotes.isEmpty()) {
        emit remotesReady(remotes);
    }

    for (const QString &remote : remotes) {
        const QStringList dirs = m_rcloneProvider->listDirs(m_rclonePath, remote);

        if (!dirs.isEmpty()){
            emit dirsReady(remote, dirs);
        }
    }

    emit finished();
}
