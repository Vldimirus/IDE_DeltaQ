// Парсер аннотаций @dqmodule — заглушка
// TODO: полная реализация через libclang
#include "AnnotationParser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QUuid>

namespace DeltaQ {

AnnotationParser::AnnotationParser(QObject *parent)
    : QObject(parent)
{
}

QVector<Module> AnnotationParser::parseFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&file);
    QString source = in.readAll();
    return parseAnnotations(source, filePath);
}

QVector<Module> AnnotationParser::parseAnnotations(const QString &source, const QString &filePath)
{
    QVector<Module> result;

    // Простой парсер: ищем @dqmodule в комментариях
    // TODO: заменить на полноценный парсинг через libclang
    QRegularExpression moduleRx("@dqmodule\\s+name=(\\w+)");
    QRegularExpression versionRx("@dqmodule\\s+version=([\\d.]+)");
    QRegularExpression categoryRx("@dqmodule\\s+category=(\\w+)");
    QRegularExpression descRx("@dqmodule\\s+description=\"([^\"]+)\"");

    auto nameMatch = moduleRx.match(source);
    if (!nameMatch.hasMatch())
        return result;

    Module mod;
    mod.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    mod.name = nameMatch.captured(1);
    mod.origin = "user";
    mod.language = "c";
    mod.sourcePath = filePath;

    auto versionMatch = versionRx.match(source);
    if (versionMatch.hasMatch())
        mod.version = versionMatch.captured(1);

    auto descMatch = descRx.match(source);
    if (descMatch.hasMatch())
        mod.description = descMatch.captured(1);

    // Парсинг @dqport аннотаций
    // Формат: @dqport direction=input name=portName type=int default=0
    //         @dqport direction=output name=portName type=float
    QRegularExpression portRx(
        "@dqport\\s+"
        "direction=(input|output)\\s+"
        "name=(\\w+)\\s+"
        "type=(\\w+)"
        "(?:\\s+default=(?:\"([^\"]*)\"|([^\\s]*)))?"
    );
    auto portIt = portRx.globalMatch(source);
    while (portIt.hasNext()) {
        auto pm = portIt.next();
        Port port;
        port.name = pm.captured(2);
        port.type = pm.captured(3);
        // default может быть в кавычках или без
        if (!pm.captured(4).isEmpty())
            port.defaultValue = pm.captured(4);
        else if (!pm.captured(5).isEmpty())
            port.defaultValue = pm.captured(5);

        if (pm.captured(1) == "input")
            mod.inputs.append(port);
        else
            mod.outputs.append(port);
    }

    result.append(mod);
    return result;
}

} // namespace DeltaQ
