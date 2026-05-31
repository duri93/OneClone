#include "view/MainWindow.h"
#include <QApplication>

#include <QLocalServer>
#include <QLocalSocket>

static const char* INSTANCE_KEY = "SRCMG_SINGLE_INSTANCE";

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("OneRClone");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("OneRClone");
    app.setQuitOnLastWindowClosed(false);

    // Try to connect to an already-running instance
    QLocalSocket socket;
    socket.connectToServer(INSTANCE_KEY);
    if (socket.waitForConnected(500)) {
        // Another instance is running — tell it to show itself and exit
        socket.write("show");
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return 0;
    }

    // No existing instance — become the server
    QLocalServer server;
    QLocalServer::removeServer(INSTANCE_KEY); // clean up stale socket if crashed
    server.listen(INSTANCE_KEY);

    MainWindow window;

    // When a second instance connects, show the window
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket* conn = server.nextPendingConnection();
        QObject::connect(conn, &QLocalSocket::readyRead, [&window, conn]() {
            conn->readAll(); // consume the message

            window.activate();

            conn->deleteLater();
        });
    });

    // show window
    if (!QCoreApplication::arguments().contains("--tray")){
        window.show();
    }

    return app.exec();
}





