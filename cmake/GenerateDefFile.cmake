cmake_minimum_required(VERSION 3.20)

foreach(required NM_TOOL OBJECT_LIST_FILE OUTPUT_FILE LIBRARY_NAME PUBLIC_INCLUDE_DIR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} is required")
  endif()
endforeach()

file(STRINGS "${OBJECT_LIST_FILE}" OBJECTS)
list(FILTER OBJECTS INCLUDE REGEX "\\.(obj|o)$")
execute_process(
  COMMAND "${NM_TOOL}" --format=just-symbols --extern-only --defined-only ${OBJECTS}
  OUTPUT_VARIABLE SYMBOLS
  RESULT_VARIABLE RESULT)
if(NOT RESULT EQUAL 0)
  message(FATAL_ERROR "llvm-nm failed with ${RESULT}")
endif()

string(REPLACE "\r\n" "\n" SYMBOLS "${SYMBOLS}")
string(REPLACE "\r" "\n" SYMBOLS "${SYMBOLS}")
string(REPLACE "\n" ";" SYMBOLS "${SYMBOLS}")
set(EXPORTED_SYMBOLS)
foreach(symbol IN LISTS SYMBOLS)
  if(NOT symbol STREQUAL "")
    list(APPEND EXPORTED_SYMBOLS "${symbol}")
  endif()
endforeach()
list(REMOVE_DUPLICATES EXPORTED_SYMBOLS)
list(SORT EXPORTED_SYMBOLS)
list(TRANSFORM EXPORTED_SYMBOLS PREPEND "    ")
list(JOIN EXPORTED_SYMBOLS "\n" EXPORTS)
file(WRITE "${OUTPUT_FILE}" "LIBRARY ${LIBRARY_NAME}\nEXPORTS\n${EXPORTS}\n")
