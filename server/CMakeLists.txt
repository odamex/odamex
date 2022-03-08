include(OdamexCopyWad)
include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# Server flags. [AM] TODO: Test -O3 again sometime.
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# #define SERVER_APP for use in the source code
add_definitions(-DSERVER_APP)

# Server
file(GLOB SERVER_SOURCES src/*.cpp src/*.h)
if(WIN32)
  file(GLOB SERVER_WIN32_SOURCES win32/*.h)
  configure_file(win32/server.rc.in "${CMAKE_CURRENT_BINARY_DIR}/server.rc" @ONLY)
  list(APPEND SERVER_WIN32_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/server.rc")
endif()

# Tag source files with correct directories
get_target_property(COMMON_SOURCES odamex-common INTERFACE_SOURCES)
source_group("Common" FILES ${COMMON_SOURCES})
source_group("Server" FILES ${SERVER_SOURCES})
source_group("Win32" FILES ${SERVER_WIN32_SOURCES})

# Server definitions
if(USE_MINIUPNP)
  add_definitions(-DODA_HAVE_MINIUPNP)
endif()

if(WIN32 AND NOT MSVC)
  add_definitions(-DWINVER=0x0500)
endif()

# Set up FHS installation path
if(NOT APPLE AND NOT WIN32)
  add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

add_executable(odasrv
  ${COMMON_SOURCES} ${SERVER_SOURCES} ${SERVER_WIN32_SOURCES})
odamex_target_settings(odasrv)

if(USE_INTERNAL_JSONCPP)
  set_property(TARGET odasrv PROPERTY CXX_STANDARD 98)
else()
  set_property(TARGET odasrv PROPERTY CXX_STANDARD 11)
endif()

target_include_directories(odasrv PRIVATE src)
if(WIN32)
  target_include_directories(odasrv PRIVATE win32)
endif()
target_link_libraries(odasrv fmt::fmt ZLIB::ZLIB odamex-common odaproto)

if(USE_INTERNAL_JSONCPP)
  target_link_libraries(odasrv jsoncpp_lib_static)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(JSONCPP jsoncpp REQUIRED IMPORTED_TARGET)
  target_link_libraries(odasrv PkgConfig::JSONCPP)
endif()

if(USE_MINIUPNP)
  if(USE_INTERNAL_MINIUPNP)
    target_link_libraries(odasrv miniupnpc::miniupnpc)
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MINIUPNPC miniupnpc REQUIRED IMPORTED_TARGET)
    target_link_libraries(odasrv PkgConfig::MINIUPNPC)
  endif()
endif()

if(WIN32)
  target_link_libraries(odasrv winmm wsock32 shlwapi)
elseif(SOLARIS)
  target_link_libraries(odasrv socket nsl)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(odasrv pthread)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(odasrv rt)
endif()

odamex_copy_wad(odasrv)

if(APPLE)
elseif(WIN32)
  install(TARGETS odasrv
    RUNTIME DESTINATION .
    COMPONENT server)
else()
  install(TARGETS odasrv
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT server)
endif()

# [AM] Not needed right now, but could be in the future...
if(BUILD_OR_FAIL AND NOT TARGET odasrv)
  message(FATAL_ERROR "Odasrv target could not be generated")
endif()
