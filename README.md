# libplatform

[Read the documentation](https://zfogg.github.io/libplatform/)

libplatform is the part of a C or C++ application that you usually end up
writing after you have already spent too much time writing it badly.

You start with a nice little program. Then it needs to run on Linux, macOS,
Windows, maybe iOS, maybe in a browser through WebAssembly. Suddenly every
file has an `#ifdef`, every thread function has a different name, sockets have
different types, terminals behave differently, and the debugging code becomes
its own second application. You can keep maintaining that pile of platform
exceptions yourself, or you can put the platform differences here and let the
application use one set of C APIs.

That is what libplatform is for.

It is a cross-platform foundation for C and C++ applications. It currently
supports Windows, Linux, Apple platforms including iOS, and WebAssembly. The
library is mostly concerned with the unglamorous but important differences
between Microsoft, Apple, and Linux systems: threads, locks, sockets, files,
terminals, processes, signals, timing, memory mapping, fonts, keyboard input,
backtraces, symbolization, and the small things that make a program feel like
it belongs on the operating system where it is running.

There is nothing particularly clever about maintaining a hundred little
platform branches by hand. It is just a lot of chores, and the chores are easy
to get subtly wrong. libplatform puts those chores in one place and gives the
rest of the application a stable interface.

## Getting started

libplatform uses CMake and is tested with Clang and Ninja. It needs utf8proc for
its Unicode helpers. You can provide an existing CMake target, or initialize
the vendored `deps/utf8proc` dependency.

```sh
cmake -S . -B build -G Ninja \
  -DLIBPLATFORM_BUILD_TESTS=ON \
  -DLIBPLATFORM_BUILD_SHARED=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

If the compiler is not already selected by a toolchain, the standalone project
defaults to Clang. An application embedding libplatform can pass its own
compiler, dependency target, install paths, and platform-specific CMake
options in the normal way.

The public headers live under `include/libplatform/`. The implementation is
split into common code and platform directories. An application normally links
`libplatform::libplatform` and includes the headers it needs.

## The shape of the library

The code is organized around one common API with small operating-system
implementations behind it:

- `lib/` contains the common implementations and the platform-independent
  parts of the public APIs.
- `lib/windows/` contains Windows implementations using Win32, Winsock,
  console, process, debugging, and synchronization facilities.
- `lib/posix/` contains the shared POSIX implementations used by Linux and
  Apple systems.
- `lib/linux/` contains Linux-specific behavior such as systemd integration
  and Linux font/system details.
- `lib/macos/` contains macOS-specific system and font behavior.
- `lib/wasm/` contains WebAssembly implementations and stubs for functionality
  that does not exist in a browser.
- `include/libplatform/` is the public interface. The application should use
  this layer instead of reaching into one of the operating-system directories.

## Header map

The installed header tree is intentionally explicit. Every public header has
one job:

- `agent.h` finds SSH/GPG agents; `app_callbacks.h` defines the callback seam
  for library-safe diagnostics; `compiler.h` carries compiler compatibility;
  and `cpu.h` reports CPU features.
- `atomic.h` provides portable atomic values and operations; `mutex.h`,
  `rwlock.h`, `cond.h`, and `thread.h` provide synchronization and threads;
  `named.h` gives long-lived objects stable names; and `lifecycle.h` handles
  one-time initialization and shutdown.
- `backtrace.h` captures and prints platform backtraces; `symbols.h` resolves
  addresses and symbol names; `memory.h` is the allocation seam; and `errno.h`
  provides the platform error boundary.
- `debug/atomic.h` tracks atomic operations; `debug/backtrace.h` owns the
  diagnostic backtrace workflow; `debug/coverage.h` exposes coverage hooks;
  `debug/debug_helpers.h`, `debug/helpers.h`, `debug/errno.h`, and
  `debug/stats.h` provide shared diagnostic state; and `debug/memory.h`,
  `debug/mutex.h`, `debug/named.h`, and `debug/sync.h` provide allocation,
  lock, registry, and synchronization reports.
- `filesystem.h` handles files, directories, paths, and installation data;
  `mmap.h` handles mapped files; `pipe.h` and `wakeup.h` handle worker
  communication; and `process.h` handles process titles and subprocesses.
- `socket.h` abstracts network sockets; `signal.h` handles signals;
  `system.h` and `systemd.h` cover operating-system services; `time.h` covers
  clocks and deadlines; and `windows_compat.h` holds Windows compatibility
  definitions.
- `terminal.h` detects terminal capabilities and controls terminal behavior;
  `keyboard.h` handles keyboard input and line editing; `question.h` handles
  interactive prompts; `clipboard.h` handles clipboard reads; and `font.h`
  provides the embedded-font registry.
- `windows/getopt.h` supplies Windows getopt compatibility and
  `windows/system.h` supplies Windows system details. `posix/system.h` is the
  POSIX system boundary. `uthash.h` is the vendored hash dependency used by
  the library's hash-backed registries.

The utility headers (`format.h`, `ip.h`, `path.h`, `string.h`, and `utf8.h`)
live under `lib/util/` and are private implementation headers. They are not
installed as consumer-facing API; applications should use their own utility
headers and the public libplatform APIs above.

For completeness, the installed common headers are `agent.h`,
`app_callbacks.h`, `atomic.h`, `backtrace.h`, `clipboard.h`, `compiler.h`,
`cond.h`, `cpu.h`, `errno.h`, `filesystem.h`, `font.h`, `keyboard.h`,
`lifecycle.h`, `memory.h`, `mmap.h`, `mutex.h`, `named.h`, `pipe.h`,
`process.h`, `question.h`, `rwlock.h`, `signal.h`, `socket.h`, `symbols.h`,
`system.h`, `systemd.h`, `terminal.h`, `thread.h`, `time.h`, `uthash.h`,
`wakeup.h`, and `windows_compat.h`. The platform-specific public headers are
`posix/system.h`, `windows/getopt.h`, and `windows/system.h`. The public debug
headers are `debug/atomic.h`, `debug/backtrace.h`, `debug/coverage.h`,
`debug/debug_helpers.h`, `debug/errno.h`, `debug/helpers.h`, `debug/memory.h`,
`debug/mutex.h`, `debug/named.h`, `debug/stats.h`, and `debug/sync.h`.

The common files own the contracts and the platform files provide the actual
behavior. That makes the differences visible in the implementation without
making every caller know about them.

## Threads, locks, and atomics

The synchronization APIs are deliberately ordinary C APIs. You initialize a
primitive, use it, and destroy it. The implementation takes care of whether
the underlying object is a Win32 critical section, a pthread primitive, or a
WebAssembly-compatible stub.

The main synchronization types are:

- `mutex_t` in `mutex.h` for mutual exclusion.
- `rwlock_t` in `rwlock.h` for reader/writer locking.
- `cond_t` in `cond.h` for waiting and signaling.
- `platform_thread_t` in `thread.h` for creating, joining, detaching, and
  timing out threads.
- `platform_wakeup_t` in `wakeup.h` for waking a worker without making the
  worker know which operating-system primitive is underneath it.
- `lifecycle_t` in `lifecycle.h` for safe module initialization, shutdown,
  reset, and permanent destruction.

The thread API includes `platform_thread_create()`, `thread_join()`,
`thread_detach()`, `thread_join_timeout()`, `thread_self()`, thread
identity helpers, and thread-local storage through `platform_tls_*`. The
lifecycle API handles the common
`UNINITIALIZED -> INITIALIZING -> INITIALIZED` race and has matching
commit/abort and shutdown/destroy operations. That is useful for libraries
because initialization should not quietly happen twice just because two
application threads reached it at the same time.

### Atomic operations

`atomic.h` provides the actual atomic building blocks used by the library and
available to applications:

- boolean load, store, compare-and-swap, and exchange;
- integer load, store, fetch-add, fetch-sub, compare-and-swap, and exchange;
- `uint64_t` equivalents;
- atomic pointer load, store, compare-and-swap, and exchange.

The types are `atomic_t` and `atomic_ptr_t`. The API keeps the portable
operation names stable while the implementation can use the right compiler or
operating-system primitive underneath.

## Synchronization statistics and debug threads

The synchronization code is not just a collection of locks that becomes
impossible to understand when something hangs. In debug builds, named
primitives can carry timing and ownership information. The library tracks the
operations that matter when debugging contention:

- acquisitions and releases;
- currently-held primitives;
- last lock and unlock times;
- wait and signal activity for condition variables;
- read and write activity for reader/writer locks;
- operation counts and time spent waiting where the primitive exposes it.

The `debug/named.h` registry gives a mutex, rwlock, condition variable, atomic,
or other registered object a human-readable name and type. The
`debug/sync.h` API can then print mutex state, rwlock state, condition-variable
state, or the complete synchronization state in one report. It also has
deadlock checks and delayed state/backtrace reports.

For a long-running application, the debug facilities can run on their own
threads instead of making the signal or callback that requested a report do
the expensive work itself:

```c
#include <libplatform/debug/stats.h>
#include <libplatform/debug/sync.h>

debug_stats_init();
debug_sync_init();
debug_stats_start_thread();
debug_sync_start_thread(0, 0);

/* Request reports from a signal handler, UI action, or application callback. */
debug_stats_trigger_print();
debug_sync_trigger_print();

debug_sync_destroy();
debug_stats_destroy();
```

The exact startup and shutdown order belongs to the application, but the
important point is that the library gives you a real reporting path that does
not require a random `printf()` in a lock or a signal handler.

## Debugging memory

`debug/memory.h` provides tracked `malloc`, `calloc`, `realloc`, and
`free` wrappers when `DEBUG_MEMORY` is enabled. Each allocation can record
its source file and line. The tracker maintains total allocated bytes, total
freed bytes, current usage, peak usage, allocation/free call counts, and the
list of live allocations.

At shutdown, `debug_memory_report(bool colors)` prints a useful report instead
of leaving you to guess whether a process leaked. The report can show the
allocation site for outstanding blocks and is protected against the recursion
that normally makes memory debugging tools fall over while they are trying to
report a memory debugging problem.

When tracking is disabled, the same public allocation seam can be compiled as
a lightweight passthrough. Applications can therefore keep the allocation
calls in place and change the build mode instead of maintaining two versions of
every cleanup path.

## Backtraces and symbolization

There are two layers because applications need two different things.

The public `backtrace.h` API provides the platform capture and symbol helpers:
`platform_backtrace_capture()`, `platform_backtrace_symbols()`, and
`platform_backtrace_print()`. The debug API in `debug/backtrace.h` provides
the larger diagnostic workflow: capture a `backtrace_t`, symbolize it, format
it, print one or many traces, and release it with `backtrace_t_free()`.

The implementation has platform-specific capture code and a common symbol
formatting/cache layer. On Windows it uses the Windows debugging APIs; on
POSIX systems it uses the native backtrace facilities available on that
platform. Symbolization can use `llvm-symbolizer` or `addr2line` when one
is available on `PATH`. If neither is available, the library still returns
the addresses and whatever symbol information the platform can provide.

Applications do not have to invent their own output stream. Register callbacks
with `app_callbacks.h` and libplatform sends synchronization reports, memory
reports, and backtraces back to the application:

```c
static void write_debug_text(const char *text, void *context) {
  /* Route text to the app's logger, TUI, file, or test collector. */
}

platform_app_callbacks_t callbacks = {
    .write_sync_state = write_debug_text,
    .write_memory_report = write_debug_text,
    .write_backtrace = write_debug_text,
    .debug_context = NULL,
};
platform_app_callbacks_register(&callbacks);
```

That callback boundary is important for a library. libplatform owns the
diagnostic machinery, but the application owns where diagnostic text belongs.

## Terminal and rendering support

Terminal behavior is one of the places where cross-platform code gets messy
fast. `terminal.h` keeps the policy and detection in one place.

The basic terminal value types include:

- `terminal_color_mode_t`: `TERM_COLOR_AUTO`, `TERM_COLOR_NONE`,
  `TERM_COLOR_16`, `TERM_COLOR_256`, and `TERM_COLOR_TRUECOLOR`.
- `terminal_render_mode_t`: `RENDER_MODE_FOREGROUND`,
  `RENDER_MODE_BACKGROUND`, and `RENDER_MODE_HALF_BLOCK`.
- `terminal_options_t`: color setting, color mode, UTF-8 preference, render
  mode, width, height, FPS, and snapshot mode.
- `terminal_capabilities_t`: detected color level, capability flags, color
  count, UTF-8 support, detection reliability, rendering preferences, palette
  state, desired FPS, padding, and the terminal's `TERM`/`COLORTERM` values.
- `terminal_size_t`: rows and columns.

The capability flags include 16-color, 256-color, truecolor, UTF-8,
background-color, and matrix-rendering support. The API can detect terminal
size and resize changes, determine whether input/output is a TTY or pipe,
enable ANSI behavior, set raw mode and echo, move the cursor, set titles,
save/restore cursor state, format hyperlinks, query background color, and
decide whether color and control sequences should actually be emitted.

The point is that the application can ask a simple question like
`terminal_should_color_output()` and get the right answer for a real
terminal, a redirected pipe, a Windows console, an SSH session, or a
browser-facing build without duplicating the detection rules everywhere.

## Files, processes, sockets, and system behavior

The rest of the library covers the pieces that every serious native program
eventually needs:

- `filesystem.h` provides stat/access, path normalization, directories,
  recursive mkdir, temporary files, file streams, read/write/close/unlink,
  executable and temp-directory discovery, application config/data locations,
  and directory iteration.
- The private `lib/util/` layer provides path, string, formatting, IP-address,
  and Unicode helpers used internally. Public callers reach formatting and
  allocation behavior through the platform APIs rather than including those
  implementation headers.
- `socket.h` provides socket initialization/cleanup, creation, bind/listen/
  accept/connect, send/receive, polling/select, socket options, blocking and
  timeout behavior, address helpers, error classification, and streaming
  optimizations.
- `process.h` covers process titles and subprocess execution.
- `system.h` covers clocks, sleeps, environment variables, safe memory/string
  operations, file descriptors, CA certificate loading, hostname resolution,
  standard-I/O redirection, timer precision, and platform startup/shutdown.
- `systemd.h` provides the small systemd notification surface when the host
  supports it.
- `mmap.h`, `pipe.h`, and `wakeup.h` provide portable mapped files, pipes,
  and worker wakeups.
- `question.h` provides cross-platform interactive questions and answers.
- `keyboard.h` provides nonblocking keyboard input, line editing, UTF-8/text
  decoding, terminal replies, and background-color queries.
- `signal.h`, `errno.h`, and `windows_compat.h` smooth over operating-system
  differences that otherwise leak into every caller.
- `font.h` gives applications a small embedded-font registry and lookup API.
- `clipboard.h` provides clipboard reading with an application-provided path
  formatter.
- `agent.h` finds SSH and GPG agent sockets.
- `cpu.h` reports CPU features such as SSE2, SSSE3, AVX2, NEON, SVE, and CRC32
  support so an application can choose the right implementation.

## Named objects

`named.h` and `debug/named.h` provide a registry for objects that need to be
understood while the application is running. A lock named `network.send`, a
condition named `decoder.frames`, or a packet type registered with the named
registry can show up in diagnostics with its type, ownership, metadata, and
statistics instead of appearing as an anonymous address.

The registry supports registration, metadata snapshots, iteration, packet and
file-descriptor metadata, hash statistics, and cleanup. The synchronization
debugger uses it as the common source of truth, but applications can use it
for their own long-lived objects too.

## A small example

This is the shape of a normal module using libplatform. The application code
does not need to know whether the lock is backed by a critical section or a
pthread mutex, and it can still name the lock so the debug tools understand it.

```c
#include <libplatform/mutex.h>
#include <libplatform/thread.h>

static mutex_t state_lock;
static platform_thread_t worker;

static void *worker_main(void *arg) {
  (void)arg;
  mutex_lock_impl(&state_lock);
  /* update application state */
  mutex_unlock_impl(&state_lock);
  return NULL;
}

int start_worker(void) {
  if (mutex_init(&state_lock, "app.state") != 0) return -1;
  return platform_thread_create(&worker, "worker", worker_main, NULL);
}
```

In production you can keep the same code and let libplatform handle the
platform details. In development you can turn on the debug modules, register
the callbacks, start the report threads, and inspect the actual lock and
memory state when something goes wrong.

## Why use this instead of more `#ifdef`s?

Because the `#ifdef`s are not the interesting part of your application.

Your application should be deciding what it does. It should not be deciding
whether Windows calls a socket a `SOCKET`, whether a POSIX file descriptor is
an `int`, whether a terminal supports truecolor, whether a thread join needs a
different timeout mechanism, how to get a backtrace from a Windows DLL, or how
to print a useful lock report without deadlocking the report code itself.

libplatform gives you one place to solve those differences and one set of
names to use everywhere else. It also gives you debugging tools that most
cross-platform utility libraries do not: synchronization timing and ownership,
named-object inspection, tracked memory reports, callback-based debug output,
and backtrace capture plus symbolization across the supported platforms.

If you are starting a C or C++ application and you already know it needs to
run on more than one operating system, libplatform is a good place to start.
You can get the boring platform work out of the way early, keep the application
code readable, and add another operating system by extending the platform
implementation instead of teaching every application module a new collection
of exceptions.

## License

See the repository license for the terms covering libplatform and its bundled
components.
