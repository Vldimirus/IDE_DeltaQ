// Тесты IR — генерация C-кода из инструкций
#include <QtTest>
#include "../../src/codegen/IR.h"

using namespace DeltaQ;

class TestIR : public QObject {
    Q_OBJECT

private slots:
    void emitCall()
    {
        IR ir;
        ir.addInstruction(IRInstruction::makeCall("x", "add", {"a", "b"}));
        QString code = ir.emitCCode();
        QVERIFY(code.contains("x = add(a, b);"));
    }

    void emitCallVoid()
    {
        IR ir;
        ir.addInstruction(IRInstruction::makeCall({}, "printf", {"\"hello\""}));
        QString code = ir.emitCCode();
        QVERIFY(code.contains("printf(\"hello\");"));
    }

    void emitAssign()
    {
        IR ir;
        ir.addInstruction(IRInstruction::makeAssign("x", "42"));
        QString code = ir.emitCCode();
        QVERIFY(code.contains("x = 42;"));
    }

    void emitComment()
    {
        IR ir;
        ir.addInstruction(IRInstruction::makeComment("test comment"));
        QString code = ir.emitCCode();
        QVERIFY(code.contains("// test comment"));
    }

    void emitReturn()
    {
        IR ir;
        ir.addInstruction(IRInstruction::makeReturn("1"));
        QString code = ir.emitCCode();
        QVERIFY(code.contains("return 1;"));
    }

    void declareVar()
    {
        IR ir;
        ir.addInstruction(IRInstruction::makeDeclareVar("x", "int"));
        QString code = ir.emitCCode();
        QVERIFY(code.contains("int x;"));
    }

    void emitTypeConvert()
    {
        IR ir;
        ir.addInstruction(IRInstruction::makeTypeConvert("y", "x", "float"));
        QString code = ir.emitCCode();
        QVERIFY(code.contains("y = (float)x;"));
    }

    void fullProgram()
    {
        IR ir;
        ir.addInclude("<stdio.h>");
        ir.addInstruction(IRInstruction::makeDeclareVar("x", "int"));
        ir.addInstruction(IRInstruction::makeAssign("x", "42"));
        ir.addInstruction(IRInstruction::makeCall({}, "printf", {"\"%d\\n\"", "x"}));
        ir.addInstruction(IRInstruction::makeReturn("0"));

        QString code = ir.emitCCode();
        QVERIFY(code.contains("#include <stdio.h>"));
        QVERIFY(code.contains("int main(void)"));
        QVERIFY(code.contains("int x;"));
        QVERIFY(code.contains("x = 42;"));
        QVERIFY(code.contains("printf(\"%d\\n\", x);"));
        QVERIFY(code.contains("return 0;"));
    }

    void includeNoDuplicates()
    {
        IR ir;
        ir.addInclude("<stdio.h>");
        ir.addInclude("<stdio.h>");
        ir.addInclude("<stdlib.h>");
        QCOMPARE(ir.includes.size(), 2);
    }
};

QTEST_APPLESS_MAIN(TestIR)
#include "test_IR.moc"
