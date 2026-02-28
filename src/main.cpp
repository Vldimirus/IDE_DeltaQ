// DeltaQ IDE — точка входа
// Кроссплатформенная среда разработки для модульного программирования

#include <QApplication>
#include "core/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DeltaQ IDE");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("DeltaQ");

    DeltaQ::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
