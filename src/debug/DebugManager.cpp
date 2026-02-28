// Менеджер отладки — реализация GDB/MI протокола
#include "DebugManager.h"
#include <QRegularExpression>
#include <QFileInfo>

namespace DeltaQ {

DebugManager::DebugManager(QObject *parent)
    : QObject(parent)
{
}

DebugManager::~DebugManager()
{
    stopDebug();
}

// === Управление ===

void DebugManager::startDebug(const QString &executable, const QStringList &args)
{
    if (m_state != DebugState::Idle) {
        stopDebug();
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &DebugManager::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &DebugManager::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DebugManager::onProcessFinished);

    // Запускаем GDB в режиме MI (Machine Interface)
    QStringList gdbArgs = {"--interpreter=mi", "--quiet", executable};
    m_process->start("gdb", gdbArgs);

    if (!m_process->waitForStarted(5000)) {
        emit errorOccurred("Не удалось запустить GDB");
        delete m_process;
        m_process = nullptr;
        return;
    }

    setState(DebugState::Paused);
    emit debugStarted();

    // Устанавливаем точки останова, которые были добавлены до запуска
    for (const auto &bp : m_breakpoints) {
        sendCommand(QString("-break-insert %1:%2").arg(bp.file).arg(bp.line));
    }

    // Передаём аргументы программы
    if (!args.isEmpty())
        sendCommand("-exec-arguments " + args.join(' '));

    // Запускаем программу
    sendCommand("-exec-run");
}

void DebugManager::stopDebug()
{
    if (!m_process)
        return;

    sendCommand("-gdb-exit");
    if (!m_process->waitForFinished(3000)) {
        m_process->terminate();
        if (!m_process->waitForFinished(2000))
            m_process->kill();
    }

    m_process->deleteLater();
    m_process = nullptr;
    m_pendingCommands.clear();
    m_localVariables.clear();
    m_callStack.clear();
    m_buffer.clear();
    setState(DebugState::Idle);
    emit debugStopped();
}

void DebugManager::pauseExecution()
{
    if (m_state != DebugState::Running)
        return;
    sendCommand("-exec-interrupt");
}

void DebugManager::continueExecution()
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand("-exec-continue");
}

// === Шаги ===

void DebugManager::stepOver()
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand("-exec-next");
}

void DebugManager::stepInto()
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand("-exec-step");
}

void DebugManager::stepOut()
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand("-exec-finish");
}

void DebugManager::runToCursor(const QString &file, int line)
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand(QString("-exec-until %1:%2").arg(file).arg(line));
}

// === Точки останова ===

void DebugManager::addBreakpoint(const QString &file, int line)
{
    // Проверяем, что breakpoint ещё не установлен
    for (const auto &bp : m_breakpoints) {
        if (bp.file == file && bp.line == line)
            return;
    }

    Breakpoint bp;
    bp.file = file;
    bp.line = line;
    m_breakpoints.append(bp);
    emit breakpointAdded(bp);

    // Если GDB запущен — отправляем команду
    if (m_process && m_process->state() == QProcess::Running) {
        sendCommand(QString("-break-insert %1:%2").arg(file).arg(line));
    }
}

void DebugManager::removeBreakpoint(const QString &file, int line)
{
    for (int i = 0; i < m_breakpoints.size(); ++i) {
        if (m_breakpoints[i].file == file && m_breakpoints[i].line == line) {
            int gdbId = m_breakpoints[i].id;
            m_breakpoints.removeAt(i);
            emit breakpointRemoved(file, line);

            // Удаляем в GDB
            if (gdbId > 0 && m_process && m_process->state() == QProcess::Running) {
                sendCommand(QString("-break-delete %1").arg(gdbId));
            }
            return;
        }
    }
}

void DebugManager::toggleBreakpoint(const QString &file, int line)
{
    for (const auto &bp : m_breakpoints) {
        if (bp.file == file && bp.line == line) {
            removeBreakpoint(file, line);
            return;
        }
    }
    addBreakpoint(file, line);
}

void DebugManager::setConditionalBreakpoint(const QString &file, int line,
                                              const QString &condition)
{
    removeBreakpoint(file, line);

    Breakpoint bp;
    bp.file = file;
    bp.line = line;
    bp.condition = condition;
    m_breakpoints.append(bp);
    emit breakpointAdded(bp);

    if (m_process && m_process->state() == QProcess::Running) {
        sendCommand(QString("-break-insert -c \"%1\" %2:%3")
                    .arg(condition, file).arg(line));
    }
}

// === Переменные ===

