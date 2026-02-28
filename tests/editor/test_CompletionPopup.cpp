// Тесты popup-окна автодополнения
#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QListWidget>
#include <QPlainTextEdit>

#include "../../src/editor/CompletionPopup.h"
#include "../../src/lsp/LSPTypes.h"

using namespace DeltaQ;

class TestCompletionPopup : public QObject {
    Q_OBJECT

private:
    QVector<CompletionItem> createItems() {
        QVector<CompletionItem> items;

        CompletionItem i1;
        i1.label = "printf";
        i1.kind = CompletionItemKind::Function;
        i1.insertText = "printf";
        i1.detail = "int printf(const char*, ...)";
        items.append(i1);

        CompletionItem i2;
        i2.label = "puts";
        i2.kind = CompletionItemKind::Function;
        i2.insertText = "puts";
        items.append(i2);

        CompletionItem i3;
        i3.label = "perror";
        i3.kind = CompletionItemKind::Function;
        i3.insertText = "perror";
        items.append(i3);

        CompletionItem i4;
        i4.label = "main";
        i4.kind = CompletionItemKind::Function;
        i4.insertText = "main";
        items.append(i4);

        CompletionItem i5;
        i5.label = "myVar";
        i5.kind = CompletionItemKind::Variable;
        i5.insertText = "myVar";
        items.append(i5);

        return items;
    }

private slots:
    void testSetItems()
    {
        CompletionPopup popup;
        auto items = createItems();
        popup.setItems(items);

        // Проверяем что все элементы добавлены
        auto *list = popup.findChild<QListWidget *>();
        QVERIFY(list != nullptr);
        QCOMPARE(list->count(), 5);
    }

    void testFilterByPrefix()
    {
        CompletionPopup popup;
        popup.setItems(createItems());

        popup.updateFilter("p");

        auto *list = popup.findChild<QListWidget *>();
        QVERIFY(list != nullptr);
        // printf, puts, perror — 3 элемента начинаются с "p"
        QCOMPARE(list->count(), 3);
    }

    void testFilterByPrefixCase()
    {
        CompletionPopup popup;
        popup.setItems(createItems());

        // Фильтрация case-insensitive
        popup.updateFilter("P");

        auto *list = popup.findChild<QListWidget *>();
        QVERIFY(list != nullptr);
        QCOMPARE(list->count(), 3);
    }

    void testEmptyHides()
    {
        CompletionPopup popup;
        popup.setItems(createItems());
        popup.popup(QPoint(0, 0));

        QVERIFY(popup.isVisible());

        // Фильтр без совпадений — скрывается
        popup.updateFilter("xyz");
        QVERIFY(!popup.isVisible());
    }

    void testInsertItem()
    {
        CompletionPopup popup;
        popup.setItems(createItems());

        // Проверяем сигнал itemSelected
        QSignalSpy spy(&popup, &CompletionPopup::itemSelected);
        auto *list = popup.findChild<QListWidget *>();
        QVERIFY(list != nullptr);
        QVERIFY(list->count() > 0);

        // Симулируем double-click
        list->setCurrentRow(0);
        emit list->itemDoubleClicked(list->currentItem());

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), "printf");
    }

    void testPositionCalc()
    {
        CompletionPopup popup;
        QPoint pos(100, 200);
        popup.setItems(createItems());
        popup.popup(pos);

        QCOMPARE(popup.pos(), pos);
        QVERIFY(popup.isVisible());

        popup.hide();
    }

    void testTriggerChars()
    {
        // Проверяем что CompletionItem правильно парсится из JSON
        QJsonObject json;
        json["label"] = "member";
        json["kind"] = static_cast<int>(CompletionItemKind::Field);
        json["insertText"] = "member";
        json["detail"] = "int";

        auto item = CompletionItem::fromJson(json);
        QCOMPARE(item.label, "member");
        QCOMPARE(item.kind, CompletionItemKind::Field);
        QCOMPARE(item.insertText, "member");
    }

    void testEmptyItems()
    {
        CompletionPopup popup;
        popup.setItems({});

        auto *list = popup.findChild<QListWidget *>();
        QVERIFY(list != nullptr);
        QCOMPARE(list->count(), 0);
    }
};

QTEST_MAIN(TestCompletionPopup)
#include "test_CompletionPopup.moc"
