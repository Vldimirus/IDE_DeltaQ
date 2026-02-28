// Тесты отображения диагностики в редакторе
#include <QTest>
#include <QApplication>
#include <QTextEdit>
#include <QPlainTextEdit>

#include "../../src/editor/CodeEditorTab.h"
#include "../../src/lsp/LSPTypes.h"

using namespace DeltaQ;

class TestDiagnosticDisplay : public QObject {
    Q_OBJECT

private:
    CodeEditorTab *createTab(const QString &text) {
        auto *tab = new CodeEditorTab("test.cpp", nullptr);
        tab->editor()->setPlainText(text);
        return tab;
    }

private slots:
    void testErrorSquiggle()
    {
        auto *tab = createTab("int main() {\n    unknownFunc();\n}\n");

        QVector<LSPDiagnostic> diags;
        LSPDiagnostic d;
        d.range = {{1, 4}, {1, 15}};
        d.severity = DiagnosticSeverity::Error;
        d.message = "unknown function";
        diags.append(d);

        tab->setDiagnostics(diags);

        QCOMPARE(tab->diagnostics().size(), 1);
        QCOMPARE(tab->diagnostics()[0].severity, DiagnosticSeverity::Error);
        QCOMPARE(tab->diagnostics()[0].message, "unknown function");

        // Проверяем что ExtraSelections содержат диагностику
        auto selections = tab->editor()->extraSelections();
        // Минимум: текущая строка + диагностика
        QVERIFY(selections.size() >= 2);

        // Ищем selection с WaveUnderline
        bool foundWave = false;
        for (const auto &sel : selections) {
            if (sel.format.underlineStyle() == QTextCharFormat::WaveUnderline) {
                foundWave = true;
                QCOMPARE(sel.format.underlineColor(), QColor(255, 0, 0)); // красный
                QCOMPARE(sel.format.toolTip(), "unknown function");
            }
        }
        QVERIFY(foundWave);

        delete tab;
    }

    void testWarningSquiggle()
    {
        auto *tab = createTab("int x = 0;\nint main() { return 0; }\n");

        QVector<LSPDiagnostic> diags;
        LSPDiagnostic d;
        d.range = {{0, 4}, {0, 5}};
        d.severity = DiagnosticSeverity::Warning;
        d.message = "unused variable";
        diags.append(d);

        tab->setDiagnostics(diags);

        auto selections = tab->editor()->extraSelections();
        bool foundWave = false;
        for (const auto &sel : selections) {
            if (sel.format.underlineStyle() == QTextCharFormat::WaveUnderline) {
                foundWave = true;
                QCOMPARE(sel.format.underlineColor(), QColor(255, 165, 0)); // оранжевый
            }
        }
        QVERIFY(foundWave);

        delete tab;
    }

    void testInfoSquiggle()
    {
        auto *tab = createTab("int x = 0;\n");

        QVector<LSPDiagnostic> diags;
        LSPDiagnostic d;
        d.range = {{0, 4}, {0, 5}};
        d.severity = DiagnosticSeverity::Information;
        d.message = "info hint";
        diags.append(d);

        tab->setDiagnostics(diags);

        auto selections = tab->editor()->extraSelections();
        bool foundWave = false;
        for (const auto &sel : selections) {
            if (sel.format.underlineStyle() == QTextCharFormat::WaveUnderline) {
                foundWave = true;
                QCOMPARE(sel.format.underlineColor(), QColor(0, 120, 255)); // синий
            }
        }
        QVERIFY(foundWave);

        delete tab;
    }

