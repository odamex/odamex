# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# Server flags. [AM] TODO: Test -O3 again sometime.
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

global_compile_options()

# #define SERVER_APP for use in the source code
add_definitions(-DSERVER_APP)

# Common
set(COMMON_DIR ../common)
file(GLOB COMMON_HEADERS ${COMMON_DIR}/*.h)
file(GLOB COMMON_SOURCES ${COMMON_DIR}/*.cpp)

# Server
set(SERVER_DIR src)
file(GLOB SERVER_HEADERS ${SERVER_DIR}/*.h)
file(GLOB SERVER_SOURCES ${SERVER_DIR}/*.cpp)
if(WIN32)
  set(SERVER_WIN32_DIR win32)
  file(GLOB SERVER_WIN32_HEADERS ${SERVER_WIN32_DIR}/*.h)
  set(SERVER_WIN32_RESOURCES ${SERVER_WIN32_DIR}/server.rc)
endif()

# JsonCpp
set(JSONCPP_DIR ../libraries/jsoncpp)
file(GLOB JSONCPP_HEADERS ${JSONCPP_DIR}/json/*.h)
set(JSONCPP_SOURCE ${JSONCPP_DIR}/jsoncpp.cpp)

# MiniUPnPc
if (USE_MINIUPNP)
  set(MINIUPNPC_DIR ../libraries/libminiupnpc)
  set(MINIUPNPC_STATIC_LIBRARIES upnpc-static)
endif()

#discordRPC
if (USE_DISCORDRPC)
  set(DISCORDRPC_DIR ../libraries/discord-rpc)
  set(DISCORDRPC_STATIC_LIBRARIES discord-rpc)
endif()

# git describe
set_source_files_properties(${COMMON_DIR}/version.cpp PROPERTIES COMPILE_FLAGS -DGIT_DESCRIBE=${GIT_DESCRIBE})

# Platform definitions
define_platform()

# Server definitions
add_definitions(-DJSON_IS_AMALGAMATION)

if (USE_MINIUPNP)
  add_definitions(-DODA_HAVE_MINIUPNP)
endif()

if(WIN32 AND NOT MSVC)
  add_definitions(-DWINVER=0x0500)
endif()
include_directories(${JSONCPP_DIR} ${COMMON_DIR} ${SERVER_DIR} ${SERVER_WIN32_DIR})

# Include miniupnpc if we want it
if (USE_MINIUPNP)
  include_directories(${MINIUPNPC_DIR})
endif()

# Set up FHS installation path
if(NOT APPLE AND NOT WIN32)
  add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

add_executable(odasrv
  ${JSONCPP_SOURCE} ${JSONCPP_HEADERS}
  ${COMMON_SOURCES} ${COMMON_HEADERS}
  ${SERVER_SOURCES} ${SERVER_HEADERS}
  ${SERVER_WIN32_HEADERS} ${SERVER_WIN32_RESOURCES})

if (USE_MINIUPNP)
  target_link_libraries(odasrv ${MINIUPNPC_STATIC_LIBRARIES})
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
