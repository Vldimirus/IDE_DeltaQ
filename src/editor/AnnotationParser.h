// Парсер аннотаций @dqmodule / @dqport
#pragma once

#include <QObject>
#include <QVector>
#include <deltaq/Module.h>

namespace DeltaQ {

// Информация об ошибке парсинга
struct AnnotationError {
    int line = 0;
    QString message;
};

class AnnotationParser : public QObject {
    Q_OBJECT

public:
    explicit AnnotationParser(QObject *parent = nullptr);

    // Парсит файл и возвращает найденные модули
    QVector<Module> parseFile(const QString &filePath);

    // Парсит текст и возвращает найденные модули
    QVector<Module> parseSource(const QString &source, const QString &filePath = QString());

    // Последние ошибки парсинга
    const QVector<AnnotationError> &errors() const { return m_errors; }
    bool hasErrors() const { return !m_errors.isEmpty(); }

    // Генерация стабильного ID из пути файла и имени модуля
    static QString generateModuleId(const QString &filePath, const QString &moduleName);

    // Генерация .dqmod файла рядом с исходником
    static QString dqmodPathForSource(const QString &sourcePath);

signals:
    void parseError(int line, const QString &message);
    void moduleParsed(const Module &module);

private:
    // Построчный парсинг аннотаций
    QVector<Module> parseAnnotations(const QString &source, const QString &filePath);

    // Парсинг key=value пар из строки аннотации
    QMap<QString, QString> parseKeyValuePairs(const QString &text);

    // Финализация текущего модуля (валидация и добавление в результат)
    void finalizeModule(Module &mod, QVector<Module> &result, int lineNum);

    QVector<AnnotationError> m_errors;
};

} // namespace DeltaQ
