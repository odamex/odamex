include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

file(GLOB TEST_SOURCES src/*.cpp src/*.h test/*.cpp test/*.h)

set(COMMON_DIR "${CMAKE_SOURCE_DIR}/common")

source_group("Test" FILES ${TEST_SOURCES})

add_executable(odagtest
    ${COMMON_SOURCES}
    ${TEST_SOURCES}
    "${CMAKE_SOURCE_DIR}/client/src/PlayerStateRoller.cpp"
    "${CMAKE_SOURCE_DIR}/client/src/PlayerStateRoller.h"
    )
target_include_directories(odagtest PRIVATE src "${CMAKE_SOURCE_DIR}/client/src")

target_link_libraries(odagtest GTest::gtest_main odacommon curl_interface)
target_compile_definitions(odagtest PRIVATE TEST_APP ODAMEX_NO_GITVER)

odamex_target_settings(TARGET odagtest NO_DEPLOYMENT_ARTIFACT)
odamex_target_errors(odagtest)
