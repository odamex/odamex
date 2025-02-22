include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# #define TEST_APP for use in the source code
add_definitions(-DTEST_APP)

file(GLOB TEST_SOURCES *.cpp *.h)

get_target_property(COMMON_SOURCES odamex-common INTERFACE_SOURCES)
source_group("Test" FILES ${TEST_SOURCES})
source_group("Common" FILES ${COMMON_SOURCES})

include_directories(${COMMON_DIR})

# we really need common in here to actually test more things but it doesn't work without the server or client sources too
add_executable(odagtest ${TEST_SOURCES})
# add_executable(odagtest ${COMMON_SOURCES} ${TEST_SOURCES})
target_link_libraries(odagtest GTest::gtest_main)
target_link_libraries(odagtest fmt::fmt tinylibs ZLIB::ZLIB odaproto minilzo)
# target_link_libraries(odagtest fmt::fmt tinylibs ZLIB::ZLIB odamex-common odaproto minilzo)
odamex_target_settings(odagtest)