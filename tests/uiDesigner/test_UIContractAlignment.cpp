// Тесты выравнивания UI contract vocabulary в дизайнере.
#include <QApplication>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QTest>
#include <QTreeWidget>

#include "../../src/uiDesigner/DesignScene.h"
#include "../../src/uiDesigner/ObjectTreeWidget.h"
#include "../../src/uiDesigner/PropertyEditor.h"
#include "../../src/uiDesigner/WidgetItem.h"

using namespace DeltaQ;

class TestUIContractAlignment : public QObject {
    Q_OBJECT

private:
    static QStringList collectLabelTexts(QWidget *widget)
    {
        QStringList texts;
        const auto labels = widget->findChildren<QLabel *>();
        for (const auto *label : labels)
            texts.append(label->text());
        return texts;
    }

private slots:
    void propertyEditorUsesSharedContractPropertiesForComboBox()
    {
        UIWidget combo = UIWidget::create("ComboBox", "combo1");
        combo.properties["items"] = "Red,Green,Blue";
        combo.properties["selected"] = 2;

        WidgetItem item(combo);
        PropertyEditor editor;
        editor.setWidget(&item);
        QCoreApplication::processEvents();

        const QStringList labels = collectLabelTexts(&editor);
        QVERIFY(labels.contains("Contract:"));
        QVERIFY(labels.contains("combo_box"));
        QVERIFY(labels.contains("Items:"));
        QVERIFY(labels.contains("Selected:"));
        QVERIFY(!labels.contains("Text:"));
    }

    void objectTreeShowsDisplayNameAndContractType()
    {
        DesignScene scene;
        UIWidget combo = UIWidget::create("ComboBox", "combo1");
        combo.properties["items"] = "One,Two";
        scene.addWidgetItem(combo);

        ObjectTreeWidget tree;
        tree.rebuild(&scene);

        auto *treeWidget = tree.findChild<QTreeWidget *>();
        QVERIFY(treeWidget != nullptr);
        QVERIFY(treeWidget->topLevelItemCount() > 0);
        auto *root = treeWidget->topLevelItem(0);
        QVERIFY(root->childCount() > 0);
        QCOMPARE(root->child(0)->text(0), QString("combo1 (Combo Box) [combo_box]"));
    }

    void windowPropertyEditorsStayInSyncAfterMinimumSizeClamp()
    {
        DesignScene scene;
        scene.setWindowRect(QRectF(0, 0, 320, 240));

        PropertyEditor editor;
        editor.setWindowProperties(&scene);
        QCoreApplication::processEvents();

        auto *widthSpin = editor.findChild<QDoubleSpinBox *>(QStringLiteral("windowWidthSpinBox"));
        auto *heightSpin = editor.findChild<QDoubleSpinBox *>(QStringLiteral("windowHeightSpinBox"));
        auto *minWidthSpin = editor.findChild<QDoubleSpinBox *>(QStringLiteral("windowMinWidthSpinBox"));
        auto *minHeightSpin = editor.findChild<QDoubleSpinBox *>(QStringLiteral("windowMinHeightSpinBox"));

        QVERIFY(widthSpin != nullptr);
        QVERIFY(heightSpin != nullptr);
        QVERIFY(minWidthSpin != nullptr);
        QVERIFY(minHeightSpin != nullptr);

        minWidthSpin->setValue(480);
        minHeightSpin->setValue(360);
        QCoreApplication::processEvents();

        QCOMPARE(scene.windowMinimumSize(), QSizeF(480, 360));
        QCOMPARE(scene.windowRect(), QRectF(0, 0, 480, 360));
        QCOMPARE(widthSpin->minimum(), 480.0);
        QCOMPARE(heightSpin->minimum(), 360.0);
        QCOMPARE(widthSpin->value(), 480.0);
        QCOMPARE(heightSpin->value(), 360.0);
    }
};

QTEST_MAIN(TestUIContractAlignment)
#include "test_UIContractAlignment.moc"
