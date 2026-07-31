# Refreshes a committed cvar documentation file from a freshly built binary.
#
#   cmake -D BINARY=<exe> -D OUTPUT=<committed.json> [-D CHECK=ON]
#         -P update_cvardoc.cmake
#
# Runs the binary with -cvardocjson and rewrites the committed file only when
# the cvars actually differ.
#
# When CHECK=ON, the committed file is never written -- a stale one is an error
# instead.

cmake_minimum_required(VERSION 3.19)

foreach(_required BINARY OUTPUT)
  if(NOT DEFINED ${_required})
    message(FATAL_ERROR "update_cvardoc: ${_required} must be set")
  endif()
endforeach()

get_filename_component(BINARY "${BINARY}" ABSOLUTE)
get_filename_component(BINARY_DIR "${BINARY}" DIRECTORY)
get_filename_component(BINARY_STEM "${BINARY}" NAME_WE)
get_filename_component(OUTPUT_NAME "${OUTPUT}" NAME)

if(NOT EXISTS "${BINARY}")
  message(FATAL_ERROR "update_cvardoc: no such binary: ${BINARY}")
endif()

set(EMPTY_STDIN "${BINARY_DIR}/${BINARY_STEM}_cvardoc.stdin")
file(WRITE "${EMPTY_STDIN}" "")

execute_process(
  COMMAND "${BINARY}" -cvardocjson
  INPUT_FILE "${EMPTY_STDIN}"
  OUTPUT_VARIABLE DUMP_STDOUT
  ERROR_VARIABLE DUMP_STDERR
  RESULT_VARIABLE DUMP_RESULT
  TIMEOUT 120)

file(REMOVE "${EMPTY_STDIN}")

if(NOT DUMP_RESULT EQUAL 0)
  message(FATAL_ERROR
    "update_cvardoc: ${BINARY} -cvardocjson failed (${DUMP_RESULT})\n${DUMP_STDERR}")
endif()

if(DUMP_STDOUT MATCHES "^[ \t\r\n]*\\{")
  set(FRESH "${DUMP_STDOUT}")
else()
  set(DUMP_FILE "${BINARY_DIR}/${BINARY_STEM}_cvardoc.json")
  if(NOT EXISTS "${DUMP_FILE}")
    message(FATAL_ERROR
      "update_cvardoc: ${BINARY} printed no document and wrote no ${DUMP_FILE}")
  endif()
  file(READ "${DUMP_FILE}" FRESH)
endif()

# Blank the fields that change on every commit.
foreach(_volatile odamex_commithash odamex_branchname)
  string(REGEX REPLACE "\"${_volatile}\"[ \t]*:[ \t]*\"[^\"]*\""
    "\"${_volatile}\" : \"\"" FRESH "${FRESH}")
endforeach()

set(EXISTING "")
if(EXISTS "${OUTPUT}")
  file(READ "${OUTPUT}" EXISTING)
endif()

if(EXISTING STREQUAL FRESH)
  message(STATUS "cvardoc: ${OUTPUT_NAME} is up to date")
  return()
endif()

if(CHECK)
  message(FATAL_ERROR
    "cvardoc: ${OUTPUT} is out of date.\n"
    "\n"
    "  If this only fails on some platforms, the cause is probably a cvar whose "
    "default, range, help text or the cvar itself is decided at compile time"
    "rather than written as a literal.\n"
    "\n"
    "  There is one committed cvardocjson for client/server for every platform, so "
    "anything that varies between builds cannot live in a cvar's declared "
    "parameters -- it will pass on the machine that generated the document and "
    "fail everywhere else.\n"
    "\n"
    "  Give the cvar a fixed literal instead and apply the platform-specific "
    "part at runtime in its CVAR_FUNC_IMPL callback, which runs once for every "
    "cvar at startup. No-op on non-target platforms if needed.\n"
    "\n"
    "If the above doesn't apply, build the cvardocs target and commit the result.")
endif()

file(WRITE "${OUTPUT}" "${FRESH}")
message(STATUS "cvardoc: updated ${OUTPUT_NAME}")
