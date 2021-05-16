# Find the protobuf we just compiled
find_package(protobuf
  PATHS "${CMAKE_CURRENT_BINARY_DIR}/../libraries/local"
  NO_DEFAULT_PATH)

set(ODAPROTO_PROTOS common.proto server.proto)

protobuf_generate(OUT_VAR ODAPROTO_SOURCES PROTOS ${ODAPROTO_PROTOS})

source_group("Definitions" FILES ${ODAPROTO_PROTOS})
source_group("Generated" FILES ${ODAPROTO_SOURCES})

add_library(odaproto STATIC ${ODAPROTO_SOURCES} ${ODAPROTO_PROTOS})
target_include_directories(odaproto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(odaproto protobuf::libprotobuf)
