include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# Enable default SIMD settings for standard builds.
option(USE_DEFAULT_SIMD "Use default SIMD optimization settings" ON)

file(GLOB TEST_SOURCES *.cpp *.h)

# we need common in here to actually test it but it doesn't work without the server or client sources too

# get_target_property(COMMON_SOURCES odamex-common INTERFACE_SOURCES)
source_group("Test" FILES ${TEST_SOURCES})
# source_group("Common" FILES ${COMMON_SOURCES})

# include_directories(${COMMON_DIR})

add_executable(odagtest ${TEST_SOURCES})
# add_executable(odagtest ${COMMON_SOURCES} ${TEST_SOURCES})
target_link_libraries(odagtest GTest::gtest_main)
target_link_libraries(odagtest fmt::fmt tinylibs ZLIB::ZLIB odaproto minilzo)
# target_link_libraries(odagtest fmt::fmt tinylibs ZLIB::ZLIB odamex-common odaproto minilzo)
odamex_target_settings(odagtest)