// Типы данных отладчика
#pragma once

#include <QString>
#include <QVector>

namespace DeltaQ {

// Точка останова
struct Breakpoint {
    int id = 0;           // ID от GDB
    QString file;
    int line = 0;
    QString condition;
    int hitCount = 0;
    bool enabled = true;

    bool operator==(const Breakpoint &other) const {
        return file == other.file && line == other.line;
    }
};

// Переменная (локальная или наблюдаемая)
struct Variable {
    QString name;
    QString type;
    QString value;
    QVector<Variable> children; // для составных типов (struct, array)

    bool hasChildren() const { return !children.isEmpty(); }
};

// Кадр стека вызовов
struct StackFrame {
    int level = 0;
    QString function;
    QString file;
    int line = 0;
    QString address;
};

// Состояние отладчика
enum class DebugState {
    Idle,       // Отладчик не запущен
    Running,    // Программа выполняется
    Paused,     // Остановлена (breakpoint, step, пауза)
    Stopped     // Завершена
};

} // namespace DeltaQ
