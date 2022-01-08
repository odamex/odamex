file(GLOB ODALPAPI_SOURCES *.cpp *.h threads/*.cpp threads/*.h)

add_library(odalpapi INTERFACE)
target_sources(odalpapi INTERFACE ${ODALPAPI_SOURCES})
target_include_directories(odalpapi INTERFACE . ${CMAKE_CURRENT_BINARY_DIR})
