// Тесты user-facing framing LibraryImportWizard для imported pack v1.
#include <QTest>

#include "../../src/libProcessor/LibraryImportWizard.h"

using namespace DeltaQ;

class TestLibraryImportWizard : public QObject {
    Q_OBJECT

private slots:
    void wizardMakesLinuxFirstCApiLimitsVisible()
    {
        LibraryImportWizard wizard(nullptr);

        QVERIFY(wizard.page(0) != nullptr);
        QVERIFY(wizard.page(4) != nullptr);

        const QString intakeSubtitle = wizard.page(0)->subTitle();
        QVERIFY2(intakeSubtitle.contains("Linux-first C ABI"), qPrintable(intakeSubtitle));
        QVERIFY2(intakeSubtitle.contains("C++/DLL"), qPrintable(intakeSubtitle));

        const QString completionSubtitle = wizard.page(4)->subTitle();
        QVERIFY2(completionSubtitle.contains("Linux-first C ABI pack"), qPrintable(completionSubtitle));
        QVERIFY2(completionSubtitle.contains("missing artifacts"), qPrintable(completionSubtitle));
        QVERIFY2(completionSubtitle.contains("half-written pack"), qPrintable(completionSubtitle));
    }
};

QTEST_MAIN(TestLibraryImportWizard)
#include "test_LibraryImportWizard.moc"
