POSIX and Apple API
===================

The POSIX implementation is shared by Linux and Apple targets where their
system facilities have the same contract. Apple-specific source files remain
behind the same public boundary.

``posix/system.h`` is a small compatibility header rather than a function
API. It supplies the POSIX constants and shell behavior needed by the common
system implementation:

* ``STDIN_FILENO`` is the standard input descriptor, guaranteed to be ``0``
  when the platform headers have not defined it.
* ``STDOUT_FILENO`` is the standard output descriptor, guaranteed to be ``1``
  when the platform headers have not defined it.
* ``STDERR_FILENO`` is the standard error descriptor, guaranteed to be ``2``
  when the platform headers have not defined it.
* ``PLATFORM_SHELL_NULL_REDIRECT`` is the POSIX shell fragment used to redirect
  unwanted standard error output to ``/dev/null``. Applications should use the
  higher-level libplatform process/system APIs instead of building shell
  commands around this macro.

The header includes ``unistd.h`` and ``limits.h`` for the POSIX implementation
boundary. The portable functions that consume this boundary are documented on
the :doc:`api-common` page, especially in ``system.h``, ``process.h``,
``filesystem.h``, and ``socket.h``.

The header is also autodocumented below so future POSIX declarations added to
it appear here automatically:

.. c:autodoc:: libplatform/posix/system.h
