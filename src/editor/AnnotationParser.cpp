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

    // TODO: поле category будет добавлено в Module позже
    // auto categoryMatch = categoryRx.match(source);

    auto descMatch = descRx.match(source);
    if (descMatch.hasMatch())
        mod.description = descMatch.captured(1);

    // TODO: парсить @dqport аннотации для портов

    result.append(mod);
    return result;
}

} // namespace DeltaQ
