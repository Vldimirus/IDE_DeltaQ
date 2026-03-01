// Менеджер модулей — библиотека + редактор + компиляция + тестирование
#pragma once

#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>

class QPlainTextEdit;
class QGraphicsScene;
class QGraphicsView;
class QTabWidget;
class QGridLayout;

namespace DeltaQ {

class ModuleRegistry;
class ModuleTestRunner;
class NodeItem;
class SyntaxHighlighter;
struct Module;

class ModuleManagerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ModuleManagerWidget(ModuleRegistry *registry, QWidget *parent = nullptr);

    // Обновить дерево из реестра
    void rebuildTree();

    // Установить путь проекта (для сохранения .dqmod файлов)
    void setProjectDir(const QString &dir);

    // Установить глобальную папку модулей
    void setGlobalModulesDir(const QString &dir);

    // Фильтрация по языку
    void setLanguageFilter(const QString &lang);

signals:
    void moduleChanged(const QString &moduleId);

private slots:
    void onModuleSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void onNewModule();
    void onNewPack();
    void onDeleteModule();
    void onSaveModule();
    void onCompileModule();
    void onTestModule();
    void onCodeChanged();

private:
    void setupUI();
    void buildTree();
    void loadModuleToEditor(const Module &module);
    void clearEditor();
    void updatePortTable(const Module &module);
    void updatePreview(const Module &module);
    void parseSignature(const QString &code);
    void updateStatusIcon(QTreeWidgetItem *item, const QString &status);
    QString moduleFilePath(const QString &moduleId) const;
    bool saveToDisk(const Module &module);
    bool deleteFromDisk(const QString &moduleId);

    ModuleRegistry *m_registry;
    ModuleTestRunner *m_testRunner;

    // Левая панель — библиотека
    QTreeWidget *m_tree = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_langFilter = nullptr;
    QPushButton *m_newBtn = nullptr;
    QPushButton *m_newPackBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;

    // Правая панель — редактор модуля
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_descEdit = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QComboBox *m_langCombo = nullptr;

    // Зависимости (#include)
    QTextEdit *m_includesEdit = nullptr;

    // Редактор кода
    QPlainTextEdit *m_codeEdit = nullptr;

    // Таблица портов
    QTableWidget *m_portTable = nullptr;

    // Кнопки действий
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_compileBtn = nullptr;
    QPushButton *m_testBtn = nullptr;

    // Панель тестирования
    QTableWidget *m_testInputs = nullptr;
    QLabel *m_testOutputLabel = nullptr;

    // Журнал
    QTextEdit *m_logView = nullptr;

    // Превью блока на графе
    QGraphicsScene *m_previewScene = nullptr;
    QGraphicsView  *m_previewView  = nullptr;

    // Табы внизу (Журнал / Тестирование)
    QTabWidget *m_bottomTabs = nullptr;

    // Подсветка синтаксиса
    SyntaxHighlighter *m_highlighter = nullptr;

    // Текущий модуль
    QString m_currentModuleId;
    QString m_projectDir;
    QString m_globalModulesDir;
    bool m_modified = false;
};

} // namespace DeltaQ
