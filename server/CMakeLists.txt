include(OdamexCvarDoc)
include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# Server flags. [AM] TODO: Test -O3 again sometime.
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# Server
file(GLOB SERVER_SOURCES src/*.cpp src/*.h)
if(WIN32)
  file(GLOB SERVER_WIN32_SOURCES win32/*.h)
  configure_file(win32/server.rc.in "${CMAKE_CURRENT_BINARY_DIR}/server.rc" @ONLY)
  list(APPEND SERVER_WIN32_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/server.rc")
endif()

add_executable(odasrv ${SERVER_SOURCES} ${SERVER_WIN32_SOURCES})
odamex_target_settings(TARGET odasrv)
odamex_target_errors(odasrv)

target_link_libraries(odasrv odacommon)
target_compile_definitions(odasrv PRIVATE SERVER_APP)

get_target_property(COMMON_SOURCES odacommon INTERFACE_SOURCES)
source_group("Common" FILES ${COMMON_SOURCES})
source_group("Server" FILES ${SERVER_SOURCES})
source_group("Win32" FILES ${SERVER_WIN32_SOURCES})

# Server definitions
if (USE_MINIUPNP)
    target_compile_definitions(odasrv PRIVATE ODA_HAVE_MINIUPNP)
endif()

if (WIN32 AND NOT MSVC)
    target_compile_definitions(odasrv PRIVATE WINVER=0x0500)
endif()

target_include_directories(odasrv PRIVATE src)
if (WIN32)
  target_include_directories(odasrv PRIVATE win32)

  # We need to call SetProcessInformation to disable power throttling,
  # but the API itself doesn't exist until Windows 8.  Here we set the minimum
  # Windows version to 8.  (Windows Server 2012, BTW)
  target_compile_definitions(odasrv PRIVATE _WIN32_WINNT=0x0602)

  # We supply our own manifest, don't use the default one.
  set_target_properties(odasrv PROPERTIES LINK_FLAGS "/MANIFEST:NO")
endif()

if (TARGET odawad)
    add_dependencies(odasrv odawad)
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

if(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(odasrv pthread)
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
  install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/net.odamex.Odamex.Server.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    COMPONENT server)
  install(FILES ${CMAKE_SOURCE_DIR}/media/icon_odasrv_512.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps
    RENAME net.odamex.Odamex.Server.png
    COMPONENT server)
endif()

if(TARGET odasrv)
  odamex_cvardoc_refresh(odasrv)
endif()

# [AM] Not needed right now, but could be in the future...
if(BUILD_OR_FAIL AND NOT TARGET odasrv)
  message(FATAL_ERROR "Odasrv target could not be generated")
endif()
