cmake_minimum_required(VERSION 3.20)

# A parent project may already have configured libplatform from a shared source
# tree.  CMake targets are global, so a second FetchContent evaluation must be
# a no-op instead of attempting to recreate the library targets.
if(TARGET libplatform OR TARGET libplatform-objects OR TARGET libplatform::libplatform)
  return()
endif()

project(libplatform VERSION 0.1.0 LANGUAGES C)
include(${CMAKE_CURRENT_SOURCE_DIR}/deps/cmake-utils/CacheLauncher.cmake)
cmake_utils_configure_cache_launcher()

option(LIBPLATFORM_BUILD_TESTS "Build libplatform tests" ON)
set(LIBPLATFORM_UTF8PROC_TARGET "" CACHE STRING
  "Existing CMake target providing utf8proc; consumers should pass their shared target")
set(LIBPLATFORM_UTF8PROC_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/utf8proc" CACHE PATH
  "utf8proc source directory used when LIBPLATFORM_UTF8PROC_TARGET is not supplied")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT CMAKE_C_COMPILER MATCHES "musl"
   AND NOT CMAKE_C_COMPILER_TARGET MATCHES "musl")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBPLATFORM_FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
endif()

if(NOT LIBPLATFORM_UTF8PROC_TARGET)
  if(TARGET utf8proc::utf8proc)
    set(LIBPLATFORM_UTF8PROC_TARGET utf8proc::utf8proc)
  elseif(TARGET utf8proc)
    set(LIBPLATFORM_UTF8PROC_TARGET utf8proc)
  elseif(EXISTS "${LIBPLATFORM_UTF8PROC_SOURCE_DIR}/CMakeLists.txt")
    set(UTF8PROC_ENABLE_TESTING OFF CACHE BOOL "Disable utf8proc tests" FORCE)
    set(UTF8PROC_INSTALL OFF CACHE BOOL "Disable utf8proc installation" FORCE)
    add_subdirectory("${LIBPLATFORM_UTF8PROC_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/utf8proc" EXCLUDE_FROM_ALL)
    if(TARGET utf8proc::utf8proc)
      set(LIBPLATFORM_UTF8PROC_TARGET utf8proc::utf8proc)
    elseif(TARGET utf8proc)
      set(LIBPLATFORM_UTF8PROC_TARGET utf8proc)
    endif()
  elseif(EXISTS "${LIBPLATFORM_UTF8PROC_SOURCE_DIR}/utf8proc.c")
    add_library(libplatform-utf8proc STATIC
      "${LIBPLATFORM_UTF8PROC_SOURCE_DIR}/utf8proc.c")
    target_include_directories(libplatform-utf8proc PRIVATE "${LIBPLATFORM_UTF8PROC_SOURCE_DIR}")
    target_compile_definitions(libplatform-utf8proc PRIVATE UTF8PROC_STATIC=1)
    set(LIBPLATFORM_UTF8PROC_TARGET libplatform-utf8proc)
  endif()
endif()
if(NOT LIBPLATFORM_UTF8PROC_TARGET OR NOT TARGET "${LIBPLATFORM_UTF8PROC_TARGET}")
  message(FATAL_ERROR
    "libplatform requires utf8proc. Pass -DLIBPLATFORM_UTF8PROC_TARGET=<target> "
    "or initialize the libplatform deps/utf8proc submodule.")
endif()

