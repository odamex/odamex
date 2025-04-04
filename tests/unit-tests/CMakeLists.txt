include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# #define TEST_APP for use in the source code
add_definitions(-DTEST_APP)
add_definitions(-DODAMEX_NO_GITVER)

file(GLOB TEST_SOURCES src/*.cpp src/*.h test/*.cpp test/*.h)

set(COMMON_DIR ../../common)

# ew, but trying to link against odamex-common requires LOTS of stubs
# file(GLOB COMMON_SOURCES ${COMMON_DIR}/olumpname.cpp)
get_target_property(COMMON_SOURCES odamex-common INTERFACE_SOURCES)
source_group("Test" FILES ${TEST_SOURCES})
source_group("Common" FILES ${COMMON_SOURCES})

include_directories(${COMMON_DIR} src)

add_executable(odagtest ${COMMON_SOURCES} ${TEST_SOURCES})
target_link_libraries(odagtest GTest::gtest_main)
# target_link_libraries(odagtest fmt::fmt tinylibs ZLIB::ZLIB odaproto minilzo CURL::libcurl)
target_link_libraries(odagtest fmt::fmt tinylibs ZLIB::ZLIB odamex-common odaproto minilzo CURL::libcurl)
if(USE_INTERNAL_JSONCPP)
  target_link_libraries(odagtest jsoncpp_static)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(JSONCPP jsoncpp REQUIRED IMPORTED_TARGET)
  target_link_libraries(odagtest PkgConfig::JSONCPP)
endif()

if(WIN32)
  target_link_libraries(odagtest winmm wsock32 shlwapi)
elseif(UNIX AND NOT APPLE)
  target_link_libraries(odagtest rt)
endif()

odamex_target_settings(odagtest)
