include(OdamexCopyWad)
include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

global_compile_options()

# Server flags. [AM] TODO: Test -O3 again sometime.
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# #define SERVER_APP for use in the source code
add_definitions(-DSERVER_APP)

# Server
set(SERVER_DIR src)
file(GLOB SERVER_HEADERS ${SERVER_DIR}/*.h)
file(GLOB SERVER_SOURCES ${SERVER_DIR}/*.cpp)
if(WIN32)
  set(SERVER_WIN32_DIR win32)
  file(GLOB SERVER_WIN32_HEADERS ${SERVER_WIN32_DIR}/*.h)
  set(SERVER_WIN32_RESOURCES ${SERVER_WIN32_DIR}/server.rc)
endif()

# Platform definitions
define_platform()

# Server definitions
if(USE_MINIUPNP)
  add_definitions(-DODA_HAVE_MINIUPNP)
endif()

if(WIN32 AND NOT MSVC)
  add_definitions(-DWINVER=0x0500)
endif()
include_directories(${SERVER_DIR} ${SERVER_WIN32_DIR})

# Set up FHS installation path
if(NOT APPLE AND NOT WIN32)
  add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

add_executable(odasrv
  ${SERVER_SOURCES} ${SERVER_HEADERS}
  ${SERVER_WIN32_HEADERS} ${SERVER_WIN32_RESOURCES})
odamex_target_settings(odasrv)
target_link_libraries(odasrv jsoncpp odamex-common)

if(USE_MINIUPNP)
  target_link_libraries(odasrv upnpc-static)
endif()

if(WIN32)
  target_link_libraries(odasrv winmm wsock32)
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

print_cxx_flags()
