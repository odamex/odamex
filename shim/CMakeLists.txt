include(OdamexTargetSettings)

# Use C++17 everywhere
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# Shim flags. [AM] TODO: Test -O3 again sometime.
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# #define SHIM_APP for use in the source code
add_definitions(-DSHIM_APP)

# Create directory for Discord SDK
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/discord")

# Get Discord SDK Library
 file(DOWNLOAD
        "https://dl-game-sdk.discordapp.net/3.2.1/discord_game_sdk.zip"
        "${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk-3.2.1.zip"
        EXPECTED_HASH SHA256=6757bb4a1f5b42aa7b6707cbf2158420278760ac5d80d40ca708bb01d20ae6b4)
 execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk-3.2.1.zip"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/discord")

# Add Discord SDK Library
add_library(dsdk_lib_import SHARED IMPORTED)

string(TOLOWER "${ODAMEX_TARGET_ARCH}" arch_lower) # Upper case in Windows but lowercase on Linux 

# Set library target version
if(arch_lower STREQUAL "i386" OR CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
set(DISCORD_ARCH "x86")
elseif(arch_lower STREQUAL "amd64")
set(DISCORD_ARCH "x86_64")
elseif(arch_lower STREQUAL "arm")
set(DISCORD_ARCH "aarch64")
else()
message(FATAL_ERROR "Cannot compile shim on this architecture.")
endif()

set(DISCORD_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/discord/lib/${DISCORD_ARCH})

message(STATUS ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
  configure_file(
    ${DISCORD_LIB_DIR}/discord_game_sdk.dll ${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk.dll COPYONLY )
  configure_file(
    ${DISCORD_LIB_DIR}/discord_game_sdk.dll.lib ${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk.dll.lib COPYONLY )
  set_target_properties( dsdk_lib_import PROPERTIES
    IMPORTED_LOCATION ${DISCORD_LIB_DIR}/discord_game_sdk.dll
    IMPORTED_IMPLIB ${DISCORD_LIB_DIR}/discord_game_sdk.dll.lib )
elseif(APPLE)
  configure_file(
    ${DISCORD_LIB_DIR}/discord_game_sdk.dylib ${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk.dylib COPYONLY )
  set_target_properties(
    dsdk_lib_import PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk.dylib )
else()
  configure_file(
    ${DISCORD_LIB_DIR}/discord_game_sdk.so ${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk.so COPYONLY )
  set_target_properties(
    dsdk_lib_import PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/discord_game_sdk.so )
endif()

set(DISCORD_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/discord/cpp)

# Shim
file(GLOB SHIM_SOURCES src/*.cpp src/*.h)
file(GLOB SHIM_DISCORD_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/discord/cpp/*.cpp ${CMAKE_CURRENT_BINARY_DIR}/discord/cpp/*.h)
if(WIN32)
  file(GLOB SHIM_WIN32_SOURCES win32/*.h)
  configure_file(win32/shim.rc.in "${CMAKE_CURRENT_BINARY_DIR}/shim.rc" @ONLY)
  list(APPEND SHIM_WIN32_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/shim.rc")
endif()

# Tag source files with correct directories
source_group("Shim" FILES ${SHIM_SOURCES})
source_group("Win32" FILES ${SHIM_WIN32_SOURCES})
source_group("Discord" FILES ${SHIM_DISCORD_SOURCES})

if(WIN32 AND NOT MSVC)
  add_definitions(-DWINVER=0x0500)
endif()

add_executable(odashim
  ${SHIM_SOURCES} ${SHIM_WIN32_SOURCES} ${SHIM_DISCORD_SOURCES})
odamex_target_settings(odashim)

target_link_libraries(odashim dsdk_lib_import)

target_include_directories(odashim PRIVATE src)
target_include_directories(odashim PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/discord/cpp)
if(WIN32)
  target_include_directories(odashim PRIVATE win32)
endif()

if(WIN32)
  target_link_libraries(odashim winmm wsock32 shlwapi)
elseif(SOLARIS)
  target_link_libraries(odashim socket nsl)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(odashim pthread)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(odashim rt)
endif()

if(APPLE)
elseif(WIN32)
  install(TARGETS odashim
    RUNTIME DESTINATION .
    COMPONENT shim)
else()
  install(TARGETS odashim
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT shim)
endif()

# [AM] Not needed right now, but could be in the future...
if(BUILD_OR_FAIL AND NOT TARGET odashim)
  message(FATAL_ERROR "Odashim target could not be generated")
endif()
