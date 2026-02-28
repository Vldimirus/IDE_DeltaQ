// Парсер аннотаций @dqmodule / @dqport
#include "AnnotationParser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QFileInfo>

namespace DeltaQ {

AnnotationParser::AnnotationParser(QObject *parent)
    : QObject(parent)
{
}

QString AnnotationParser::generateModuleId(const QString &filePath, const QString &moduleName)
{
    // Стабильный ID на основе пути файла и имени модуля
    QByteArray data = (filePath + "|" + moduleName).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    QString hex = hash.toHex().left(32);
    return QString("%1-%2-%3-%4-%5")
        .arg(hex.mid(0, 8), hex.mid(8, 4), hex.mid(12, 4), hex.mid(16, 4), hex.mid(20, 12));
}

QString AnnotationParser::dqmodPathForSource(const QString &sourcePath)
{
    QFileInfo fi(sourcePath);
    return fi.absolutePath() + "/" + fi.completeBaseName() + ".dqmod";
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

QVector<Module> AnnotationParser::parseSource(const QString &source, const QString &filePath)
{
    return parseAnnotations(source, filePath);
}

QMap<QString, QString> AnnotationParser::parseKeyValuePairs(const QString &text)
{
    QMap<QString, QString> result;

    // Парсим пары key=value или key="value with spaces"
    QRegularExpression kvRx(R"re((\w+)\s*=\s*(?:"([^"]*)"|(\S+)))re");
    auto it = kvRx.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        QString key = match.captured(1);
        // Значение в кавычках или без
        QString value = match.captured(2).isEmpty() ? match.captured(3) : match.captured(2);
        result[key] = value;
    }
    return result;
}

void AnnotationParser::finalizeModule(Module &mod, QVector<Module> &result, int lineNum)
{
    if (mod.name.isEmpty()) {
        m_errors.append({lineNum, "Модуль без имени (name= не указан)"});
        emit parseError(lineNum, "Модуль без имени (name= не указан)");
        return;
    }

    if (!mod.isValid()) {
        m_errors.append({lineNum, QString("Модуль '%1' не прошёл валидацию").arg(mod.name)});
        emit parseError(lineNum, QString("Модуль '%1' не прошёл валидацию").arg(mod.name));
        return;
    }

    result.append(mod);
    emit moduleParsed(mod);
}

QVector<Module> AnnotationParser::parseAnnotations(const QString &source, const QString &filePath)
{
    m_errors.clear();
    QVector<Module> result;

    QStringList lines = source.split('\n');

    Module currentModule;
    bool inModule = false;
    int moduleStartLine = 0;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();

        // Очистка от комментариев (// или * в начале строки многострочного комментария)
        // Поддерживаем: "// @dqmodule ...", "* @dqmodule ...", "/* @dqmodule ..."
        QString cleanLine = line;
        if (cleanLine.startsWith("//"))
            cleanLine = cleanLine.mid(2).trimmed();
        else if (cleanLine.startsWith("/*"))
            cleanLine = cleanLine.mid(2).trimmed();
        else if (cleanLine.startsWith("*") && !cleanLine.startsWith("*/"))
            cleanLine = cleanLine.mid(1).trimmed();

        // Убираем завершающий */
        if (cleanLine.endsWith("*/"))
            cleanLine = cleanLine.left(cleanLine.length() - 2).trimmed();

        // Проверяем на @dqmodule
        if (cleanLine.startsWith("@dqmodule")) {
            // Завершаем предыдущий модуль, если есть
            if (inModule) {
                finalizeModule(currentModule, result, moduleStartLine);
            }

            // Начинаем новый модуль
            currentModule = Module();
            currentModule.origin = "user";
            currentModule.language = "c";
            currentModule.sourcePath = filePath;

            if (filePath.endsWith(".cpp") || filePath.endsWith(".cxx") ||
                filePath.endsWith(".cc") || filePath.endsWith(".hpp"))
                currentModule.language = "cpp";

            inModule = true;
            moduleStartLine = i + 1; // 1-based

            // Парсим параметры @dqmodule
            QString params = cleanLine.mid(9).trimmed(); // после "@dqmodule"
            auto kv = parseKeyValuePairs(params);

            if (kv.contains("name"))
                currentModule.name = kv["name"];
            if (kv.contains("version"))
                currentModule.version = kv["version"];
            if (kv.contains("category"))
                currentModule.category = kv["category"];
            if (kv.contains("description"))
                currentModule.description = kv["description"];
            if (kv.contains("language"))
                currentModule.language = kv["language"];

            // Генерируем стабильный ID (после парсинга имени)
            if (!currentModule.name.isEmpty())
                currentModule.id = generateModuleId(filePath, currentModule.name);
        }
        // Проверяем на @dqport (привязывается к текущему модулю)
        else if (cleanLine.startsWith("@dqport") && inModule) {
            QString params = cleanLine.mid(7).trimmed(); // после "@dqport"
            auto kv = parseKeyValuePairs(params);

            if (!kv.contains("name") || !kv.contains("type")) {
                m_errors.append({i + 1, "Порт без name или type"});
                emit parseError(i + 1, "Порт без name или type");
                continue;
            }

            Port port;
            port.name = kv["name"];
            port.type = kv["type"];
            if (kv.contains("default"))
                port.defaultValue = kv["default"];

            QString direction = kv.value("direction", "input");
            if (direction == "output")
                currentModule.outputs.append(port);
            else
                currentModule.inputs.append(port);
        }
        else if (cleanLine.startsWith("@dqport") && !inModule) {
            m_errors.append({i + 1, "@dqport без предшествующего @dqmodule"});
            emit parseError(i + 1, "@dqport без предшествующего @dqmodule");
        }
    }

    // Финализируем последний модуль
    if (inModule) {
        finalizeModule(currentModule, result, moduleStartLine);
    }

    return result;
}

} // namespace DeltaQ
