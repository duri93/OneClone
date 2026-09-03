#include "src/app/SingleInstanceGuard.h"
#include "src/common/Config.h"
#include "src/core/Status.h"
#include "src/ui/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(Config::APP_NAME);
    app.setApplicationVersion(Config::APP_VERSION);
    app.setOrganizationName(Config::APP_AUTHOR);
    app.setQuitOnLastWindowClosed(false);

    SingleInstanceGuard instanceGuard(Config::APP_ID);
    if (instanceGuard.tryNotifyExisting()) {
        return 0;
    }
    const bool listening = instanceGuard.listen();

    MainWindow window;

    if (!listening) {
        // Not fatal — the app still works standalone — but future launches
        // won't be able to find this instance and will start duplicates.
        Status::notify(
            "Could not start the single-instance listener; opening this app again may launch a second copy.",
            Status::Level::Warning);
    }
    QObject::connect(&instanceGuard, &SingleInstanceGuard::showRequested, &window, &MainWindow::activate);

    if (!QCoreApplication::arguments().contains("--tray")) {
        window.show();
    }

    return app.exec();
}