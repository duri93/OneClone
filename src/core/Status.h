#pragma once
#include <QObject>
#include <QString>

// ---------------------------------------------------------------------------
// StatusBus
// App-wide broadcast channel for transient status-bar-style messages.
// Any object, anywhere (model or view), can emit through this without
// holding a reference to MainWindow. MainWindow connects to it once.
// ---------------------------------------------------------------------------
class Status : public QObject
{
    Q_OBJECT
public:
    enum class Level { Info, Success, Warning, Error };

    static Status& instance(){
        static Status bus;   // Meyer's singleton - thread-safe init (C++11+)
        return bus;
    }

    // convenience for call sites that don't want to spell out Level::Info
    static void notify(const QString& message, Level level = Level::Info){
        Status::instance().sendMessage(message, level);
    }

signals:
    void statusMessage(const QString& message, Status::Level level);

private:
    Status() = default;
    Q_DISABLE_COPY(Status)

    void sendMessage(const QString& message, Level level = Level::Info){
        emit statusMessage(message, level);
    }
};