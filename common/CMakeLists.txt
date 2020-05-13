file(GLOB COMMON_HEADERS *.h)
file(GLOB COMMON_SOURCES *.cpp)

add_library(odamex-common INTERFACE IMPORTED GLOBAL)
target_sources(odamex-common INTERFACE ${COMMON_SOURCES} ${COMMON_HEADERS})
target_include_directories(odamex-common INTERFACE .)