void DebugManager::evaluateExpression(const QString &expression)
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand(QString("-data-evaluate-expression \"%1\"").arg(expression));
}

void DebugManager::requestLocalVariables()
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand("-stack-list-variables --simple-values");
}

// === Стек вызовов ===

void DebugManager::requestCallStack()
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand("-stack-list-frames");
}

void DebugManager::selectFrame(int index)
{
    if (m_state != DebugState::Paused)
        return;
    sendCommand(QString("-stack-select-frame %1").arg(index));
    // После смены кадра запрашиваем переменные
    requestLocalVariables();
}

// === Внутренние методы ===

void DebugManager::sendCommand(const QString &command)
{
    if (!m_process || m_process->state() != QProcess::Running)
        return;

    int token = m_nextToken++;
    m_pendingCommands[token] = command;

    QString line = QString("%1%2\n").arg(token).arg(command);
    m_process->write(line.toUtf8());
}

void DebugManager::onProcessOutput()
{
    m_buffer += m_process->readAllStandardOutput();

    // Обрабатываем построчно
    while (m_buffer.contains('\n')) {
        int idx = m_buffer.indexOf('\n');
        QString line = m_buffer.left(idx).trimmed();
        m_buffer = m_buffer.mid(idx + 1);

        if (!line.isEmpty())
            processLine(line);
    }
}

void DebugManager::onProcessError()
{
    QString error = m_process->readAllStandardError();
    if (!error.isEmpty())
        emit debugOutput(error);
}

void DebugManager::onProcessFinished(int /*exitCode*/, QProcess::ExitStatus /*status*/)
{
    setState(DebugState::Idle);
    m_process->deleteLater();
    m_process = nullptr;
    m_pendingCommands.clear();
    m_buffer.clear();
    emit debugStopped();
}

void DebugManager::processLine(const QString &line)
{
    // Пропускаем строку приглашения GDB
    if (line == "(gdb)")
        return;

    auto record = parseMIOutput(line);

    switch (record.type) {
    case MIRecord::Result:
        handleResultRecord(record);
        break;
    case MIRecord::ExecAsync:
        handleExecAsyncRecord(record);
        break;
    case MIRecord::ConsoleStream:
        emit debugOutput(unquoteMI(record.payload));
        break;
    case MIRecord::TargetStream:
        emit targetOutput(unquoteMI(record.payload));
        break;
    case MIRecord::LogStream:
        // Логи GDB — игнорируем
        break;
    default:
        break;
    }
}

