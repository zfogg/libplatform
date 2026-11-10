# libplatform

Small, dependency-free cross-platform primitives extracted from ascii-chat.

The first slice provides native stack capture, best-effort symbolization, and an application-owned output callback. It has no logging, allocator, option parser, or ascii-chat dependency. WebAssembly exposes the same API with a safe no-op capture stub.

Build and test:

```sh
cmake -S . -B build -G Ninja -DLIBPLATFORM_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The library intentionally does not promise an external symbolizer binary. Applications may use the returned addresses/symbol strings with their own debug tooling.
