// This file was generated with the assistance of an AI coding tool.
// Bench of the mass conversions, one IFC per process so the peak RAM is the
// file's own (target bonsaiviewer_mass_convert_bench, not built by default):
//
//   bonsaiviewer_mass_convert_bench ifcview <file.ifc> [threads|auto] [zstd]
//   bonsaiviewer_mass_convert_bench rdb <file.ifc>
//
// "auto" picks the threads like the mass tool (ifcViewThreadsPerFile). Prints
// one "RESULT key=value ..." line. How to use it: mass_convert_bench/README.md.
#include "modules/models/BakeBudget.h"

#include "../ifcviewer/IfcViewBuilder.h"
#include "../ifcgeom/serializer.h"
#include "../serializers/document_serializer_plugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#include <sys/resource.h>
#if defined(__GLIBC__)
#  include <malloc.h>
#endif

#include <iostream>

using namespace bonsaiviewer::modules::models;

namespace {

qint64 peakRssMib() {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024;  // kB on Linux
}

qint64 currentRssMib() {
    QFile status(QStringLiteral("/proc/self/status"));
    if (!status.open(QIODevice::ReadOnly)) return -1;
    for (const QByteArray& line : status.readAll().split('\n')) {
        if (line.startsWith("VmRSS:")) return line.simplified().split(' ').value(1).toLongLong() / 1024;
    }
    return -1;
}

int bakeIfcView(const QString& input, const QString& threads_arg, int zstd) {
    const qint64 ifc_bytes = QFileInfo(input).size();
    const int threads = threads_arg == QLatin1String("auto")
                            ? ifcViewThreadsPerFile(QThread::idealThreadCount())
                            : threads_arg.toInt();

    QElapsedTimer timer;
    timer.start();
    bool ok = false;
    QString error;
    IfcViewBuilder::Timings t;
    {
        IfcViewBuilder builder;
        ok = builder.build(input, input, threads, zstd);
        error = builder.lastError();
        t = builder.timings();
    }
    const qint64 total = timer.elapsed();
    const qint64 rss_after = currentRssMib();
#if defined(__GLIBC__)
    malloc_trim(0);
#endif
    const qint64 rss_trimmed = currentRssMib();

    std::cout << "RESULT kind=ifcview file=" << QFileInfo(input).fileName().toStdString()
              << " mib=" << ifc_bytes / (1024 * 1024) << " threads=" << threads << " zstd=" << zstd
              << " first_mesh=" << t.first_mesh_ms << " stream=" << t.stream_ms
              << " finalize=" << t.finalize_ms << " write=" << t.write_ms << " total=" << total
              << " peak_mib=" << peakRssMib() << " rss_after_mib=" << rss_after
              << " rss_trimmed_mib=" << rss_trimmed << " ok=" << ok;
    if (!ok) std::cout << " error=\"" << error.toStdString() << "\"";
    std::cout << std::endl;
    return ok ? 0 : 1;
}

int convertRdb(const QString& input) {
    const QFileInfo info(input);
    const QString output = info.absoluteDir().filePath(info.completeBaseName() + ".rdb");
    QElapsedTimer timer;
    timer.start();
    QString error;
    try {
        ifcopenshell::serializers::document_serializer_context context;
        context.file = nullptr;
        context.input_filename = input.toStdString();
        context.output_filename = output.toStdString();
        context.stream = true;
        auto& registry = ifcopenshell::serializers::document_serializer_registry_instance();
        if (!registry.find("rdb")) {
            error = QStringLiteral("no rdb serializer");
        } else {
            registry.create("rdb", context)->finalize();
        }
    } catch (const std::exception& e) {
        error = QString::fromUtf8(e.what());
    }
    std::cout << "RESULT kind=rdb file=" << info.fileName().toStdString()
              << " mib=" << info.size() / (1024 * 1024) << " total=" << timer.elapsed()
              << " peak_mib=" << peakRssMib() << " ok=" << error.isEmpty();
    if (!error.isEmpty()) std::cout << " error=\"" << error.toStdString() << "\"";
    std::cout << std::endl;
    return error.isEmpty() ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    // Same identity as BonsaiViewer, so AppSettings reads the user's geometry settings.
    app.setApplicationName("Bonsai Viewer");
    app.setOrganizationName("IfcOpenShell");

    const QStringList args = app.arguments();
    if (args.size() >= 3 && args[1] == QLatin1String("ifcview")) {
        return bakeIfcView(args[2], args.value(3, QStringLiteral("auto")), args.value(4, QStringLiteral("9")).toInt());
    }
    if (args.size() >= 3 && args[1] == QLatin1String("rdb")) return convertRdb(args[2]);
    std::cerr << "usage: " << argv[0] << " ifcview <file.ifc> [threads|auto] [zstd]\n"
              << "       " << argv[0] << " rdb <file.ifc>\n";
    return 2;
}
