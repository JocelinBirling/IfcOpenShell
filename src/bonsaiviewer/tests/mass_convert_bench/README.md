<!-- This file was generated with the assistance of an AI coding tool. -->

# Mass conversion bench (.ifcview / .rdb)

How fast the Add Model TOOLS mass conversions ("Mass Build .ifcview", "Mass
Convert IFC to Database") can go, how much RAM they take, and why the
scheduler in `modules/models/MassConversion.cpp` is built the way it is.

Goal set for it: **a mass conversion must always finish on an 8 GB RAM machine**
(slowly if need be), **be as fast as possible when there is more RAM**, and
**never freeze the PC**.

Measured on 2026-11-13. Reuse the tools below to redo it on other files or
another machine.

## Tools

| Tool | What it does |
|---|---|
| `bonsaiviewer_mass_convert_bench` (target, not built by default) | One IFC per process: `ifcview <file> [threads\|auto] [zstd]` or `rdb <file>`. Prints one `RESULT` line: phase times (ms), peak RSS, RSS after the bake and after `malloc_trim`. |
| `run_series.sh <dir> <log> <ifcview\|rdb> <cap> [threads zstd]` | Every IFC of a folder, one at a time, smallest first, each in a memory-capped systemd scope (a file over the cap is reported as killed instead of making the machine swap). |
| `run_parallel.sh <dir> <log> <ifcview\|rdb> <workers> [threads]` | Several files at once, biggest first, shared queue. No RAM check of its own: run it inside a capped scope. |
| `BonsaiViewer --mass-convert ifcview\|rdb <file.ifc>...` | The real scheduler without a window (same code as the menu entries). Outputs go next to each IFC and are overwritten. |

Build (Linux, see the project's build notes):

```bash
cmake --build BUILD --target bonsaiviewer_mass_convert_bench BonsaiViewer -j6
```

Examples:

```bash
run_series.sh ~/IFC/Big_Model /tmp/a.log ifcview 4G auto 9
systemd-run --user --scope -p MemoryMax=7G -p MemorySwapMax=0 -p OOMPolicy=continue \
    run_parallel.sh ~/IFC/Big_Model /tmp/p.log ifcview 2 4
# An 8 GB machine: about 5 GB left once the OS, a desktop and a browser run.
systemd-run --user --scope -p MemoryMax=5G -p MemorySwapMax=0 -p OOMPolicy=continue \
    BonsaiViewer --mass-convert ifcview ~/IFC/Big_Model/*.ifc
```

`IfcViewBuilder::timings()` splits a bake into: `first_mesh` (parse + geometry
setup until the first mesh, single-threaded), `stream` (all the geometry,
includes `first_mesh`), `finalize` (LOD + chunk layout) and `write` (zstd +
file).

## Test set and machine

- `Big_Model`: 32 IFC files, 1 to 466 MiB, 2.7 GB in total, 29 IFC2X3 and 3 IFC4.
- 12 logical cores, 14 GB RAM, Linux, ~10 GB free during the runs.
- Default geometry settings of the viewer (AppSettings), zstd level 9.

## Results

### .ifcview, whole folder

| Run | Wall time | Result |
|---|---|---|
| Before (in the viewer process, RAM nearly full) | ~15 min | 32/32 |
| One file at a time, threads from the free RAM (≤ 11) | 261 s | 31/32: BigModel15 killed at the 4 GB cap |
| 2 files at once × 4 threads (7 GB cap) | 163 s | 32/32 |
| 3 files at once × 4 threads (9 GB cap) | 150 s + 26 s retry | the 3 biggest together exceed 9 GB |
| One file at a time × 4 threads, 5 GB cap (8 GB machine) | 272 s | 32/32, peak 4.18 GB |
| **Scheduler, no cap** | **186 s** | 32/32, never under 4.8 GB free |
| **Scheduler, 5 GB cap (8 GB machine)** | **220 s** | 32/32, no retry, no OOM kill |

### .rdb, whole folder

| Run | Wall time | Result |
|---|---|---|
| One file at a time | 337 s | 32/32, peak ≤ 1.55 GB |
| 4 files at once | 113 s | 32/32 |
| 6 files at once | 108 s | plateau |
| **Scheduler, no cap** | **118 s** | 32/32 |
| **Scheduler, 5 GB cap (8 GB machine)** | **202 s** | 32/32 |

### More threads per file stop helping at 4

| File | 1 thread | 4 threads | 8 threads | 11 threads |
|---|---|---|---|---|
| BigModel28 (56 MiB) | 164 s, 1.25 GB | 82.7 s, 2.9 GB | 80.8 s, 2.9 GB | 81.5 s, 3.0 GB |
| BigModel29 (12 MiB) | 31.6 s, 1.0 GB | – | – | 33.1 s, 1.1 GB |
| BigModel15 (466 MiB) | 45.2 s, 4.13 GB | – | – | – (2 threads: 33.5 s, 4.18 GB) |

A bake's time goes to the **single-threaded parse and geometry setup**
(`first_mesh`: 91 s of the 261 s of the sequential run; up to two thirds of a
big file) and to **one or two very heavy elements** that keep a single thread
busy for tens of seconds (BigModel29: 30 s before its first mesh whatever the
threads). Extra cores are better spent on other files.

### RAM is not predictable from the file size

Peak RSS of one .ifcview bake (one file at a time, threads ≤ 11):

| File | IFC (MiB) | Threads | first_mesh (s) | Total (s) | Peak (MiB) | RSS after malloc_trim (MiB) |
|---|---|---|---|---|---|---|
| BigModel29 | 12 | 11 | 32.8 | 33.1 | 1106 | 89 |
| BigModel5 | 20 | 11 | 0.6 | 3.0 | 514 | 302 |
| BigModel28 | 56 | 11 | 1.7 | 81.5 | 3043 | 463 |
| BigModel8 | 75 | 11 | 2.1 | 10.8 | 1300 | 104 |
| BigModel24 | 91 | 11 | 3.0 | 6.0 | 1561 | 382 |
| BigModel22 | 217 | 9 | 4.6 | 9.9 | 1946 | 160 |
| BigModel13 | 385 | 6 | 9.6 | 19.0 | 3870 | 248 |
| BigModel1 | 400 | 6 | 10.7 | 27.8 | 2817 | 174 |
| BigModel14 | 452 | 5 | 6.7 | 10.2 | 2233 | 74 |
| BigModel15 | 466 | 2 | 14.4 | 33.5 | 4177 | 96 |

From 5× to 90× the file size. For a big file the parsed model dominates (1 and 2
threads cost the same for BigModel15); for some mid-size files the threads do
(BigModel28: 1.25 GB on 1 thread, 2.9 GB on 4).

.rdb peaks stay small: 1.55 GB for BigModel14 (452 MiB), under 1 GB otherwise.

### A converting process keeps the freed RAM

After a bake, glibc keeps the freed heap: BigModel13 left the process at 3.3 GB
until `malloc_trim` brought it to 0.25 GB. The viewer, after the old in-process
mass conversion, sat idle at 5.6 GB, and the next bakes ran out of RAM (the
~8 min of BigModel8 in the "before" run, against 11 s here).

## What the scheduler does (and why)

`MassConversionRunner` (`modules/models/MassConversion.cpp`), budget numbers in
`modules/models/BakeBudget.cpp`:

- **One worker process per file**: BonsaiViewer re-launched headless with
  `--mass-convert-worker`. All its RAM comes back when it exits, and running
  out of RAM kills only that worker, not the viewer.
- **Biggest files first**, so the long ones do not end the run alone.
- **.ifcview: 4 threads per file** (cores − 1 below 5 cores), **cores / 4 files
  at once**. **.rdb: up to 4 files at once.**
- **One more file starts only if the free RAM covers** what the running workers
  may still take (estimate − current RSS), the next file's estimate and 1 GB of
  reserve. Estimates are pessimistic: .ifcview 1 GB + 9 × IFC size, .rdb
  256 MB + 4 × IFC size. When the free RAM is unknown, one file at a time.
- The free RAM read on Linux is **MemAvailable, capped by the cgroup v2 limit**
  (systemd scope, container, flatpak); Windows `GlobalMemoryStatusEx`; macOS
  free + inactive + purgeable pages.
- **Watchdog every 250 ms**: under 384 MB free, the latest started worker is
  stopped and requeued to run alone (if it was alone and multi-threaded: alone
  on one thread). A worker that crashes or is killed by the system is retried
  once, alone, on one thread. A file that cannot be converted (worker exit
  code 3) is not retried.
- **Workers run at low CPU priority** (nice 10 / below-normal priority class),
  so the PC stays usable.
- **.ifcview written at zstd level 9** instead of the viewer's 19: far faster
  for a slightly bigger file; decode speed does not depend on the level.

## Limits and open points

- Measured on one Linux machine. The Windows and macOS code paths
  (`GlobalMemoryStatusEx`, `K32GetProcessMemoryInfo`, mach statistics) are
  written but were not compiled or run.
- On macOS the RSS of a worker is not read: the scheduler then counts the full
  estimate of each running worker (fewer files at once).
- The estimates are on the safe side: with no cap the scheduler (186 s) is
  slower than the best hand-tuned run (163 s), because big files mostly run one
  at a time.
- Files whose IFC alone needs more RAM than the machine has still fail; the
  biggest file here (466 MiB) needs ~4.2 GB.
- `first_mesh` is single-threaded and is the main cost of big files; making the
  parse or the iterator setup parallel would help every machine.
