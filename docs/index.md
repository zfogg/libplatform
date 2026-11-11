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

The installed headers are grouped by responsibility rather than by operating
system, so application code can use one stable API:

- `agent.h` finds SSH/GPG agents; `app_callbacks.h` defines callbacks for
  library-owned diagnostics; `compiler.h` handles compiler compatibility; and
  `cpu.h` reports CPU features.
- `atomic.h` provides atomic values and operations; `mutex.h`, `rwlock.h`,
  `cond.h`, and `thread.h` provide synchronization and threads; `named.h`
  names long-lived objects; and `lifecycle.h` controls initialization and
  shutdown.
- `backtrace.h` captures and prints platform backtraces; `symbols.h` resolves
  addresses; `memory.h` provides the allocation seam; and `errno.h` defines
  the platform error boundary.
- `debug/atomic.h` tracks atomic operations; `debug/backtrace.h` provides the
  diagnostic backtrace workflow; `debug/coverage.h` exposes coverage hooks;
  `debug/debug_helpers.h`, `debug/helpers.h`, `debug/errno.h`, and
  `debug/stats.h` provide diagnostic support; and `debug/memory.h`,
  `debug/mutex.h`, `debug/named.h`, and `debug/sync.h` provide memory, lock,
  registry, and synchronization reports.
- `filesystem.h` handles files and directories; `mmap.h` handles mapped files;
  `pipe.h` and `wakeup.h` handle worker communication; and `process.h` handles
  process titles and subprocesses.
- `socket.h` abstracts sockets; `signal.h` handles signals; `system.h` and
  `systemd.h` cover system services; `time.h` covers clocks and deadlines; and
  `windows_compat.h` contains Windows compatibility definitions.
- `terminal.h` detects and controls terminals; `keyboard.h` handles keyboard
  input and line editing; `question.h` handles prompts; `clipboard.h` reads
  the clipboard; and `font.h` provides the embedded-font registry.
- `windows/getopt.h` provides Windows getopt compatibility;
  `windows/system.h` provides Windows system details; `posix/system.h` is the
  POSIX system boundary; and `uthash.h` is the vendored hash dependency.

The utility headers (`format.h`, `ip.h`, `path.h`, `string.h`, and `utf8.h`)
are private implementation headers under `lib/util/`; they are not installed
for consumers.
