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

#ifndef BONSAIVIEWER_MODELS_MASSCONVERSION_H
#define BONSAIVIEWER_MODELS_MASSCONVERSION_H

#include "BakeBudget.h"

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>

#include <deque>
#include <functional>
#include <vector>

class QProcess;
class QTimer;

// Mass conversions run each file in its own worker process: BonsaiViewer
// re-launched with --mass-convert-worker. A worker gives all its RAM back when
// it exits, and running out of memory only kills that worker, which is then
// retried alone. Bench and figures: tests/mass_convert_bench/README.md.
namespace bonsaiviewer::modules::models {

// True when argv asks for a headless mass conversion (checked before any
// QApplication exists):
//   --mass-convert ifcview|rdb <file.ifc>...   the whole scheduler, outputs next
//                                               to each file (overwritten)
//   --mass-convert-worker ...                   one file (internal, see the .cpp)
bool isMassConvertWorker(int argc, char* argv[]);
// Runs it without any window and returns the process exit code.
int runMassConvertWorker(int argc, char* argv[]);

// Schedules the workers of one mass conversion: biggest files first, several
// at once while the free RAM allows, low CPU priority, a worker stopped when
// RAM runs short and retried alone (on one thread for an .ifcview).
class MassConversionRunner : public QObject {
public:
    struct Outcome {
        int converted = 0;
        int total = 0;
        qint64 elapsed_ms = 0;
        QStringList report;  // one line per converted file
        QStringList errors;  // "<file>: <reason>"
    };

    MassConversionRunner(MassKind kind, const QStringList& inputs, const QStringList& outputs,
                         QObject* parent);
    ~MassConversionRunner() override;

    std::function<void(int done, int total)> on_progress;
    std::function<void(const Outcome&)> on_finished;  // the runner deletes itself after

    void start();

private:
    struct Job {
        QString input;
        QString output;
        qint64 bytes = 0;
        int threads = 1;
        bool alone = false;  // runs only when no other worker does
        int attempts = 0;
    };
    struct Running {
        QProcess* process = nullptr;
        Job job;
        quint64 estimate = 0;
        QElapsedTimer timer;
        bool stopped_for_memory = false;
        int retry_threads = 1;  // when stopped for memory
    };

    void tick();
    void launch(Job job);
    void onWorkerFinished(QProcess* process);
    void retryAlone(Job job, int threads);
    void stopForMemory(Running& running, int retry_threads);
    quint64 runningRemaining() const;
    bool anyRunningAlone() const;

    MassKind kind_;
    int cores_ = 1;
    int max_parallel_ = 1;
    int total_ = 0;
    int done_ = 0;
    std::deque<Job> queue_;
    std::vector<Running> running_;
    QTimer* timer_ = nullptr;
    QElapsedTimer elapsed_;
    Outcome outcome_;
    bool finished_ = false;
};

} // namespace bonsaiviewer::modules::models

#endif // BONSAIVIEWER_MODELS_MASSCONVERSION_H
