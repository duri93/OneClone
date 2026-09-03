#include "RCloneConfigWorker.h"

RCloneConfigWorker::RCloneConfigWorker(RCloneProvider* rcloneProvider, QObject* parent)
    : QObject(parent), m_rcloneProvider(rcloneProvider) {}

void RCloneConfigWorker::fetchRemotes(const QString& rclonePath)
{
    emit remotesReady(m_rcloneProvider->listRemotes(rclonePath));
}

void RCloneConfigWorker::openConfigFile(const QString& rclonePath)
{
    emit configFileOpened(m_rcloneProvider->openConfigFile(rclonePath));
}
