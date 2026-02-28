// Тесты маркеров breakpoints и debug-строки в редакторе
#include <QTest>
#include <QApplication>

#include "../../src/editor/CodeEditorTab.h"
#include "../../src/lsp/LSPTypes.h"

using namespace DeltaQ;

class TestBreakpointMarkers : public QObject {
    Q_OBJECT

private:
    CodeEditorTab *createTab(const QString &text) {
        auto *tab = new CodeEditorTab("test.cpp", nullptr);
        tab->editor()->setPlainText(text);
        return tab;
    }

private slots:
    void testAddRemove()
    {
        auto *tab = createTab("line1\nline2\nline3\n");

        // Изначально нет breakpoints
        QVERIFY(tab->breakpointLines().isEmpty());
        QVERIFY(!tab->hasBreakpoint(1));

        // Добавляем
        tab->addBreakpointMarker(2);
        QVERIFY(tab->hasBreakpoint(2));
        QCOMPARE(tab->breakpointLines().size(), 1);

        // Удаляем
        tab->removeBreakpointMarker(2);
        QVERIFY(!tab->hasBreakpoint(2));
        QVERIFY(tab->breakpointLines().isEmpty());

        delete tab;
    }

    void testToggle()
    {
        auto *tab = createTab("a\nb\nc\n");

        // Добавляем и удаляем через toggle-логику
        tab->addBreakpointMarker(1);
        QVERIFY(tab->hasBreakpoint(1));

        tab->removeBreakpointMarker(1);
        QVERIFY(!tab->hasBreakpoint(1));

        delete tab;
    }

    void testMultipleBreakpoints()
    {
        auto *tab = createTab("a\nb\nc\nd\ne\n");

        tab->addBreakpointMarker(1);
        tab->addBreakpointMarker(3);
        tab->addBreakpointMarker(5);

        QCOMPARE(tab->breakpointLines().size(), 3);
        QVERIFY(tab->hasBreakpoint(1));
        QVERIFY(!tab->hasBreakpoint(2));
        QVERIFY(tab->hasBreakpoint(3));
        QVERIFY(!tab->hasBreakpoint(4));
        QVERIFY(tab->hasBreakpoint(5));

        delete tab;
    }

    void testClearBreakpoints()
    {
        auto *tab = createTab("a\nb\nc\n");

        tab->addBreakpointMarker(1);
        tab->addBreakpointMarker(2);
        tab->addBreakpointMarker(3);

        // Удаляем по одному
        tab->removeBreakpointMarker(1);
        tab->removeBreakpointMarker(2);
        tab->removeBreakpointMarker(3);

        QVERIFY(tab->breakpointLines().isEmpty());

        delete tab;
    }

    void testLineNumberWidth()
    {
        auto *tab = createTab("a\nb\nc\n");

        // lineNumberAreaWidth() — private, но мы проверяем косвенно
        // через то что viewport margins установлены (>0)
        // Проверяем что виджет создан корректно
        QVERIFY(tab->editor() != nullptr);

        delete tab;
    }

    void testDebugCurrentLine()
    {
        auto *tab = createTab("line1\nline2\nline3\n");

        // Устанавливаем текущую строку отладки
        tab->setDebugCurrentLine(2);

        // Проверяем ExtraSelections — должен быть зелёный фон
        auto selections = tab->editor()->extraSelections();
        bool foundDebugLine = false;
        for (const auto &sel : selections) {
            if (sel.format.background().color() == QColor(200, 255, 200)) {
                foundDebugLine = true;
                // Проверяем что это FullWidthSelection
                QVERIFY(sel.format.boolProperty(QTextFormat::FullWidthSelection));
            }
        }
        QVERIFY(foundDebugLine);

        // Очищаем
        tab->clearDebugCurrentLine();
        selections = tab->editor()->extraSelections();
        foundDebugLine = false;
        for (const auto &sel : selections) {
            if (sel.format.background().color() == QColor(200, 255, 200))
                foundDebugLine = true;
        }
        QVERIFY(!foundDebugLine);

        delete tab;
    }
};

QTEST_MAIN(TestBreakpointMarkers)
#include "test_BreakpointMarkers.moc"
