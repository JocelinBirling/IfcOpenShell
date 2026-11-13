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

#ifndef BONSAIVIEWER_MODELS_BAKEBUDGET_H
#define BONSAIVIEWER_MODELS_BAKEBUDGET_H

#include <QtGlobal>

// Resource budget of the mass conversions (.ifcview / .rdb), one worker
// process per file. The numbers come from the Big_Model bench, see
// tests/mass_convert_bench/README.md.
namespace bonsaiviewer::modules::models {

enum class MassKind { IfcView, Rdb };

// Free RAM kept when deciding to start one more file next to running ones.
constexpr quint64 kMassStartReserveBytes = 1024ull * 1024 * 1024;
// Below this much free RAM a running worker is stopped and retried alone.
constexpr quint64 kMassKillFloorBytes = 384ull * 1024 * 1024;

// RAM the system can hand out right now without swapping, in bytes; 0 when the
// platform does not tell.
quint64 availableMemoryBytes();

// Resident RAM of another process (a worker), in bytes; 0 when unknown.
quint64 processResidentBytes(qint64 pid);

// Lowers the calling process's CPU priority so the PC stays responsive. Call
// before starting any thread (on Linux the nice value is per thread and is
// inherited by the threads created afterwards).
void lowerOwnPriority();

// Geometry threads of one .ifcview bake: past 4 a file is no faster (its time
// goes to the single-threaded parse and to a few heavy elements), so the cores
// are better spent on other files.
int ifcViewThreadsPerFile(int cores);

// Files converted at the same time, before the RAM check.
int maxParallelFiles(MassKind kind, int cores);

// Pessimistic peak RAM of one worker for an IFC of `ifc_bytes`.
quint64 estimatedPeakBytes(MassKind kind, qint64 ifc_bytes);

// May one more worker of `next_estimate` start while others run? `available`
// is the free RAM now (0 = unknown: never), `running_remaining` what the
// running workers may still take on top of what they already use.
bool canStartAnother(quint64 available, quint64 running_remaining, quint64 next_estimate);

} // namespace bonsaiviewer::modules::models

#endif // BONSAIVIEWER_MODELS_BAKEBUDGET_H
