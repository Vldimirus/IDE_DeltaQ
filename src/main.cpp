// DeltaQ IDE — точка входа
// Кроссплатформенная среда разработки для модульного программирования

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QTimer>
#include "core/MainWindow.h"
#include "core/SessionManager.h"

namespace {

QString decodeEscapedText(const QString &text)
{
    QString decoded;
    decoded.reserve(text.size());

    bool escaped = false;
    for (const QChar ch : text) {
        if (!escaped) {
            if (ch == '\\') {
                escaped = true;
                continue;
            }
            decoded += ch;
            continue;
        }

        switch (ch.unicode()) {
        case 'n':
            decoded += '\n';
            break;
        case 'r':
            decoded += '\r';
            break;
        case 't':
            decoded += '\t';
            break;
        case '\\':
            decoded += '\\';
            break;
        default:
            decoded += ch;
            break;
        }
        escaped = false;
    }

    if (escaped)
        decoded += '\\';

    return decoded;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DeltaQ IDE");
    app.setApplicationVersion(QStringLiteral(DQ_APP_VERSION));
    app.setOrganizationName("DeltaQ");

    QCommandLineParser parser;
    parser.setApplicationDescription("DeltaQ IDE");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption automationProjectOption(
        "automation-project",
        "Open a project automatically after startup.",
        "dqproj");
    const QCommandLineOption automationBuildOption(
        "automation-build",
        "Run build automatically after opening the project.");
    const QCommandLineOption automationExportOption(
        "automation-export",
        "Run build/export automatically after opening the project.");
    const QCommandLineOption automationRunOption(
        "automation-run",
        "Run the built executable automatically after opening/building the project.");
    const QCommandLineOption automationQuitOption(
        "automation-quit",
        "Exit DeltaQ automatically after startup automation finishes.");
    const QCommandLineOption automationStdinOption(
        "automation-stdin",
        "Write escaped text (for example, Line1\\nLine2\\n) to the started program stdin.",
        "text");
    const QCommandLineOption automationExpectOutputOption(
        "automation-expect-output",
        "Require stdout from the started program to contain the escaped text.",
        "text");

    parser.addOption(automationProjectOption);
    parser.addOption(automationBuildOption);
    parser.addOption(automationExportOption);
    parser.addOption(automationRunOption);
    parser.addOption(automationQuitOption);
    parser.addOption(automationStdinOption);
    parser.addOption(automationExpectOutputOption);
    parser.process(app);

    // Язык читается из того же writable-root, что и остальная сессия IDE,
    // поэтому isolated first-run через DELTAQ_HOME остаётся детерминированным.
    const QString lang = DeltaQ::SessionManager::storedLanguage();

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

    if (parser.isSet(automationProjectOption)) {
        mainWindow.startStartupAutomation(
            parser.value(automationProjectOption),
            parser.isSet(automationBuildOption),
            parser.isSet(automationExportOption),
            parser.isSet(automationRunOption),
            parser.isSet(automationQuitOption),
            decodeEscapedText(parser.value(automationStdinOption)),
            decodeEscapedText(parser.value(automationExpectOutputOption)));
    }

    bool smokeExitOk = false;
    const int smokeExitMs = qEnvironmentVariableIntValue("DELTAQ_SMOKE_EXIT_MS", &smokeExitOk);
    if (smokeExitOk && smokeExitMs > 0)
        QTimer::singleShot(smokeExitMs, &app, &QCoreApplication::quit);

    return app.exec();
}
