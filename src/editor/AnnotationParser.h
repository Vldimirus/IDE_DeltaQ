// Парсер аннотаций @dqmodule — заглушка
// TODO: полная реализация через libclang
#pragma once

#include <QObject>
#include <QStringList>
#include <deltaq/Module.h>

namespace DeltaQ {

class AnnotationParser : public QObject {
    Q_OBJECT

public:
    explicit AnnotationParser(QObject *parent = nullptr);

    // Парсит файл и возвращает найденные модули
    QVector<Module> parseFile(const QString &filePath);

private:
    // Парсинг комментариев @dqmodule из текста
    QVector<Module> parseAnnotations(const QString &source, const QString &filePath);
};

} // namespace DeltaQ
