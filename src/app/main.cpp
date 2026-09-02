#include "src/app/SingleInstanceGuard.h"
#include "src/common/Config.h"
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
    instanceGuard.listen();

    MainWindow window;
    QObject::connect(&instanceGuard, &SingleInstanceGuard::showRequested, &window, &MainWindow::activate);

    if (!QCoreApplication::arguments().contains("--tray")) {
        window.show();
    }

    return app.exec();
}