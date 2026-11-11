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

### Common headers

- `agent.h` — discovers SSH and GPG agent endpoints.
- `app_callbacks.h` — defines callbacks for library-owned diagnostics.
- `atomic.h` — provides portable atomic values and operations.
- `backtrace.h` — captures and prints platform backtraces.
- `clipboard.h` — reads clipboard text through an application formatter.
- `compiler.h` — provides compiler and language compatibility definitions.
- `cond.h` — provides condition variables, waits, signals, and broadcasts.
- `cpu.h` — detects SSE2, SSSE3, AVX2, NEON, SVE, and CRC32 features.
- `errno.h` — exposes the platform error boundary.
- `filesystem.h` — provides files, directories, permissions, and data paths.
- `font.h` — provides the embedded-font registry and lookup operations.
- `keyboard.h` — provides keyboard input, editing, and text decoding.
- `lifecycle.h` — coordinates one-time initialization and shutdown.
- `memory.h` — defines the portable allocation seam.
- `mmap.h` — maps and unmaps files and memory regions.
- `mutex.h` — provides mutual exclusion and try-lock operations.
- `named.h` — gives long-lived objects stable names.
- `pipe.h` — provides portable anonymous pipe operations.
- `process.h` — manages process titles and subprocess execution.
- `question.h` — asks interactive questions and reads answers.
- `rwlock.h` — provides reader/writer locks and timing operations.
- `signal.h` — installs and manages portable signal handlers.
- `socket.h` — creates, configures, connects, polls, and closes sockets.
- `symbols.h` — resolves addresses and formats symbol names.
- `system.h` — provides environment, descriptors, clocks, sleeps, and startup.
- `systemd.h` — sends service readiness and watchdog notifications.
- `terminal.h` — detects terminal capabilities and controls terminal output.
- `thread.h` — creates, joins, detaches, and identifies platform threads.
- `time.h` — provides monotonic time, deadlines, and sleep helpers.
- `uthash.h` — vendors the hash-table dependency used by registries.
- `wakeup.h` — wakes worker threads portably.
- `windows_compat.h` — supplies Windows compatibility definitions.

### Platform-specific headers

- `posix/system.h` — exposes the POSIX system adapter boundary.
- `windows/getopt.h` — provides getopt and getopt-long compatibility on Windows.
- `windows/system.h` — exposes Windows-specific system adapter details.

### Debug headers

- `debug/atomic.h` — registers and reports atomic-operation statistics.
- `debug/backtrace.h` — captures, filters, formats, and prints diagnostic traces.
- `debug/coverage.h` — exposes coverage instrumentation hooks.
- `debug/debug_helpers.h` — provides shared initialization and debug helpers.
- `debug/errno.h` — reports debug error and errno state.
- `debug/helpers.h` — provides common debug formatting and callback helpers.
- `debug/memory.h` — tracks allocations, frees, peaks, and live blocks.
- `debug/mutex.h` — tracks mutex ownership, waits, and deadlock state.
- `debug/named.h` — reports the named-object registry and metadata.
- `debug/stats.h` — schedules and triggers debug report threads.
- `debug/sync.h` — reports mutex, rwlock, condition-variable, and atomic state.

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
