### Protocol Buffers ###

if(BUILD_CLIENT OR BUILD_SERVER)
  set(_PROTOBUF_BUILDGEN_PARAMS
    "-Dprotobuf_BUILD_SHARED_LIBS=OFF"
    "-Dprotobuf_BUILD_TESTS=OFF"
    "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF")

  if(MSVC)
    # https://developercommunity.visualstudio.com/t/Visual-Studio-1740-no-longer-compiles-/10193665
    set(protobuf_CXXFLAGS "/D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS")
  endif()

  lib_buildgen(
    LIBRARY protobuf
    SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/cmake"
    PARAMS ${_PROTOBUF_BUILDGEN_PARAMS}
    CXXFLAGS ${protobuf_CXXFLAGS})
  lib_build(LIBRARY protobuf)
endif()
