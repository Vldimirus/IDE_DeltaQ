// Мастер импорта библиотеки — 5 шагов
#pragma once

#include "LibclangParser.h"
#include "LibraryDecomposer.h"
#include "WrapperGenerator.h"

#include <QWizard>

class QLineEdit;
class QTreeWidget;
class QComboBox;
class QProgressBar;
class QTextEdit;
class QLabel;

namespace DeltaQ {

class ModuleRegistry;

class LibraryImportWizard : public QWizard {
    Q_OBJECT

public:
    explicit LibraryImportWizard(ModuleRegistry *registry, QWidget *parent = nullptr);

    // Результаты
    QVector<Module> importedModules() const { return m_modules; }
    QVector<WrapperCode> generatedWrappers() const { return m_wrappers; }

private:
    void setupPage1_SelectLibrary();
    void setupPage2_Parse();
    void setupPage3_SelectElements();
    void setupPage4_ConfigureModules();
    void setupPage5_Generate();

    ModuleRegistry *m_registry;

    // Страница 1: Выбор библиотеки
    QLineEdit *m_headerPathEdit = nullptr;
    QLineEdit *m_includePathsEdit = nullptr;
    QLineEdit *m_definesEdit = nullptr;
    QComboBox *m_standardCombo = nullptr;

    // Страница 2: Парсинг
    QTreeWidget *m_parseResultTree = nullptr;
    QLabel *m_parseStatsLabel = nullptr;

    // Страница 3: Выбор элементов
    QTreeWidget *m_selectTree = nullptr;

    // Страница 4: Настройка модулей
    QTreeWidget *m_modulesTree = nullptr;
    QTextEdit *m_wrapperPreview = nullptr;

    // Страница 5: Генерация
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_resultLabel = nullptr;

    // Данные
    ParseResult m_parseResult;
    DecompositionOptions m_options;
    QVector<Module> m_modules;
    QVector<WrapperCode> m_wrappers;
};

} // namespace DeltaQ
