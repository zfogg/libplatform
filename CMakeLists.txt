cmake_minimum_required(VERSION 3.20)
project(libplatform VERSION 0.1.0 LANGUAGES C)

option(LIBPLATFORM_BUILD_TESTS "Build libplatform tests" ON)
include(GNUInstallDirs)

add_library(libplatform-objects OBJECT)
set_target_properties(libplatform-objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(BUILD_SHARED_LIBS)
  add_library(libplatform SHARED $<TARGET_OBJECTS:libplatform-objects>)
else()
  add_library(libplatform STATIC $<TARGET_OBJECTS:libplatform-objects>)
endif()
add_library(libplatform::libplatform ALIAS libplatform)
if(PROJECT_IS_TOP_LEVEL)
  target_compile_definitions(libplatform-objects PRIVATE LIBPLATFORM_STANDALONE=1)
endif()
target_sources(libplatform-objects PRIVATE
  lib/symbols.c lib/lifecycle.c lib/mutex.c lib/cond.c lib/system_common.c lib/terminal.c lib/util/path.c lib/util/string.c
)
target_sources(libplatform-objects PRIVATE lib/thread.c lib/time.c lib/named.c lib/cpu.c lib/process.c)
if(WIN32)
    target_sources(libplatform-objects PRIVATE
      lib/windows/backtrace.c lib/windows/string.c lib/windows/filesystem.c lib/windows/terminal.c lib/windows/terminal_capabilities.c lib/windows/agent.c
      lib/windows/process.c lib/windows/rwlock.c lib/windows/mutex.c lib/windows/cond.c lib/windows/system.c
    )
elseif(EMSCRIPTEN)
    target_sources(libplatform-objects PRIVATE lib/wasm/backtrace.c lib/wasm/terminal.c)
else()
    target_sources(libplatform-objects PRIVATE
      lib/backtrace.c lib/posix/string.c lib/posix/filesystem.c lib/posix/terminal.c lib/posix/terminal_sync.c lib/posix/terminal_capabilities.c lib/posix/agent.c
      lib/posix/process.c lib/posix/rwlock.c lib/posix/mutex.c lib/posix/cond.c
    )
endif()

if(APPLE)
  target_sources(libplatform-objects PRIVATE lib/macos/system.c)
elseif(UNIX)
  target_sources(libplatform-objects PRIVATE lib/linux/system.c lib/linux/systemd.c)
endif()
target_include_directories(libplatform-objects PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(libplatform PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(libplatform-objects PUBLIC c_std_11)
target_compile_features(libplatform PUBLIC c_std_11)
set_target_properties(libplatform PROPERTIES OUTPUT_NAME platform)
if(WIN32)
  target_link_libraries(libplatform PUBLIC dbghelp)
elseif(NOT EMSCRIPTEN)
  target_link_libraries(libplatform PUBLIC ${CMAKE_DL_LIBS})
endif()

install(TARGETS libplatform EXPORT libplatformTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT libplatformTargets NAMESPACE libplatform:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libplatform)

if(LIBPLATFORM_BUILD_TESTS)
  enable_testing()
  add_executable(libplatform-test tests/test_backtrace.c)
  target_link_libraries(libplatform-test PRIVATE libplatform)
  add_test(NAME libplatform-test COMMAND libplatform-test)
endif()