void DebugManager::handleResultRecord(const MIRecord &record)
{
    if (record.resultClass == "running") {
        setState(DebugState::Running);
        return;
    }

    if (record.resultClass == "error") {
        auto fields = parseMITuple(record.payload);
        emit errorOccurred(unquoteMI(fields.value("msg")));
        return;
    }

    if (record.resultClass != "done")
        return;

    // Определяем какая команда была отправлена
    QString cmd = m_pendingCommands.take(record.token);

    // Парсим результат в зависимости от команды
    if (cmd.startsWith("-break-insert")) {
        // Парсим номер breakpoint из ответа
        // bkpt={number="1",type="breakpoint",file="main.c",line="10",...}
        auto fields = parseMITuple(record.payload);
        // Ищем вложенный bkpt tuple
        int bkptStart = record.payload.indexOf("bkpt={");
        if (bkptStart >= 0) {
            int braceStart = record.payload.indexOf('{', bkptStart);
            int braceEnd = record.payload.indexOf('}', braceStart);
            if (braceEnd > braceStart) {
                QString bkptStr = record.payload.mid(braceStart + 1, braceEnd - braceStart - 1);
                auto bkptFields = parseMITuple(bkptStr);
                int gdbId = bkptFields.value("number").toInt();
                QString file = unquoteMI(bkptFields.value("fullname", bkptFields.value("file")));
                int line = bkptFields.value("line").toInt();

                // Обновляем ID в нашем списке
                for (auto &bp : m_breakpoints) {
                    if (bp.file == file && bp.line == line) {
                        bp.id = gdbId;
                        break;
                    }
                    // Сравниваем также по basename
                    if (QFileInfo(bp.file).fileName() == QFileInfo(file).fileName()
                        && bp.line == line) {
                        bp.id = gdbId;
                        break;
                    }
                }
            }
        }
    }
    else if (cmd.startsWith("-stack-list-frames")) {
        // stack=[frame={level="0",addr="0x...",func="main",file="main.c",line="5"},...]
        m_callStack.clear();
        int stackStart = record.payload.indexOf("stack=[");
        if (stackStart >= 0) {
            QString stackStr = record.payload.mid(stackStart + 7);
            // Парсим каждый frame={...}
            QRegularExpression frameRx("frame=\\{([^}]+)\\}");
            auto it = frameRx.globalMatch(stackStr);
            while (it.hasNext()) {
                auto match = it.next();
                auto fields = parseMITuple(match.captured(1));

                StackFrame frame;
                frame.level = fields.value("level").toInt();
                frame.function = unquoteMI(fields.value("func"));
                frame.file = unquoteMI(fields.value("fullname", fields.value("file")));
                frame.line = fields.value("line").toInt();
                frame.address = unquoteMI(fields.value("addr"));
                m_callStack.append(frame);
            }
        }
        emit callStackUpdated(m_callStack);
    }
    else if (cmd.startsWith("-stack-list-variables")) {
        // variables=[{name="x",arg="1",type="int",value="5"},...]
        m_localVariables.clear();
        int varsStart = record.payload.indexOf("variables=[");
        if (varsStart >= 0) {
            QString varsStr = record.payload.mid(varsStart + 11);
            QRegularExpression varRx("\\{([^}]+)\\}");
            auto it = varRx.globalMatch(varsStr);
            while (it.hasNext()) {
                auto match = it.next();
                auto fields = parseMITuple(match.captured(1));

                Variable var;
                var.name = unquoteMI(fields.value("name"));
                var.type = unquoteMI(fields.value("type"));
                var.value = unquoteMI(fields.value("value"));
                m_localVariables.append(var);
            }
        }
        emit variablesUpdated(m_localVariables);
    }
    else if (cmd.startsWith("-data-evaluate-expression")) {
        auto fields = parseMITuple(record.payload);
        QString value = unquoteMI(fields.value("value"));
        // Извлекаем выражение из команды
        QRegularExpression exprRx("-data-evaluate-expression\\s+\"(.+)\"");
        auto match = exprRx.match(cmd);
        QString expr = match.hasMatch() ? match.captured(1) : cmd;
        emit expressionEvaluated(expr, value);
    }
}

void DebugManager::handleExecAsyncRecord(const MIRecord &record)
{
    if (record.resultClass == "stopped") {
        setState(DebugState::Paused);

        auto fields = parseMITuple(record.payload);
        QString reason = unquoteMI(fields.value("reason"));

        // Парсим frame из payload
        QString file;
        int line = 0;
        int frameStart = record.payload.indexOf("frame={");
        if (frameStart >= 0) {
            int braceStart = record.payload.indexOf('{', frameStart);
            int braceEnd = record.payload.indexOf('}', braceStart);
            if (braceEnd > braceStart) {
                QString frameStr = record.payload.mid(braceStart + 1, braceEnd - braceStart - 1);
                auto frameFields = parseMITuple(frameStr);
                file = unquoteMI(frameFields.value("fullname", frameFields.value("file")));
                line = frameFields.value("line").toInt();
            }
        }

        if (reason == "breakpoint-hit") {
            emit breakpointHit(file, line);
        } else if (reason == "end-stepping-range" || reason == "function-finished") {
            emit stepped(file, line);
        } else if (reason == "exited-normally" || reason == "exited") {
            setState(DebugState::Stopped);
            emit debugOutput("Программа завершилась нормально\n");
        } else if (reason == "signal-received") {
            QString signalName = unquoteMI(fields.value("signal-name"));
            emit debugOutput(QString("Получен сигнал: %1\n").arg(signalName));
        }

        // Автоматически запрашиваем стек и переменные при остановке
        if (m_state == DebugState::Paused) {
            requestCallStack();
            requestLocalVariables();
        }
    }
    else if (record.resultClass == "running") {
        setState(DebugState::Running);
    }
}

