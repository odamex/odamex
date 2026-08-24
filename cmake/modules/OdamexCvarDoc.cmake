# Cvar documentation
#
# The client and the server refresh those documents as they build, and "cvardocs-check"
# fails a build that would have changed one.
#
# This file is used two ways:
#
#   include(OdamexCvarDoc)       defines the odamex_cvardoc_* functions
#   cmake -D ... -P <this file>  performs one of the build-time actions that
#                                those functions attach to a target
#
# The second mode is needed because refreshing a document means running the binary
# that was just built, which cannot happen while CMake is still configuring.

# Refreshes a committed document from a freshly built binary.
#
# Reads BINARY and OUTPUT.  When CHECK is set the committed file is never
# written -- a stale one is an error instead.
function(odamex_cvardoc_write_document)
  foreach(_required BINARY OUTPUT)
    if(NOT DEFINED ${_required})
      message(FATAL_ERROR "cvardoc: ${_required} must be set")
    endif()
  endforeach()

  get_filename_component(_binary "${BINARY}" ABSOLUTE)
  get_filename_component(_binary_dir "${_binary}" DIRECTORY)
  get_filename_component(_binary_stem "${_binary}" NAME_WE)
  get_filename_component(_output_name "${OUTPUT}" NAME)

  if(NOT EXISTS "${_binary}")
    message(FATAL_ERROR "cvardoc: no such binary: ${_binary}")
  endif()

  set(_empty_stdin "${_binary_dir}/${_binary_stem}_cvardoc.stdin")
  file(WRITE "${_empty_stdin}" "")

  execute_process(
    COMMAND "${_binary}" --cvardocjson
    INPUT_FILE "${_empty_stdin}"
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    RESULT_VARIABLE _result
    TIMEOUT 120)

  file(REMOVE "${_empty_stdin}")

  if(NOT _result EQUAL 0)
    message(FATAL_ERROR
      "cvardoc: ${_binary} --cvardocjson failed (${_result})\n${_stderr}")
  endif()

  if(_stdout MATCHES "^[ \t\r\n]*\\{")
    set(_fresh "${_stdout}")
  else()
    set(_dump_file "${_binary_dir}/${_binary_stem}_cvardoc.json")
    if(NOT EXISTS "${_dump_file}")
      message(FATAL_ERROR
        "cvardoc: ${_binary} printed no document and wrote no ${_dump_file}")
    endif()
    file(READ "${_dump_file}" _fresh)
  endif()

  # Blank the fields that change on every commit.
  foreach(_volatile odamex_commithash odamex_branchname)
    string(REGEX REPLACE "\"${_volatile}\"[ \t]*:[ \t]*\"[^\"]*\""
      "\"${_volatile}\" : \"\"" _fresh "${_fresh}")
  endforeach()

  set(_existing "")
  if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" _existing)
  endif()

  if(_existing STREQUAL _fresh)
    message(STATUS "cvardoc: ${_output_name} is up to date")
    return()
  endif()

  if(CHECK)
    message(FATAL_ERROR
      "cvardoc: ${OUTPUT} is out of date.\n"
      "\n"
      "  If this only fails on some platforms, the cause is probably a cvar whose "
      "default, range, help text or the cvar itself is decided at compile time "
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

  file(WRITE "${OUTPUT}" "${_fresh}")
  message(STATUS "cvardoc: updated ${_output_name}")
endfunction()

# Renders one document as a byte array plus the string_view that spans it.
#
# The array itself stays internal to the generated file -- odalaunch only ever
# sees the view, so nothing has to agree on a separate length symbol.
function(odamex_cvardoc_emit_array out_var symbol path)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "cvardoc: no such file: ${path}")
  endif()

  # Read file as hex, get length in bytes
  file(READ "${path}" _hex HEX)

  string(LENGTH "${_hex}" _hexlen)
  math(EXPR _bytelen "${_hexlen} / 2")
  if(_bytelen EQUAL 0)
    message(FATAL_ERROR "cvardoc: ${path} is empty")
  endif()

  # Break up one big hex string into smaller byte chars
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")

  # Break the lines up so the file is not one enormous line.
  string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)"
    "\\1\n\t" _bytes "${_bytes}")

  set(${out_var}
    "static const char ${symbol}_DATA[] = {\n\t${_bytes}\n};\nextern const std::string_view ${symbol}(${symbol}_DATA, ${_bytelen});\n\n"
    PARENT_SCOPE)
endfunction()

