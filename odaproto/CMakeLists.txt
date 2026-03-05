include(OdamexTargetSettings)

# Find the protobuf we just compiled
find_package(protobuf
  PATHS
    "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protobuf-build/cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protobuf-build/lib/cmake/protobuf"
    "${CMAKE_CURRENT_BINARY_DIR}/../libraries/protobuf-build/lib64/cmake/protobuf"
  NO_DEFAULT_PATH)

set(ODAPROTO_PROTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/common.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/server.proto
    )
set(ODAPROTO_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/common.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/common.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/server.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/server.pb.h
    )
add_custom_command(
    OUTPUT
        ${ODAPROTO_SOURCES}
    COMMAND
        protoc
    ARGS
        ${ODAPROTO_PROTOS}
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
        protoc
        ${ODAPROTO_PROTOS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

source_group("Definitions" FILES ${ODAPROTO_PROTOS})
source_group("Generated" FILES ${ODAPROTO_SOURCES})

add_library(odaproto STATIC ${ODAPROTO_SOURCES})# ${ODAPROTO_PROTOS})
odamex_target_settings(odaproto)
target_include_directories(odaproto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(odaproto protobuf::libprotobuf)

if(MSVC)
  # https://developercommunity.visualstudio.com/t/Visual-Studio-1740-no-longer-compiles-/10193665
  target_compile_definitions(odaproto PUBLIC "_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS")
endif()
