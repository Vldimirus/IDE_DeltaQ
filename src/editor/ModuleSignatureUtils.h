#pragma once

#include <QString>

namespace DeltaQ {

struct Module;

// Разбирает C-сигнатуру `dq_*` и синхронизирует контракт модуля с исходником.
bool parseModuleSignatureIntoContract(Module *module, const QString &code);
// Строит C-сигнатуру `dq_*` по текущему контракту модуля.
QString buildModuleSignatureFromContract(const Module &module, QString *errorMessage = nullptr);
// Переписывает сигнатуру функции в исходнике так, чтобы она совпадала с контрактом.
bool rewriteModuleSignatureFromContract(QString *code,
                                        const Module &module,
                                        QString *errorMessage = nullptr);

} // namespace DeltaQ