    void testClearOnNew()
    {
        auto *tab = createTab("int main() { return 0; }\n");

        // Устанавливаем диагностику
        QVector<LSPDiagnostic> diags;
        LSPDiagnostic d;
        d.range = {{0, 0}, {0, 3}};
        d.severity = DiagnosticSeverity::Error;
        d.message = "error1";
        diags.append(d);
        tab->setDiagnostics(diags);
        QCOMPARE(tab->diagnostics().size(), 1);

        // Заменяем на пустой список
        tab->setDiagnostics({});
        QVERIFY(tab->diagnostics().isEmpty());

        // Убеждаемся что WaveUnderline нет
        auto selections = tab->editor()->extraSelections();
        bool foundWave = false;
        for (const auto &sel : selections) {
            if (sel.format.underlineStyle() == QTextCharFormat::WaveUnderline)
                foundWave = true;
        }
        QVERIFY(!foundWave);

        delete tab;
    }

    void testTooltipMessage()
    {
        auto *tab = createTab("int main() {}\n");

        QVector<LSPDiagnostic> diags;
        LSPDiagnostic d;
        d.range = {{0, 0}, {0, 3}};
        d.severity = DiagnosticSeverity::Error;
        d.message = "expected type-specifier";
        diags.append(d);
        tab->setDiagnostics(diags);

        auto selections = tab->editor()->extraSelections();
        bool foundTooltip = false;
        for (const auto &sel : selections) {
            if (sel.format.toolTip() == "expected type-specifier") {
                foundTooltip = true;
            }
        }
        QVERIFY(foundTooltip);

        delete tab;
    }

    void testRangeConvert()
    {
        // Проверяем конвертацию LSP-диапазона (0-based строки)
        auto *tab = createTab("line0\nline1\nline2\n");

        QVector<LSPDiagnostic> diags;
        LSPDiagnostic d;
        d.range = {{2, 0}, {2, 5}};
        d.severity = DiagnosticSeverity::Warning;
        d.message = "test";
        diags.append(d);
        tab->setDiagnostics(diags);

        auto selections = tab->editor()->extraSelections();
        bool foundOnLine2 = false;
        for (const auto &sel : selections) {
            if (sel.format.underlineStyle() == QTextCharFormat::WaveUnderline) {
                // Проверяем что курсор на строке 2 (0-based)
                int block = sel.cursor.block().blockNumber();
                QCOMPARE(block, 2);
                foundOnLine2 = true;
            }
        }
        QVERIFY(foundOnLine2);

        delete tab;
    }

    void testMultipleDiagnostics()
    {
        auto *tab = createTab("int a;\nint b;\nint c;\n");

        QVector<LSPDiagnostic> diags;
        for (int i = 0; i < 3; ++i) {
            LSPDiagnostic d;
            d.range = {{i, 4}, {i, 5}};
            d.severity = DiagnosticSeverity::Warning;
            d.message = QString("unused var %1").arg(i);
            diags.append(d);
        }
        tab->setDiagnostics(diags);

        QCOMPARE(tab->diagnostics().size(), 3);

        int waveCount = 0;
        auto selections = tab->editor()->extraSelections();
        for (const auto &sel : selections) {
            if (sel.format.underlineStyle() == QTextCharFormat::WaveUnderline)
                ++waveCount;
        }
        QCOMPARE(waveCount, 3);

        delete tab;
    }

    void testZeroLengthRange()
    {
        // Нулевой диапазон → подчёркивается слово
        auto *tab = createTab("int myVar = 0;\n");

        QVector<LSPDiagnostic> diags;
        LSPDiagnostic d;
        d.range = {{0, 4}, {0, 4}}; // нулевой диапазон
        d.severity = DiagnosticSeverity::Error;
        d.message = "err";
        diags.append(d);
        tab->setDiagnostics(diags);

        auto selections = tab->editor()->extraSelections();
        bool foundWave = false;
        for (const auto &sel : selections) {
            if (sel.format.underlineStyle() == QTextCharFormat::WaveUnderline) {
                foundWave = true;
                // При нулевом диапазоне выделяется слово — курсор должен иметь selection
                QVERIFY(sel.cursor.hasSelection());
            }
        }
        QVERIFY(foundWave);

        delete tab;
    }
};

QTEST_MAIN(TestDiagnosticDisplay)
#include "test_DiagnosticDisplay.moc"
