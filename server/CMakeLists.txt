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
set(JSONCPP_DIR ../jsoncpp)
file(GLOB JSONCPP_HEADERS ${JSONCPP_DIR}/json/*.h)
set(JSONCPP_SOURCE ${JSONCPP_DIR}/jsoncpp.cpp)

# Platform definitions
define_platform()

# Server definitions
add_definitions(-DJSON_IS_AMALGAMATION)
if(WIN32 AND NOT MSVC)
  add_definitions(-DWINVER=0x0500)
endif()
include_directories(${JSONCPP_DIR} ${COMMON_DIR} ${SERVER_DIR} ${SERVER_WIN32_DIR})

# MiniUPnPc
find_package(MiniUPnPc)
if(MINIUPNPC_FOUND)
  add_definitions(-DODA_HAVE_MINIUPNP)
  include_directories(${MINIUPNPC_INCLUDE_DIRS})
else()
  message(WARNING "MiniUPnPc not found, server will not be built with MiniUPnPc support.")
endif()

add_executable(odasrv
  ${JSONCPP_SOURCE} ${JSONCPP_HEADERS}
  ${COMMON_SOURCES} ${COMMON_HEADERS}
  ${SERVER_SOURCES} ${SERVER_HEADERS}
  ${SERVER_WIN32_HEADERS} ${SERVER_WIN32_RESOURCES})

if(MINIUPNPC_FOUND)
  if(MINIUPNPC_STATIC_LIBRARIES)
    target_link_libraries(odasrv ${MINIUPNPC_STATIC_LIBRARIES})
  else()
    target_link_libraries(odasrv ${MINIUPNPC_LIBRARIES})
  endif()
endif()

if(WIN32)
  target_link_libraries(odasrv winmm wsock32)
elseif(SOLARIS)
  target_link_libraries(odasrv socket nsl)
endif()