add_library(libplatform-objects OBJECT)
set_target_properties(libplatform-objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(libplatform-objects PRIVATE /experimental:c11atomics)
endif()
if(BUILD_SHARED_LIBS)
  add_library(libplatform SHARED $<TARGET_OBJECTS:libplatform-objects>)
else()
  add_library(libplatform STATIC $<TARGET_OBJECTS:libplatform-objects>)
endif()
add_library(libplatform::libplatform ALIAS libplatform)
if(TARGET PkgConfig::LIBPLATFORM_FONTCONFIG)
  target_link_libraries(libplatform PUBLIC PkgConfig::LIBPLATFORM_FONTCONFIG)
endif()
target_compile_definitions(libplatform-objects PRIVATE LIBPLATFORM_STANDALONE=1)
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
  target_compile_definitions(libplatform-objects PRIVATE _GNU_SOURCE)
endif()
if(BUILD_SHARED_LIBS)
  target_compile_definitions(libplatform-objects PRIVATE BUILDING_LIBPLATFORM_DLL=1)
else()
  target_compile_definitions(libplatform-objects PRIVATE BUILDING_STATIC_LIB=1)
endif()
target_sources(libplatform-objects PRIVATE
  lib/symbols.c
  # Shared rwlock diagnostics are called by each platform-specific lock
  # implementation. Keep the hooks in the platform library so shared builds
  # do not leave unresolved debug symbols at link time.
  lib/rwlock.c
  lib/filesystem_common.c
  lib/application.c
  lib/app_callbacks.c
  lib/lifecycle.c
  lib/mutex.c
  lib/cond.c
  lib/rwlock.c
  lib/system_common.c
  lib/keyboard.c
  lib/terminal.c
  lib/socket.c
  lib/socket_io.c
  lib/question.c
  lib/frame_flush.c
  lib/process.c
  lib/time.c
  lib/atomic.c
  lib/named.c
  lib/cpu.c
  lib/font.c
)
target_sources(libplatform-objects PRIVATE
  lib/util/path.c
  lib/util/string.c
  lib/util/format.c
  lib/util/ip.c
)
if(PROJECT_IS_TOP_LEVEL)
  target_sources(libplatform-objects PRIVATE lib/util/utf8.c)
endif()
target_sources(libplatform-objects PRIVATE
  lib/debug/atomic.c
  lib/debug/memory.c
  lib/debug/sync.c
  lib/debug/backtrace.c
  lib/debug/helpers.c
  lib/debug/mutex.c
)
if(WIN32)
  target_sources(libplatform-objects PRIVATE
    lib/keyboard.c
    lib/windows/thread.c
    lib/windows/backtrace.c
    lib/backtrace.c
    lib/windows/string.c
    lib/windows/filesystem.c
    lib/windows/terminal.c
    lib/windows/agent.c
    lib/windows/socket.c
    lib/windows/process.c
    lib/windows/rwlock.c
    lib/windows/mutex.c
    lib/windows/cond.c
    lib/windows/system.c
    lib/windows/wakeup.c
    lib/windows/pipe.c
    lib/windows/keyboard.c
    lib/windows/errno.c
    lib/windows/getopt.c
    lib/windows/memory.c
    lib/windows/mmap.c
    lib/windows/keepawake.c
    lib/windows/signal.c
    lib/windows/clipboard.c
    lib/windows/font.c
    lib/windows/question.c
    lib/windows/frame_flush.c
  )
elseif(APPLE)
  target_sources(libplatform-objects PRIVATE
    lib/posix/thread.c
    lib/posix/string.c
    lib/posix/system.c
    lib/posix/filesystem.c
    lib/posix/clipboard.c
    lib/posix/keepawake.c
    lib/posix/terminal.c
    lib/posix/agent.c
    lib/posix/socket.c
    lib/posix/process.c
    lib/posix/rwlock.c
    lib/posix/mutex.c
    lib/posix/cond.c
    lib/posix/errno.c
    lib/posix/memory.c
    lib/posix/mmap.c
    lib/posix/pipe.c
    lib/posix/keyboard.c
    lib/posix/question.c
    lib/posix/backtrace.c
    lib/posix/frame_flush.c
    lib/posix/wakeup.c
    lib/macos/system.c
    lib/macos/font.c
  )
elseif(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
  target_sources(libplatform-objects PRIVATE
    lib/keyboard.c
    lib/posix/thread.c
    lib/posix/string.c
    lib/posix/filesystem.c
    lib/posix/terminal.c
    lib/posix/agent.c
    lib/posix/socket.c
    lib/posix/process.c
    lib/posix/rwlock.c
    lib/posix/mutex.c
    lib/posix/cond.c
    lib/posix/errno.c
    lib/posix/memory.c
    lib/posix/mmap.c
    lib/posix/system.c
    lib/posix/pipe.c
    lib/posix/keyboard.c
    lib/posix/question.c
    lib/posix/backtrace.c
    lib/posix/frame_flush.c
    lib/posix/wakeup.c
    lib/linux/system.c
    lib/linux/systemd.c
    lib/linux/font.c
    lib/linux/clipboard.c
  )
elseif(EMSCRIPTEN)
  # The browser replacements below intentionally supersede generic helpers
  # that are added by the common source list above.  Keeping both object files
  # in this static archive creates duplicate symbols (and, more importantly,
  # lets terminal/system code accidentally bypass the browser implementation).
  get_target_property(_libplatform_common_sources libplatform-objects SOURCES)
  foreach(_wasm_replaced_source
    lib/filesystem_common.c
    lib/keyboard.c
    lib/lifecycle.c
    lib/system_common.c
    lib/terminal.c
    lib/backtrace.c
    lib/wasm/stubs/audio.c
    lib/wasm/stubs/backtrace.c
    lib/wasm/stubs/comprehensive.c
    lib/wasm/stubs/misc.c)
    list(REMOVE_ITEM _libplatform_common_sources
      ${_wasm_replaced_source}
      ${CMAKE_CURRENT_SOURCE_DIR}/${_wasm_replaced_source})
  endforeach()
  set_property(TARGET libplatform-objects PROPERTY SOURCES ${_libplatform_common_sources})
  unset(_libplatform_common_sources)
  unset(_wasm_replaced_source)
  target_sources(libplatform-objects PRIVATE
    lib/symbols.c
    lib/wasm/backtrace.c
    lib/wasm/filesystem.c
    lib/wasm/init.c
    lib/wasm/lifecycle.c
    lib/wasm/string.c
    lib/wasm/system.c
    lib/wasm/terminal.c
    lib/wasm/thread.c
    lib/wasm/time.c
    lib/wasm/stubs/actions.c
    lib/wasm/stubs/crypto.c
    lib/wasm/stubs/log.c
    lib/wasm/stubs/manpage.c
    lib/wasm/stubs/mode_defaults.c
    lib/wasm/stubs/network.c
    lib/wasm/stubs/stats.c
    lib/wasm/stubs/util.c
    lib/wasm/stubs/video.c
    lib/wasm/stubs/ffmpeg_decoder.c
  )
endif()
target_include_directories(libplatform-objects PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(libplatform-objects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_include_directories(libplatform-objects PRIVATE ${LIBPLATFORM_UTF8PROC_SOURCE_DIR})
target_include_directories(libplatform PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(libplatform-objects PRIVATE "${LIBPLATFORM_UTF8PROC_TARGET}")
target_link_libraries(libplatform PRIVATE "${LIBPLATFORM_UTF8PROC_TARGET}")
target_compile_features(libplatform-objects PUBLIC c_std_11)
target_compile_features(libplatform PUBLIC c_std_11)
target_compile_definitions(libplatform-objects PRIVATE UTF8PROC_STATIC=1)
if(CMAKE_C_COMPILER MATCHES "musl" OR CMAKE_C_COMPILER_TARGET MATCHES "musl")
  target_compile_definitions(libplatform-objects PRIVATE USE_MUSL=1)
endif()
set_target_properties(libplatform PROPERTIES OUTPUT_NAME platform WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
if(WIN32)
  target_link_libraries(libplatform PUBLIC dbghelp)
elseif(NOT EMSCRIPTEN)
  target_link_libraries(libplatform PUBLIC ${CMAKE_DL_LIBS})
endif()

if(PROJECT_IS_TOP_LEVEL)
  install(TARGETS libplatform EXPORT libplatformTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  if(TARGET utf8proc)
    install(TARGETS utf8proc EXPORT libplatformTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES "${LIBPLATFORM_UTF8PROC_SOURCE_DIR}/utf8proc.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  elseif(TARGET libplatform-utf8proc)
    install(TARGETS libplatform-utf8proc EXPORT libplatformTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  endif()
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(EXPORT libplatformTargets NAMESPACE libplatform:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libplatform)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/libplatformConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libplatformConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libplatform")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libplatformConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion)
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libplatformConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libplatformConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libplatform")
  install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/libplatform")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/libplatform.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libplatform.pc" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libplatform.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

if(LIBPLATFORM_BUILD_TESTS)
  enable_testing()
  add_executable(libplatform-test tests/test_backtrace.c)
  target_link_libraries(libplatform-test PRIVATE libplatform)
  add_test(NAME libplatform-test COMMAND libplatform-test)
endif()
