// Тесты LayoutEngine
#include <QTest>
#include <QApplication>
#include "../../src/uiDesigner/WidgetItem.h"
#include "../../src/uiDesigner/LayoutEngine.h"
#include <deltaq/UILayout.h>

using namespace DeltaQ;

class TestLayoutEngine : public QObject {
    Q_OBJECT

private:
    WidgetItem *createContainer(int childCount)
    {
        UIWidget container = UIWidget::create("Panel", "container");
        container.geometry = QRectF(0, 0, 300, 200);
        auto *item = new WidgetItem(container);

        for (int i = 0; i < childCount; ++i) {
            UIWidget child = UIWidget::create("Button", QString("btn_%1").arg(i));
            child.geometry = QRectF(0, 0, 60, 30);
            auto *childItem = new WidgetItem(child);
            item->addChildWidget(childItem);
        }

        return item;
    }

private slots:
    void testHBoxLayout()
    {
        auto *container = createContainer(3);
        container->setLayoutType("HBox");
        LayoutEngine::applyLayout(container);

        auto children = container->childWidgets();
        QCOMPARE(children.size(), 3);

        // Все должны быть расположены горизонтально
        for (int i = 1; i < children.size(); ++i) {
            QVERIFY(children[i]->pos().x() > children[i-1]->pos().x());
            QCOMPARE(children[i]->pos().y(), children[0]->pos().y());
        }

        delete container;
    }

    void testVBoxLayout()
    {
        auto *container = createContainer(3);
        container->setLayoutType("VBox");
        LayoutEngine::applyLayout(container);

        auto children = container->childWidgets();
        QCOMPARE(children.size(), 3);

        // Все должны быть расположены вертикально
        for (int i = 1; i < children.size(); ++i) {
            QVERIFY(children[i]->pos().y() > children[i-1]->pos().y());
            QCOMPARE(children[i]->pos().x(), children[0]->pos().x());
        }

        delete container;
    }

    void testGridLayout()
    {
        auto *container = createContainer(4);
        container->setLayoutType("Grid");
        LayoutEngine::applyLayout(container);

        auto children = container->childWidgets();
        QCOMPARE(children.size(), 4);

        // 4 элемента → 2x2 сетка
        // children[0] = (0,0), children[1] = (1,0), children[2] = (0,1), children[3] = (1,1)
        QVERIFY(children[1]->pos().x() > children[0]->pos().x());
        QVERIFY(children[2]->pos().y() > children[0]->pos().y());

        delete container;
    }

    void testFlowLayout()
    {
        // Создаём контейнер 300px шириной с 5 кнопками по 80px
        UIWidget container = UIWidget::create("Panel", "container");
        container.geometry = QRectF(0, 0, 300, 200);
        auto *item = new WidgetItem(container);

        for (int i = 0; i < 5; ++i) {
            UIWidget child = UIWidget::create("Button", QString("btn_%1").arg(i));
            child.geometry = QRectF(0, 0, 80, 30);
            auto *childItem = new WidgetItem(child);
            item->addChildWidget(childItem);
        }

        item->setLayoutType("Flow");
        LayoutEngine::applyLayout(item);

        auto children = item->childWidgets();
        // Должен быть перенос строки (300 - 20 margin = 280, 3 кнопки по 80 = 240 < 280, 4-я уже не влезет)
        // Значит, на первой строке 3 кнопки, на второй 2
        QVERIFY(children[3]->pos().y() > children[0]->pos().y());

        delete item;
    }

    void testNoneLayout()
    {
        auto *container = createContainer(2);
        // layout по умолчанию = None, не должен менять позиции
        LayoutEngine::applyLayout(container);

        // Позиции не должны измениться — все на (0,0)
        auto children = container->childWidgets();
        // При None позиции остаются без изменений
        QCOMPARE(children.size(), 2);

        delete container;
    }

    void testEmptyContainer()
    {
        auto *container = createContainer(0);
        container->setLayoutType("HBox");
        // Не должен упасть
        LayoutEngine::applyLayout(container);
        QCOMPARE(container->childWidgets().size(), 0);
        delete container;
    }
};

QTEST_MAIN(TestLayoutEngine)
#include "test_LayoutEngine.moc"
