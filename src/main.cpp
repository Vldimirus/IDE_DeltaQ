// DeltaQ IDE — точка входа
// Кроссплатформенная среда разработки для модульного программирования

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSettings>
#include "core/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DeltaQ IDE");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("DeltaQ");

    // Загрузка переводов до создания виджетов
    QSettings settings("DeltaQ", "IDE");
    QString lang = settings.value("app/language", "en").toString();

    QTranslator qtTranslator;
    QTranslator appTranslator;

    if (lang != "en") {
        // Стандартные переводы Qt (диалоги, кнопки)
        if (qtTranslator.load("qt_" + lang,
                QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            app.installTranslator(&qtTranslator);
        }

        // Переводы приложения
        QString translationsDir = QApplication::applicationDirPath() + "/translations";
        if (appTranslator.load("deltaq_" + lang, translationsDir)) {
            app.installTranslator(&appTranslator);
        }
    }

    DeltaQ::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
