#pragma once

#include <memory>
#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QLocalServer;
QT_END_NAMESPACE

// ---------------------------------------------------------------------------
// SingleInstanceGuard
// Ensures only one instance of the application runs at a time, using a
// QLocalServer/QLocalSocket handshake identified by an app-specific id.
// A second launch notifies the first instance and exits.
// ---------------------------------------------------------------------------
class SingleInstanceGuard : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstanceGuard(QString appId, QObject* parent = nullptr);
    ~SingleInstanceGuard() override;

    // Tries to connect to an already-running instance and tell it to show
    // itself. Returns true if another instance is running (caller should
    // exit); returns false if this is the first instance.
    bool tryNotifyExisting();

    // Starts listening for future instances. Call only after
    // tryNotifyExisting() returned false. Returns false if the listen
    // socket could not be created (e.g. a stale lock that removeServer()
    // couldn't clear) — in that case future launches will silently start
    // second instances instead of notifying this one, so the caller should
    // surface that to the user.
    bool listen();

signals:
    // Emitted whenever a second instance is launched and should be shown.
    void showRequested();

private:
    void handleNewConnection();

    QString m_appId;
    std::unique_ptr<QLocalServer> m_server;
};
