include(CMakeDependentOption)

project(Odamex)
cmake_minimum_required(VERSION 2.8)

set(PROJECT_VERSION 0.6.4)
set(PROJECT_COPYRIGHT "2006-2013")

# Default build type
if(NOT MSVC)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
  endif()
endif()

# Global compile options as shown in a GUI.
if(NOT MSVC)
  set(USE_STATIC_STDLIB "No" CACHE STRING
    "Statically link against the C and C++ Standard Library.")
  set_property(CACHE USE_STATIC_STDLIB PROPERTY STRINGS Yes No)

  set(USE_SANITIZE_ADDRESS "No" CACHE STRING
    "Turn on Address Sanitizer in Debug builds, requires GCC >= 4.8 or Clang >= 3.1")
  set_property(CACHE USE_SANITIZE_ADDRESS PROPERTY STRINGS Yes No)
endif()

# Global compile options.  Useful defines for any Odamex project.
macro(global_compile_options)
  if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
    if(USE_STATIC_STDLIB)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
    if(USE_SANITIZE_ADDRESS)
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    endif()
  endif()
endmacro(global_compile_options)

if( ${CMAKE_SYSTEM_NAME} MATCHES SunOS )
  set(SOLARIS 1)
endif()

# Platform definitions
# [AM] TODO: Eliminate redundant defines
macro(define_platform)
  if(APPLE)
    add_definitions(-DOSX -DUNIX)
  elseif(SOLARIS)
    add_definitions(-DSOLARIS -DUNIX -DBSD_COMP -gstabs+)
  elseif(UNIX)
    add_definitions(-DUNIX)
  endif()
endmacro(define_platform)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# MiniUPnPc
set(UPNPC_BUILD_STATIC ON CACHE INTERNAL "" FORCE)
set(UPNPC_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)
set(UPNPC_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(UPNPC_INSTALL OFF CACHE INTERNAL "" FORCE)
mark_as_advanced(FORCE UPNPC_INSTALL)
add_subdirectory(libminiupnpc)

# PortMidi
cmake_dependent_option(USE_INTREE_PORTMIDI "Compile with the version of PortMidi included in the source tree." ON "WIN32" OFF)
if(USE_INTREE_PORTMIDI)
  add_subdirectory(portmidi)
endif()

# Subdirectories for Odamex projects
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(master)
add_subdirectory(odalaunch)

# Disable the ag-odalaunch target completely: -DNO_AG-ODALAUNCH_TARGET
# This is only really useful when setting up a universal build.
if(NOT NO_AG-ODALAUNCH_TARGET)
  add_subdirectory(ag-odalaunch)
endif()

# Packaging options.
# TODO: Integrate OSX stuff into here.
if(NOT APPLE)
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY Odamex)
  set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)

  set(CPACK_COMPONENTS_ALL client server odalaunch common)
  set(CPACK_COMPONENT_CLIENT_DEPENDS common)
  set(CPACK_COMPONENT_CLIENT_DISPLAY_NAME "Odamex")
  set(CPACK_COMPONENT_SERVER_DEPENDS common)
  set(CPACK_COMPONENT_SERVER_DISPLAY_NAME "Odamex Dedicated Server")
  set(CPACK_COMPONENT_ODALAUNCH_DEPENDS client)
  set(CPACK_COMPONENT_ODALAUNCH_DISPLAY_NAME "Odalaunch Odamex Server Browser and Launcher")
  set(CPACK_COMPONENT_COMMON_DISPLAY_NAME "Support files")

  # TODO: If deutex is available, use that to build the wad.
  file(GLOB CONFIG_SAMPLES config-samples/*.cfg)
  if(WIN32)
    install(FILES odamex.wad LICENSE README
      DESTINATION .
      COMPONENT common)
    install(FILES ${CONFIG_SAMPLES}
      DESTINATION config-samples
      COMPONENT common)

      # Windows ZIP packages are "tarbombs" by default.
      set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  else()
    install(FILES odamex.wad LICENSE README
      DESTINATION share/odamex
      COMPONENT common)
    install(FILES ${CONFIG_SAMPLES}
      DESTINATION share/odamex/config-samples
      COMPONENT common)

    option(ODAMEX_COMPONENT_PACKAGES "Create several rpm/deb packages for repository maintainers." OFF)
    if(ODAMEX_COMPONENT_PACKAGES)
      # TODO: Dependencies for components
      set(CPACK_RPM_COMPONENT_INSTALL YES)
      set(CPACK_DEB_COMPONENT_INSTALL YES)
    else()
      set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libsdl1.2debian, libsdl-mixer1.2, libwxbase2.8-0, libwxgtk2.8-0")
    endif()

    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A free, cross-platform modification of the Doom engine that allows players to easily join servers dedicated to playing Doom online.")
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alex Mayfield <alexmax2742@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION Games)
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS "boom-wad | doom-wad, libportmidi0")

  endif()
endif()

include(CPack)
