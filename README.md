# libplatform

Small, dependency-free cross-platform primitives extracted from ascii-chat.

The backtrace and symbolization implementation is extracted from ascii-chat, including its address cache, batch `llvm-symbolizer`/`addr2line` resolution, source-path formatting, and WebAssembly-safe behavior. The library owns its fallback allocator, lifecycle, locking, and diagnostics seams; applications provide presentation through the output callback.

Build and test:

```sh
cmake -S . -B build -G Ninja -DLIBPLATFORM_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

An external `llvm-symbolizer` or `addr2line` binary is optional at runtime; unresolved frames fall back to addresses.
