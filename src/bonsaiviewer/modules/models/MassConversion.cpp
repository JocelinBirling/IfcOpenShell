// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "MassConversion.h"

#include "Commands.h"
#include "../../../ifcviewer/IfcViewBuilder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <cstring>
#include <iostream>

namespace bonsaiviewer::modules::models {

namespace {

constexpr const char* kWorkerFlag = "--mass-convert-worker";
constexpr const char* kCommandFlag = "--mass-convert";
// zstd level of the mass .ifcview bake: far faster than the viewer's 19 for a
// slightly bigger cache; decode speed does not depend on it.
constexpr int kMassBakeZstdLevel = 9;
// Worker exit code for a file that cannot be converted (no retry). Any other
// non-zero exit or a crash (e.g. killed for lack of RAM) is retried once.
constexpr int kWorkerFileError = 3;
constexpr int kTickMs = 250;

QString seconds(qint64 ms) {
    return QString::number(ms / 1000.0, 'f', 1) + QStringLiteral(" s");
}

} // namespace

bool isMassConvertWorker(int argc, char* argv[]) {
    return argc >= 2 && (std::strcmp(argv[1], kWorkerFlag) == 0 || std::strcmp(argv[1], kCommandFlag) == 0);
}

namespace {

// <exe> --mass-convert ifcview|rdb <file.ifc>...
int runMassConvertCommand(QCoreApplication& app) {
    const QStringList args = app.arguments();
    const QString kind_name = args.value(2);
    if (args.size() < 4 || (kind_name != QLatin1String("ifcview") && kind_name != QLatin1String("rdb"))) {
        std::cerr << "usage: " << args.value(0).toStdString() << " " << kCommandFlag
                  << " ifcview|rdb <file.ifc>...\n";
        return 2;
    }
    const MassKind kind = kind_name == QLatin1String("ifcview") ? MassKind::IfcView : MassKind::Rdb;
    QStringList inputs, outputs;
    for (int i = 3; i < args.size(); ++i) {
        const QFileInfo info(args[i]);
        inputs << info.absoluteFilePath();
        outputs << info.absoluteDir().filePath(info.completeBaseName() + QLatin1Char('.') + kind_name);
    }
    int exit_code = 0;
    auto* runner = new MassConversionRunner(kind, inputs, outputs, &app);
    runner->on_progress = [](int done, int total) { std::cout << done << "/" << total << std::endl; };
    runner->on_finished = [&exit_code](const MassConversionRunner::Outcome& outcome) {
        for (const QString& line : outcome.report) std::cout << line.toStdString() << "\n";
        for (const QString& line : outcome.errors) std::cout << "FAILED " << line.toStdString() << "\n";
        std::cout << outcome.converted << "/" << outcome.total << " converted in "
                  << seconds(outcome.elapsed_ms).toStdString() << std::endl;
        exit_code = outcome.converted == outcome.total ? 0 : 1;
        QCoreApplication::quit();
    };
    QTimer::singleShot(0, runner, [runner]() { runner->start(); });
    app.exec();
    return exit_code;
}

} // namespace

// argv: <exe> --mass-convert-worker ifcview <ifc> <threads>
//       <exe> --mass-convert-worker rdb <ifc> <output.rdb>
int runMassConvertWorker(int argc, char* argv[]) {
    const bool command = std::strcmp(argv[1], kCommandFlag) == 0;
    lowerOwnPriority();  // before any thread exists, so every thread inherits it
    QCoreApplication app(argc, argv);
    // Same identity as the viewer, so AppSettings reads the user's geometry settings.
    app.setApplicationName("Bonsai Viewer");
    app.setOrganizationName("IfcOpenShell");
    if (command) return runMassConvertCommand(app);

    const QStringList args = app.arguments();
    if (args.size() < 5) {
        std::cout << "ERROR bad worker arguments" << std::endl;
        return 2;
    }
    const QString& kind = args[2];
    const QString& input = args[3];
    QString error;
    bool ok = false;
    if (!QFileInfo(input).isFile()) {
        error = QStringLiteral("file not found");
    } else if (kind == QLatin1String("ifcview")) {
        IfcViewBuilder builder;
        ok = builder.build(input, input, std::max(1, args[4].toInt()), kMassBakeZstdLevel);
        if (!ok) error = builder.lastError();
    } else if (kind == QLatin1String("rdb")) {
        ok = commands::detail::convertOneIfcToRdb(input, args[4], error);
    } else {
        error = QStringLiteral("unknown worker kind '%1'").arg(kind);
    }
    if (!ok) {
        std::cout << "ERROR " << error.simplified().toStdString() << std::endl;
        return kWorkerFileError;
    }
    std::cout << "OK" << std::endl;
    return 0;
}

MassConversionRunner::MassConversionRunner(MassKind kind, const QStringList& inputs,
                                           const QStringList& outputs, QObject* parent)
    : QObject(parent), kind_(kind) {
    cores_ = std::max(1, QThread::idealThreadCount());
    max_parallel_ = maxParallelFiles(kind_, cores_);
    for (int i = 0; i < inputs.size(); ++i) {
        Job job;
        job.input = inputs[i];
        job.output = outputs.value(i);
        job.bytes = QFileInfo(inputs[i]).size();
        job.threads = kind_ == MassKind::IfcView ? ifcViewThreadsPerFile(cores_) : 1;
        queue_.push_back(job);
    }
    // Biggest first: the long files start early instead of ending the run alone.
    std::stable_sort(queue_.begin(), queue_.end(),
                     [](const Job& a, const Job& b) { return a.bytes > b.bytes; });
    total_ = int(queue_.size());
    outcome_.total = total_;
    timer_ = new QTimer(this);
    timer_->setInterval(kTickMs);
    connect(timer_, &QTimer::timeout, this, [this]() { tick(); });
}

MassConversionRunner::~MassConversionRunner() {
    for (Running& running : running_) {
        running.process->disconnect(this);
        running.process->kill();
        running.process->waitForFinished(2000);
    }
}

void MassConversionRunner::start() {
    elapsed_.start();
    timer_->start();
    tick();
}

quint64 MassConversionRunner::runningRemaining() const {
    quint64 remaining = 0;
    for (const Running& running : running_) {
        const quint64 used = processResidentBytes(running.process->processId());
        remaining += running.estimate > used ? running.estimate - used : 0;
    }
    return remaining;
}

bool MassConversionRunner::anyRunningAlone() const {
    return std::any_of(running_.begin(), running_.end(), [](const Running& r) { return r.job.alone; });
}

void MassConversionRunner::stopForMemory(Running& running, int retry_threads) {
    running.stopped_for_memory = true;
    running.retry_threads = retry_threads;
    running.process->kill();
}

void MassConversionRunner::tick() {
    if (finished_) return;

    // Watchdog first: the PC must not run out of RAM (it would swap and freeze).
    // One stop at a time: wait for the stopped worker's RAM to come back.
    const bool stopping = std::any_of(running_.begin(), running_.end(),
                                      [](const Running& r) { return r.stopped_for_memory; });
    const quint64 available = availableMemoryBytes();
    if (!stopping && available != 0 && available < kMassKillFloorBytes && !running_.empty()) {
        Running& latest = running_.back();
        if (running_.size() > 1) {
            stopForMemory(latest, latest.job.threads);  // retried alone, same threads
        } else if (latest.job.threads > 1) {
            stopForMemory(latest, 1);  // alone but multi-threaded: retried on one thread
        }
    }

    while (!queue_.empty() && int(running_.size()) < max_parallel_) {
        const Job& next = queue_.front();
        if (!running_.empty()) {
            if (next.alone || anyRunningAlone()) break;
            if (!canStartAnother(availableMemoryBytes(), runningRemaining(),
                                 estimatedPeakBytes(kind_, next.bytes))) {
                break;
            }
        }
        Job job = next;
        queue_.pop_front();
        launch(job);
    }

    if (queue_.empty() && running_.empty()) {
        finished_ = true;
        timer_->stop();
        outcome_.elapsed_ms = elapsed_.elapsed();
        qInfo().noquote() << QStringLiteral("Mass conversion: %1/%2 converted in %3")
                                 .arg(outcome_.converted).arg(total_).arg(seconds(outcome_.elapsed_ms));
        if (on_finished) on_finished(outcome_);
        deleteLater();
    }
}

void MassConversionRunner::launch(Job job) {
    auto* process = new QProcess(this);
    process->setStandardErrorFile(QProcess::nullDevice());
    QStringList args{QString::fromLatin1(kWorkerFlag)};
    if (kind_ == MassKind::IfcView) {
        args << QStringLiteral("ifcview") << job.input << QString::number(job.threads);
    } else {
        args << QStringLiteral("rdb") << job.input << job.output;
    }
    connect(process, &QProcess::finished, this, [this, process]() { onWorkerFinished(process); });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) onWorkerFinished(process);
    });

    Running running;
    running.process = process;
    running.job = job;
    running.estimate = estimatedPeakBytes(kind_, job.bytes);
    running.timer.start();
    running_.push_back(running);
    process->start(QCoreApplication::applicationFilePath(), args);
}

