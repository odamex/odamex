# Find the protobuf we just compiled
find_package(protobuf
  PATHS
    "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protobuf-build/cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protobuf-build/lib/cmake/protobuf"
    "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protobuf-build/lib64/cmake/protobuf"
  NO_DEFAULT_PATH)

# protoc was compiled separately using the host system (and not the target)
# so we need to point to it by hand.
if(CMAKE_HOST_WIN32)
  set(HOST_EXECUTABLE_SUFFIX ".exe")
else()
  set(HOST_EXECUTABLE_SUFFIX "")
endif()
get_cmake_property(_IS_MULTI_CONFIG GENERATOR_IS_MULTI_CONFIG)
if(_IS_MULTI_CONFIG)
  set(PROTOC_EXE "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protoc-build/RelWithDebInfo/protoc${HOST_EXECUTABLE_SUFFIX}")
else()
  set(PROTOC_EXE "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protoc-build/protoc${HOST_EXECUTABLE_SUFFIX}")
endif()

set_target_properties(protobuf::protoc PROPERTIES
  IMPORTED_LOCATION_DEBUG "${PROTOC_EXE}"
  IMPORTED_LOCATION_RELEASE "${PROTOC_EXE}"
  IMPORTED_LOCATION_RELWITHDEBINFO "${PROTOC_EXE}"
  IMPORTED_LOCATION_MINSIZEREL "${PROTOC_EXE}")

set(ODAPROTO_PROTOS common.proto server.proto)

protobuf_generate(OUT_VAR ODAPROTO_SOURCES PROTOS ${ODAPROTO_PROTOS})

source_group("Definitions" FILES ${ODAPROTO_PROTOS})
source_group("Generated" FILES ${ODAPROTO_SOURCES})

add_library(odaproto STATIC ${ODAPROTO_SOURCES} ${ODAPROTO_PROTOS})
target_include_directories(odaproto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(odaproto protobuf::libprotobuf)
