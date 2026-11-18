# libplatform release package

Extract this archive anywhere; it has no required installation directory. The
`static/` and `shared/` trees contain the corresponding library, headers, CMake
package, pkg-config file, and license.

For CMake, set `CMAKE_PREFIX_PATH` to the extracted tree and use
`find_package(libplatform CONFIG REQUIRED)`, then link
`libplatform::libplatform`. For pkg-config, set `PKG_CONFIG_PATH` to the
extracted `lib/pkgconfig` directory and request `libplatform`.

Static consumers must link the system libraries required by their platform;
shared consumers must make the extracted library directory available to the
platform loader.
