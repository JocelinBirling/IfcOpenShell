// This file was generated with the assistance of an AI coding tool.
// The resource budget of the mass conversions (target bonsaiviewer_bake_budget_tests).
#include "modules/models/BakeBudget.h"

#include <QCoreApplication>
#include <QString>

#include <iostream>

using namespace bonsaiviewer::modules::models;

namespace {

int failures = 0, checks = 0;

void check(bool ok, const QString& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAILED: " << what.toStdString() << "\n";
    }
}

constexpr qint64 MiB = 1024 * 1024;
constexpr quint64 GiB = 1024ull * 1024 * 1024;

} // namespace

int main() {
    // Threads per .ifcview file: cores - 1, never above 4, never 0.
    check(ifcViewThreadsPerFile(12) == 4, "12 cores -> 4 threads");
    check(ifcViewThreadsPerFile(4) == 3, "4 cores -> 3 threads");
    check(ifcViewThreadsPerFile(1) == 1, "1 core -> 1 thread");
    check(ifcViewThreadsPerFile(0) == 1, "no core count -> 1 thread");

    // Files at once before the RAM check.
    check(maxParallelFiles(MassKind::IfcView, 12) == 3, "ifcview, 12 cores -> 3 files");
    check(maxParallelFiles(MassKind::IfcView, 4) == 1, "ifcview, 4 cores -> 1 file");
    check(maxParallelFiles(MassKind::IfcView, 1) == 1, "ifcview, 1 core -> 1 file");
    check(maxParallelFiles(MassKind::Rdb, 12) == 4, "rdb, 12 cores -> 4 files");
    check(maxParallelFiles(MassKind::Rdb, 2) == 1, "rdb, 2 cores -> 1 file");

    // Estimates cover the bench peaks (466 MiB IFC -> 4.2 GB .ifcview, 452 MiB -> 1.55 GB .rdb).
    check(estimatedPeakBytes(MassKind::IfcView, 466 * MiB) >= quint64(4.2 * GiB), "ifcview estimate >= 4.2 GB");
    check(estimatedPeakBytes(MassKind::IfcView, 12 * MiB) >= quint64(1.1 * GiB), "small ifcview estimate >= 1.1 GB");
    check(estimatedPeakBytes(MassKind::Rdb, 452 * MiB) >= quint64(1.55 * GiB), "rdb estimate >= 1.55 GB");

    // Starting one more file next to running ones.
    check(canStartAnother(10 * GiB, 2 * GiB, 3 * GiB), "10 GB free, 2 + 3 + 1 reserve -> yes");
    check(!canStartAnother(5 * GiB, 2 * GiB, 3 * GiB), "5 GB free, 2 + 3 + 1 reserve -> no");
    check(!canStartAnother(0, 0, 1 * GiB), "unknown RAM -> no second file");

    // The platform queries answer something on the build machine.
    check(availableMemoryBytes() > 0, "availableMemoryBytes() > 0");
#if defined(__linux__) || defined(_WIN32)
    check(processResidentBytes(QCoreApplication::applicationPid()) > 0, "processResidentBytes(own pid) > 0");
#endif
    check(processResidentBytes(0) == 0, "processResidentBytes(0) == 0");

    std::cout << "available RAM seen: " << availableMemoryBytes() / MiB << " MiB\n";
    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