# Turns the committed documents into a C++ source file for odalaunch.
#
# Reads CLIENT_JSON, SERVER_JSON and OUTPUT.
function(odamex_cvardoc_write_source)
  foreach(_required CLIENT_JSON SERVER_JSON OUTPUT)
    if(NOT DEFINED ${_required})
      message(FATAL_ERROR "cvardoc: ${_required} must be set")
    endif()
  endforeach()

  odamex_cvardoc_emit_array(_client_array ODAMEX_CVARDOC_JSON "${CLIENT_JSON}")
  odamex_cvardoc_emit_array(_server_array ODASRV_CVARDOC_JSON "${SERVER_JSON}")

  get_filename_component(_client_name "${CLIENT_JSON}" NAME)
  get_filename_component(_server_name "${SERVER_JSON}" NAME)

  set(_contents "// Generated by cmake/modules/OdamexCvarDoc.cmake - do not edit.\n")
  string(APPEND _contents "//\n")
  string(APPEND _contents "// Source documents: ${_client_name}, ${_server_name}\n")
  string(APPEND _contents "\n#include <string_view>\n\n")
  string(APPEND _contents "${_client_array}")
  string(APPEND _contents "${_server_array}")

  # Only rewrite when the contents change, so odalaunch does not relink for a
  # regenerated but identical source.
  set(_existing "")
  if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" _existing)
  endif()

  if(NOT _existing STREQUAL _contents)
    file(WRITE "${OUTPUT}" "${_contents}")
  endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE)

  cmake_minimum_required(VERSION 3.19)

  if(CVARDOC_ACTION STREQUAL "document")
    odamex_cvardoc_write_document()
  elseif(CVARDOC_ACTION STREQUAL "source")
    odamex_cvardoc_write_source()
  else()
    message(FATAL_ERROR "cvardoc: unknown CVARDOC_ACTION \"${CVARDOC_ACTION}\"")
  endif()

else()

  # -------------------------------------------------------------------------
  # Configure-time helpers.
  # -------------------------------------------------------------------------

  set(ODAMEX_CVARDOC_MODULE "${CMAKE_CURRENT_LIST_FILE}")
  set(ODAMEX_CVARDOC_RES_DIR "${CMAKE_SOURCE_DIR}/odalaunch/res")

  option(ODAMEX_UPDATE_CVARDOCS
    "Refresh the committed cvar documentation when the client or server builds" ON)

  # Refreshes this target's committed document every time it builds.
  function(odamex_cvardoc_refresh target)
    if(NOT ODAMEX_UPDATE_CVARDOCS)
      return()
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_COMMAND}"
      -D "CVARDOC_ACTION=document"
      -D "BINARY=$<TARGET_FILE:${target}>"
      -D "OUTPUT=${ODAMEX_CVARDOC_RES_DIR}/${target}_cvardoc.json"
      -P "${ODAMEX_CVARDOC_MODULE}"
      VERBATIM)
  endfunction()

  # Adds the "cvardocs" and "cvardocs-check" targets, which refresh the
  # committed documentation on demand and verify it is current, respectively.
  #
  # Call this after the client and server have been added.
  function(odamex_cvardoc_add_targets)
    if(NOT TARGET odamex AND NOT TARGET odasrv)
      return()
    endif()

    add_custom_target(cvardocs COMMENT "Refreshing cvar documentation")
    add_custom_target(cvardocs-check
      COMMENT "Checking cvar documentation is up to date")

    foreach(_target odamex odasrv)
      if(NOT TARGET ${_target})
        continue()
      endif()

      add_custom_command(TARGET cvardocs POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
        -D "CVARDOC_ACTION=document"
        -D "BINARY=$<TARGET_FILE:${_target}>"
        -D "OUTPUT=${ODAMEX_CVARDOC_RES_DIR}/${_target}_cvardoc.json"
        -P "${ODAMEX_CVARDOC_MODULE}"
        VERBATIM)
      add_dependencies(cvardocs ${_target})

      add_custom_command(TARGET cvardocs-check POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
        -D "CVARDOC_ACTION=document"
        -D "BINARY=$<TARGET_FILE:${_target}>"
        -D "OUTPUT=${ODAMEX_CVARDOC_RES_DIR}/${_target}_cvardoc.json"
        -D "CHECK=ON"
        -P "${ODAMEX_CVARDOC_MODULE}"
        VERBATIM)
      add_dependencies(cvardocs-check ${_target})
    endforeach()
  endfunction()

  # Generates the source file that compiles the committed documents into
  # odalaunch, and sets out_var to its path so it can be added to the target.
  function(odamex_cvardoc_source out_var)
    set(_client_json "${ODAMEX_CVARDOC_RES_DIR}/odamex_cvardoc.json")
    set(_server_json "${ODAMEX_CVARDOC_RES_DIR}/odasrv_cvardoc.json")
    set(_source "${CMAKE_CURRENT_BINARY_DIR}/cvardoc_data.cpp")

    add_custom_command(
      OUTPUT "${_source}"
      DEPENDS "${_client_json}" "${_server_json}" "${ODAMEX_CVARDOC_MODULE}"
      COMMAND "${CMAKE_COMMAND}"
      -D "CVARDOC_ACTION=source"
      -D "CLIENT_JSON=${_client_json}"
      -D "SERVER_JSON=${_server_json}"
      -D "OUTPUT=${_source}"
      -P "${ODAMEX_CVARDOC_MODULE}"
      COMMENT "Embedding cvar documentation"
      VERBATIM)

    set(${out_var} "${_source}" PARENT_SCOPE)
  endfunction()

endif()
