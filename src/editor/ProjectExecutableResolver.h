// Resolver runtime-бинарника проекта внутри build-tree.
#pragma once

#include <QString>

namespace DeltaQ {

class ProjectExecutableResolver {
public:
    QString resolve(const QString &buildDir, const QString &targetName) const;

private:
    bool isRunnableExecutable(const QString &path) const;
};

} // namespace DeltaQ
