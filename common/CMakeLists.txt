file(GLOB COMMON_HEADERS *.h)
file(GLOB COMMON_SOURCES *.cpp)

file(GLOB RESOURCE_HEADERS resources/*.h)
file(GLOB RESOURCE_SOURCES resources/*.cpp)

configure_file(git_describe.h.in "${CMAKE_CURRENT_BINARY_DIR}/git_describe.h")

add_library(odamex-common INTERFACE)
target_sources(odamex-common INTERFACE ${COMMON_SOURCES} ${COMMON_HEADERS} ${RESOURCE_SOURCES} ${RESOURCE_HEADERS})
target_include_directories(odamex-common INTERFACE . ${CMAKE_CURRENT_BINARY_DIR})