void DebugManager::setState(DebugState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

// === Статические методы парсинга MI ===

DebugManager::MIRecord DebugManager::parseMIOutput(const QString &line)
{
    MIRecord record;

    if (line.isEmpty())
        return record;

    int pos = 0;

    // Парсим токен (число в начале строки)
    while (pos < line.length() && line[pos].isDigit()) {
        pos++;
    }
    if (pos > 0) {
        record.token = line.left(pos).toInt();
    }

    if (pos >= line.length())
        return record;

    QChar indicator = line[pos];
    QString rest = line.mid(pos + 1);

    if (indicator == '^') {
        record.type = MIRecord::Result;
        int commaIdx = rest.indexOf(',');
        if (commaIdx >= 0) {
            record.resultClass = rest.left(commaIdx);
            record.payload = rest.mid(commaIdx + 1);
        } else {
            record.resultClass = rest;
        }
    }
    else if (indicator == '*') {
        record.type = MIRecord::ExecAsync;
        int commaIdx = rest.indexOf(',');
        if (commaIdx >= 0) {
            record.resultClass = rest.left(commaIdx);
            record.payload = rest.mid(commaIdx + 1);
        } else {
            record.resultClass = rest;
        }
    }
    else if (indicator == '+') {
        record.type = MIRecord::StatusAsync;
        record.resultClass = rest;
    }
    else if (indicator == '=') {
        record.type = MIRecord::NotifyAsync;
        int commaIdx = rest.indexOf(',');
        if (commaIdx >= 0) {
            record.resultClass = rest.left(commaIdx);
            record.payload = rest.mid(commaIdx + 1);
        } else {
            record.resultClass = rest;
        }
    }
    else if (indicator == '~') {
        record.type = MIRecord::ConsoleStream;
        record.payload = rest;
    }
    else if (indicator == '@') {
        record.type = MIRecord::TargetStream;
        record.payload = rest;
    }
    else if (indicator == '&') {
        record.type = MIRecord::LogStream;
        record.payload = rest;
    }

    return record;
}

QMap<QString, QString> DebugManager::parseMITuple(const QString &tuple)
{
    QMap<QString, QString> result;

    int i = 0;
    while (i < tuple.length()) {
        // Пропускаем пробелы и запятые
        while (i < tuple.length() && (tuple[i] == ' ' || tuple[i] == ','))
            ++i;

        // Ищем имя ключа
        int keyStart = i;
        while (i < tuple.length() && tuple[i] != '=' && tuple[i] != ',' && tuple[i] != '}')
            ++i;

        if (i >= tuple.length() || tuple[i] != '=')
            break;

        QString key = tuple.mid(keyStart, i - keyStart).trimmed();
        ++i; // пропускаем '='

        if (i >= tuple.length())
            break;

        QString value;
        if (tuple[i] == '"') {
            // Строковое значение в кавычках
            ++i;
            int valueStart = i;
            while (i < tuple.length()) {
                if (tuple[i] == '\\' && i + 1 < tuple.length()) {
                    i += 2; // пропускаем escape-последовательность
                } else if (tuple[i] == '"') {
                    break;
                } else {
                    ++i;
                }
            }
            value = tuple.mid(valueStart, i - valueStart);
            if (i < tuple.length())
                ++i; // пропускаем закрывающую кавычку
        }
        else if (tuple[i] == '{') {
            // Вложенный tuple — пропускаем (берём как есть)
            int depth = 1;
            int valueStart = i;
            ++i;
            while (i < tuple.length() && depth > 0) {
                if (tuple[i] == '{') ++depth;
                else if (tuple[i] == '}') --depth;
                ++i;
            }
            value = tuple.mid(valueStart, i - valueStart);
        }
        else if (tuple[i] == '[') {
            // Список — пропускаем (берём как есть)
            int depth = 1;
            int valueStart = i;
            ++i;
            while (i < tuple.length() && depth > 0) {
                if (tuple[i] == '[') ++depth;
                else if (tuple[i] == ']') --depth;
                ++i;
            }
            value = tuple.mid(valueStart, i - valueStart);
        }
        else {
            // Простое значение
            int valueStart = i;
            while (i < tuple.length() && tuple[i] != ',' && tuple[i] != '}')
                ++i;
            value = tuple.mid(valueStart, i - valueStart).trimmed();
        }

        if (!key.isEmpty())
            result[key] = value;
    }

    return result;
}

QVector<QMap<QString, QString>> DebugManager::parseMIList(const QString &list)
{
    QVector<QMap<QString, QString>> result;

    QRegularExpression itemRx("\\{([^}]+)\\}");
    auto it = itemRx.globalMatch(list);
    while (it.hasNext()) {
        auto match = it.next();
        result.append(parseMITuple(match.captured(1)));
    }

    return result;
}

QString DebugManager::unquoteMI(const QString &str)
{
    if (str.isEmpty())
        return str;

    QString s = str;

    // Убираем внешние кавычки
    if (s.startsWith('"') && s.endsWith('"'))
        s = s.mid(1, s.length() - 2);

    // Обрабатываем escape-последовательности
    s.replace("\\n", "\n");
    s.replace("\\t", "\t");
    s.replace("\\\\", "\\");
    s.replace("\\\"", "\"");

    return s;
}

} // namespace DeltaQ
