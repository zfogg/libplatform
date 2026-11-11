Common API
==========

These headers are the portable surface shared by native and browser builds.

Core lifecycle and platform services
------------------------------------

.. c:autodoc:: libplatform/agent.h libplatform/app_callbacks.h libplatform/compiler.h libplatform/cpu.h libplatform/errno.h libplatform/filesystem.h libplatform/lifecycle.h libplatform/memory.h libplatform/mmap.h libplatform/process.h libplatform/signal.h libplatform/symbols.h libplatform/system.h libplatform/systemd.h libplatform/time.h libplatform/windows_compat.h

Concurrency and communication
-----------------------------

.. c:autodoc:: libplatform/atomic.h libplatform/cond.h libplatform/mutex.h libplatform/named.h libplatform/pipe.h libplatform/rwlock.h libplatform/socket.h libplatform/thread.h libplatform/wakeup.h

Terminal and user-facing services
---------------------------------

.. c:autodoc:: libplatform/clipboard.h libplatform/font.h libplatform/keyboard.h libplatform/question.h libplatform/terminal.h

Internal utility layer
----------------------

The implementation utilities live under ``lib/util`` and are deliberately not
installed as public headers. They support libplatform's own implementation;
applications should use their own utility headers for application policy and
formatting. The implementation headers remain covered by the source comments
and the library's internal build.

Vendored dependency surface
---------------------------

``uthash.h`` is included for consumers that use the library's public hash
types. It is a vendored third-party header and is intentionally listed here as
a dependency surface rather than expanded into thousands of macro entries.
