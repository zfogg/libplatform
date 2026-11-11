# libplatform

libplatform is a small, opinionated foundation for C and C++ programs that
need to run on the major operating systems without turning every useful bit of
code into a nest of `#ifdef`s. It gives an application one set of names for
threads, locks, files, sockets, terminals, processes, signals, fonts, paths,
time, and the other pieces that are annoyingly different on Apple, Microsoft,
Linux, iOS, and WebAssembly.

The point is not to hide the operating system. The point is to put the awkward
differences in one well-tested place so the application can spend its time on
what it is actually trying to do.

## Start here

The public headers are the real reference. The API page below is generated
directly from every header in `include/libplatform`, including the platform
specific and debugging headers. Comments live beside the declarations, so the
documentation and the code do not quietly drift apart.

```{toctree}
:maxdepth: 2

api
```

## What it gives you

There is a common layer for the things every native application eventually
needs:

- `thread.h`, `mutex.h`, `rwlock.h`, and `cond.h` provide portable thread and
  synchronization primitives.
- `atomic.h` provides the small atomic operations that need to be shared by
  code which cannot depend on a platform-specific spelling.
- `socket.h`, `pipe.h`, `wakeup.h`, and `clipboard.h` cover communication and
  desktop integration boundaries.
- `filesystem.h`, `mmap.h`, `path.h`, `process.h`, `system.h`, and `systemd.h`
  keep files, processes, installation paths, and service integration in one
  place.
- `terminal.h`, `keyboard.h`, `font.h`, and `question.h` provide the terminal
  and console behavior that tends to vary most visibly between platforms.
- `time.h`, `cpu.h`, `compiler.h`, `windows_compat.h`, and the utility headers
  make the boring portability details predictable.
- `named.h` gives synchronization objects stable names when an application or
  its debugger needs to identify them.

The implementation is selected by the platform at build time. The public API
stays the same whether the implementation underneath is POSIX, Windows,
Apple, or the WebAssembly target.

## Debugging that can live in a library

The debug headers are designed to be useful to a library, not only to one
particular executable. Applications provide callbacks through
`app_callbacks.h`; libplatform does not need to know how an application's
logger, allocator report, or terminal UI is wired.

The synchronization debugger can track mutexes, read/write locks, condition
variables, and the atomic activity around them. In debug builds it can report
the number of lock, unlock, wait, signal, and broadcast actions, along with
time spent waiting. That turns “it is stuck somewhere” into a report that says
which primitive is hot and which thread is waiting on it.

The memory tools track allocations and frees for the parts of an application
that opt into them. The report includes outstanding allocations and their
captured call sites. An application can keep application-specific, colored
memory-report presentation in its own code while still using libplatform's
portable tracking machinery.

Backtrace support captures and prints call stacks on the supported operating
systems. Symbolization uses the host platform's facilities and may need a
symbolizer such as LLVM's tools on `PATH`. The callbacks make it possible for
the host application to decide whether a backtrace goes to a logger, a test
report, or a diagnostic screen.

## A normal way to use it

Link libplatform, include the public headers with the `libplatform/...` path,
and let CMake select the implementation for the target. Keep application
policy in the application: choose its install prefix, configure its logging
callbacks, decide whether colors are enabled, and decide which debug reports
are useful. Keep portability mechanics in libplatform.

That division is the reason this exists. There are plenty of projects that
can make one operating system pleasant. There are very few places where the
cross-platform C details, terminal behavior, debugging primitives, and
library-safe callbacks are gathered into one coherent starting point. Without
something like this, each application grows its own collection of subtly
different portability wrappers and spends years maintaining them.

libplatform is meant to be the place you start instead.

## Header map

Every installed header is listed here explicitly:

- `agent.h` — discovers SSH and GPG agent endpoints.
- `app_callbacks.h` — defines application callbacks for library diagnostics.
- `atomic.h` — provides portable atomic values, loads, stores, and exchanges.
- `backtrace.h` — captures, symbolizes, and prints platform backtraces.
- `clipboard.h` — reads clipboard text through an application formatter.
- `compiler.h` — supplies compiler and language compatibility definitions.
- `cond.h` — provides condition variables, waits, signals, and broadcasts.
- `cpu.h` — detects SSE2, SSSE3, AVX2, NEON, SVE, and CRC32 features.
- `errno.h` — exposes the platform error boundary.
- `filesystem.h` — provides files, directories, paths, permissions, and data locations.
- `font.h` — provides the embedded-font registry and lookup operations.
- `keyboard.h` — provides nonblocking keyboard input, editing, and text decoding.
- `lifecycle.h` — coordinates one-time initialization and shutdown.
- `memory.h` — defines the portable memory allocation seam.
- `mmap.h` — maps and unmaps files and memory regions.
- `mutex.h` — provides mutual exclusion and try-lock operations.
- `named.h` — gives synchronization and application objects stable names.
- `pipe.h` — provides portable anonymous pipe operations.
- `process.h` — manages process titles and subprocess execution.
- `question.h` — asks interactive questions and reads answers.
- `rwlock.h` — provides reader/writer locks and their timing operations.
- `signal.h` — installs and manages portable signal handlers.
- `socket.h` — creates, configures, connects, polls, and closes sockets.
- `symbols.h` — resolves addresses and formats symbol names.
- `system.h` — provides environment, descriptor, clock, sleep, and startup APIs.
- `systemd.h` — sends service readiness and watchdog notifications on systemd hosts.
- `terminal.h` — detects terminal capabilities, colors, dimensions, and control sequences.
- `thread.h` — creates, joins, detaches, and identifies platform threads.
- `time.h` — provides monotonic time, deadlines, and sleep helpers.
- `uthash.h` — vendors the hash-table dependency used by internal registries.
- `wakeup.h` — wakes worker threads portably.
- `windows_compat.h` — supplies compatibility definitions for Windows builds.
- `posix/system.h` — exposes the POSIX system adapter boundary.
- `windows/getopt.h` — supplies getopt and getopt-long compatibility on Windows.
- `windows/system.h` — exposes Windows-specific system adapter details.

The debug headers are also individually public:

- `debug/atomic.h` — registers and reports atomic-operation statistics.
- `debug/backtrace.h` — captures, filters, formats, and prints diagnostic traces.
- `debug/coverage.h` — exposes coverage instrumentation hooks.
- `debug/debug_helpers.h` — shares initialization and diagnostic helper routines.
- `debug/errno.h` — reports debug error and errno state.
- `debug/helpers.h` — provides common debug formatting and callback helpers.
- `debug/memory.h` — tracks allocations, frees, peaks, and live blocks.
- `debug/mutex.h` — tracks mutex ownership, waits, and deadlock state.
- `debug/named.h` — reports the named-object registry and metadata.
- `debug/stats.h` — schedules and triggers debug report threads.
- `debug/sync.h` — reports mutex, rwlock, condition-variable, and atomic state.
