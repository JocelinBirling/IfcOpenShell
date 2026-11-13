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

#include "BakeBudget.h"

#include <QFile>

#include <algorithm>

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <psapi.h>
#else
#  include <sys/resource.h>
#  if defined(__APPLE__)
#    include <mach/mach.h>
#  endif
#endif

namespace bonsaiviewer::modules::models {

namespace {

constexpr quint64 MiB = 1024ull * 1024;

#if !defined(_WIN32) && !defined(__APPLE__)
// One "<key>: <value> kB" line of a /proc file, in bytes; 0 if absent. /proc
// files report a size of 0, so they are read whole instead of with atEnd().
quint64 procKbField(const QString& path, const QByteArray& key) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;
    for (const QByteArray& line : file.readAll().split('\n')) {
        if (line.startsWith(key)) {
            const QList<QByteArray> parts = line.simplified().split(' ');
            if (parts.size() >= 2) return parts[1].toULongLong() * 1024;
        }
    }
    return 0;
}
#endif

} // namespace

quint64 availableMemoryBytes() {
#if defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) return status.ullAvailPhys;
    return 0;
#elif defined(__APPLE__)
    vm_statistics64_data_t stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&stats), &count) != KERN_SUCCESS) {
        return 0;
    }
    vm_size_t page_size = 0;
    if (host_page_size(mach_host_self(), &page_size) != KERN_SUCCESS) return 0;
    return (quint64(stats.free_count) + stats.inactive_count + stats.purgeable_count) * page_size;
#else
    quint64 available = procKbField(QStringLiteral("/proc/meminfo"), "MemAvailable:");
    // A cgroup v2 memory limit (container, flatpak, systemd scope) may leave
    // less than the whole system: take the tightest headroom up the tree.
    QFile cgroup(QStringLiteral("/proc/self/cgroup"));
    if (cgroup.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (const QByteArray& line : cgroup.readAll().split('\n')) {
            if (!line.startsWith("0::")) continue;
            QString dir = QStringLiteral("/sys/fs/cgroup") + QString::fromUtf8(line.mid(3)).trimmed();
            while (dir.startsWith(QLatin1String("/sys/fs/cgroup/"))) {
                QFile max_file(dir + QStringLiteral("/memory.max"));
                QFile current_file(dir + QStringLiteral("/memory.current"));
                if (max_file.open(QIODevice::ReadOnly) && current_file.open(QIODevice::ReadOnly)) {
                    bool ok_max = false, ok_current = false;
                    const quint64 max = max_file.readAll().trimmed().toULongLong(&ok_max);  // "max" -> !ok
                    const quint64 current = current_file.readAll().trimmed().toULongLong(&ok_current);
                    if (ok_max && ok_current) available = std::min(available, max > current ? max - current : 0);
                }
                dir = dir.left(dir.lastIndexOf(QLatin1Char('/')));
            }
        }
    }
    return available;
#endif
}

quint64 processResidentBytes(qint64 pid) {
    if (pid <= 0) return 0;
#if defined(_WIN32)
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));
    if (!process) return 0;
    PROCESS_MEMORY_COUNTERS counters;
    const BOOL ok = K32GetProcessMemoryInfo(process, &counters, sizeof(counters));
    CloseHandle(process);
    return ok ? quint64(counters.WorkingSetSize) : 0;
#elif defined(__APPLE__)
    return 0;  // unknown: the scheduler then counts the whole estimate
#else
    return procKbField(QStringLiteral("/proc/%1/status").arg(pid), "VmRSS:");
#endif
}

void lowerOwnPriority() {
#if defined(_WIN32)
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#else
    setpriority(PRIO_PROCESS, 0, 10);
#endif
}

int ifcViewThreadsPerFile(int cores) {
    return std::clamp(cores - 1, 1, 4);
}

int maxParallelFiles(MassKind kind, int cores) {
    // .ifcview: 4 threads per file, so cores / 4 files fill the machine.
    // .rdb: about one core per file; past 4 files the bench gained nothing.
    if (kind == MassKind::IfcView) return std::max(1, cores / 4);
    return std::clamp(cores - 1, 1, 4);
}

quint64 estimatedPeakBytes(MassKind kind, qint64 ifc_bytes) {
    const quint64 bytes = quint64(std::max<qint64>(0, ifc_bytes));
    // Bench peaks: .ifcview 466 MiB -> 4.2 GB, 12 MiB -> 1.1 GB; .rdb 452 MiB -> 1.55 GB.
    if (kind == MassKind::IfcView) return 1024 * MiB + bytes * 9;
    return 256 * MiB + bytes * 4;
}

bool canStartAnother(quint64 available, quint64 running_remaining, quint64 next_estimate) {
    if (available == 0) return false;
    return available >= running_remaining + next_estimate + kMassStartReserveBytes;
}

} // namespace bonsaiviewer::modules::models