void MassConversionRunner::retryAlone(Job job, int threads) {
    job.alone = true;
    job.threads = threads;
    ++job.attempts;
    queue_.push_front(job);
}

void MassConversionRunner::onWorkerFinished(QProcess* process) {
    auto it = std::find_if(running_.begin(), running_.end(),
                           [process](const Running& r) { return r.process == process; });
    if (it == running_.end()) return;
    const Running running = *it;
    running_.erase(it);

    const QString name = QFileInfo(running.job.input).fileName();
    const QString out = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
    const bool crashed = process->exitStatus() != QProcess::NormalExit ||
                         process->error() == QProcess::FailedToStart;
    const int code = crashed ? -1 : process->exitCode();
    process->deleteLater();

    if (running.stopped_for_memory) {
        qInfo().noquote() << "Mass conversion:" << name << "stopped for lack of RAM, retried alone";
        retryAlone(running.job, running.retry_threads);
        tick();
        return;
    }
    if (code == 0) {
        ++outcome_.converted;
        QString line = QStringLiteral("%1: %2").arg(name, seconds(running.timer.elapsed()));
        if (kind_ == MassKind::IfcView) line += QStringLiteral(", %1 thread(s)").arg(running.job.threads);
        if (running.job.attempts > 0) line += QStringLiteral(", retried alone");
        outcome_.report << line;
        qInfo().noquote() << "Mass conversion:" << line;
    } else if (code != kWorkerFileError && running.job.attempts == 0) {
        // Crashed or killed (often by the OS for lack of RAM): once more, alone.
        qInfo().noquote() << "Mass conversion:" << name << "worker failed, retried alone";
        retryAlone(running.job, 1);
        tick();
        return;
    } else {
        QString reason = out.startsWith(QLatin1String("ERROR ")) ? out.mid(6) : out;
        if (reason.isEmpty()) reason = QStringLiteral("the conversion process stopped unexpectedly");
        outcome_.errors << QStringLiteral("%1: %2").arg(name, reason);
        qInfo().noquote() << "Mass conversion:" << name << "failed:" << reason;
    }
    ++done_;
    if (on_progress) on_progress(done_, total_);
    tick();
}

} // namespace bonsaiviewer::modules::models
