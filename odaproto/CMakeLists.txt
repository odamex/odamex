# Find the protobuf we just compiled
find_package(protobuf
  PATHS "${CMAKE_CURRENT_BINARY_DIR}/../libraries/local"
  NO_DEFAULT_PATH)

add_library(odaproto STATIC server.proto)
target_include_directories(odaproto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(odaproto protobuf::libprotobuf)
protobuf_generate(TARGET odaproto)
