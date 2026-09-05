#include "SingleInstanceGuard.h"

#include "src/common/Config.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

SingleInstanceGuard::SingleInstanceGuard(QString appId, QObject* parent)
    : QObject(parent)
    , m_appId(std::move(appId))
{
}

SingleInstanceGuard::~SingleInstanceGuard() = default;

bool SingleInstanceGuard::tryNotifyExisting()
{
    QLocalSocket socket;
    socket.connectToServer(m_appId);
    if (!socket.waitForConnected(500)) {
        return false;
    }

    // Another instance is running — tell it to show itself and exit.
    socket.write("show");
    socket.waitForBytesWritten(500);
    socket.disconnectFromServer();
    return true;
}

bool SingleInstanceGuard::listen()
{
    m_server = std::make_unique<QLocalServer>();
    QLocalServer::removeServer(m_appId); // clean up stale socket if crashed

    if (!m_server->listen(m_appId)) {
        m_server.reset();
        return false;
    }

    connect(m_server.get(), &QLocalServer::newConnection, this, &SingleInstanceGuard::handleNewConnection);
    return true;
}

void SingleInstanceGuard::handleNewConnection()
{
    QLocalSocket* conn = m_server->nextPendingConnection();
    if (!conn) {
        return;
    }

    auto fired = std::make_shared<bool>(false);

    connect(conn, &QLocalSocket::readyRead, this, [this, conn, fired]() {
        if (!*fired) {
            *fired = true;
            conn->readAll();
            emit showRequested();
        }
        conn->deleteLater();
    });

    QTimer::singleShot(Config::SINGLE_INSTANCE_ACK_TIMEOUT_MS, conn, [this, conn, fired]() {
        if (conn->bytesAvailable()) conn->readAll();
        if (!*fired) {
            *fired = true;
            emit showRequested();
        }
        conn->deleteLater();
    });
}
