# Generate a Windows .def file from the active libplatform object files.
cmake_minimum_required(VERSION 3.20)

foreach(required NM_TOOL OBJECT_LIST_FILE OUTPUT_FILE LIBRARY_NAME)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

file(STRINGS "${OBJECT_LIST_FILE}" OBJECTS)
if(NOT OBJECTS)
  message(FATAL_ERROR "No object files in ${OBJECT_LIST_FILE}")
endif()

list(LENGTH OBJECTS OBJECT_COUNT)
message(STATUS "Scanning ${OBJECT_COUNT} libplatform object files")
list(FILTER OBJECTS INCLUDE REGEX "\\.(obj|o)$")
if(NOT OBJECTS)
  message(FATAL_ERROR "No exportable object files in ${OBJECT_LIST_FILE}")
endif()

# Use a response file so the command remains within Windows' command-line limit.
set(NM_RESPONSE "${OUTPUT_FILE}.objects.rsp")
set(NM_RESPONSE_CONTENT "")
foreach(object IN LISTS OBJECTS)
  file(TO_CMAKE_PATH "${object}" object_path)
  string(APPEND NM_RESPONSE_CONTENT "\"${object_path}\"\n")
endforeach()
file(WRITE "${NM_RESPONSE}" "${NM_RESPONSE_CONTENT}")

execute_process(
  COMMAND "${NM_TOOL}" --format=just-symbols --extern-only --defined-only "@${NM_RESPONSE}"
  OUTPUT_VARIABLE NM_OUTPUT
  ERROR_VARIABLE NM_ERROR
  RESULT_VARIABLE NM_RESULT)
if(NOT NM_RESULT STREQUAL "0")
  message(FATAL_ERROR "Symbol extraction failed (${NM_RESULT}): ${NM_ERROR}")
endif()

string(REPLACE "\r\n" "\n" NM_OUTPUT "${NM_OUTPUT}")
string(REPLACE "\r" "\n" NM_OUTPUT "${NM_OUTPUT}")
string(REPLACE "\n" ";" ALL_SYMBOLS "${NM_OUTPUT}")
list(TRANSFORM ALL_SYMBOLS STRIP)

# Multiple inputs add filename headings; only accept symbol-name lines.
list(FILTER ALL_SYMBOLS INCLUDE REGEX "^[^ \t]+$")
list(FILTER ALL_SYMBOLS EXCLUDE REGEX ":$")

# Exclude compiler/runtime/SDK/C++ implementation symbols. The remaining
# symbols come only from libplatform's active object list.
list(FILTER ALL_SYMBOLS EXCLUDE REGEX "^(__imp_|__real|__xmm@|__ymm@|_?GLOBAL__|DW\\.|_fltused|_tls_|__security|__guard|GS_|_RTC|\\?|^\\.|___|CLSID_|IID_|GUID_|MF[A-Z_]|__local_stdio|__sanitizer_|_Avx2|_Check_memory|printf|fprintf|snprintf|vsnprintf|vfprintf|vsprintf|sprintf|malloc|calloc|realloc|free|memcpy|memmove|memset|strlen|strcmp|strncmp|strcpy|strncpy|abort|exit|_Exit)$")
list(FILTER ALL_SYMBOLS EXCLUDE REGEX "^\\?.*")
list(FILTER ALL_SYMBOLS EXCLUDE REGEX "^\\..*")
list(FILTER ALL_SYMBOLS EXCLUDE REGEX "^(__sanitizer_|__asan_|__ubsan_|__llvm_).*")
list(FILTER ALL_SYMBOLS EXCLUDE REGEX "^[A-Za-z]$|^a[0-9]+$")
list(REMOVE_DUPLICATES ALL_SYMBOLS)
list(SORT ALL_SYMBOLS)

list(LENGTH ALL_SYMBOLS EXPORT_COUNT)
list(TRANSFORM ALL_SYMBOLS PREPEND "    ")
list(JOIN ALL_SYMBOLS "\n" EXPORTS)
file(WRITE "${OUTPUT_FILE}" "LIBRARY ${LIBRARY_NAME}\nEXPORTS\n${EXPORTS}\n")
message(STATUS "Generated ${OUTPUT_FILE} with ${EXPORT_COUNT} exported symbols")
