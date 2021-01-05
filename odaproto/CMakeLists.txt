# Find the protobuf we just compiled
find_package(protobuf
  PATHS "${CMAKE_CURRENT_BINARY_DIR}/../libraries/local"
  NO_DEFAULT_PATH)

protobuf_generate(OUT_VAR ODAPROTO_SOURCES PROTOS server.proto)

add_library(odaproto STATIC ${ODAPROTO_SOURCES})
target_include_directories(odaproto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(odaproto protobuf::libprotobuf)
