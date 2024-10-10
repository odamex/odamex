### Protocol Buffers ###

if(BUILD_CLIENT OR BUILD_SERVER)
  message(STATUS "Compiling protobuf...")

  set(_PROTOBUF_BUILDGEN_PARAMS
    "-Dprotobuf_BUILD_SHARED_LIBS=OFF"
    "-Dprotobuf_BUILD_TESTS=OFF"
    "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF")

  lib_buildgen(
    LIBRARY protobuf
    SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/protobuf"
    PARAMS ${_PROTOBUF_BUILDGEN_PARAMS})
  lib_build(LIBRARY protobuf)
endif()
