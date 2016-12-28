# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

include(CMakeDependentOption)

# CMAKE_INSTALL_BINDIR and CMAKE_INSTALL_DATADIR will be changed if GNUInstallDirs is availible
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_DATADIR "share")
include(GNUInstallDirs OPTIONAL)

add_definitions(-DINSTALL_BINDIR="${CMAKE_INSTALL_BINDIR}")
add_definitions(-DINSTALL_DATADIR="${CMAKE_INSTALL_DATADIR}")

# options
option(BUILD_CLIENT "Build client target" 1)
option(BUILD_SERVER "Build server target" 1)
option(BUILD_MASTER "Build master server target" 1)
cmake_dependent_option( BUILD_ODALAUNCH "Build odalaunch target" 1 BUILD_CLIENT 0 )
cmake_dependent_option( ENABLE_PORTMIDI "Enable portmidi support" 1 BUILD_CLIENT 0 )
cmake_dependent_option( USE_MINIUPNP "Build with UPnP support" 1 BUILD_SERVER 0 )

project(Odamex)
cmake_minimum_required(VERSION 2.8)

set(PROJECT_VERSION 0.7.X)
set(PROJECT_COPYRIGHT "2006-2016")

# identify the target CPU
# adapted from the FindJNI.cmake module included with the CMake distribution
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(target_arch "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-9]86$")
  set(target_arch "i386")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^alpha")
  set(target_arch "alpha")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  set(target_arch "arm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
  set(target_arch "ppc64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")
  set(target_arch "ppc")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^sparc")
  # Both flavors can run on the same processor
  set(target_arch "${CMAKE_SYSTEM_PROCESSOR}" "sparc" "sparcv9")
else()
  set(target_arch "${CMAKE_SYSTEM_PROCESSOR}")
endif()

list(REMOVE_DUPLICATES target_arch)
message("Using target architecture " ${target_arch})

# Default build type
if(NOT MSVC)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
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
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      add_definitions(-DODAMEX_DEBUG)
	endif()
  endif()
endmacro(global_compile_options)

if(${CMAKE_SYSTEM_NAME} MATCHES SunOS )
  set(SOLARIS 1)
endif()

# Platform definitions
# [AM] TODO: Eliminate redundant defines
macro(define_platform)
  if(APPLE)
    add_definitions(-DOSX -DUNIX )
  elseif(SOLARIS)
    add_definitions(-DSOLARIS -DUNIX -DBSD_COMP -gstabs+)
  elseif(UNIX)
    add_definitions(-DUNIX)
    find_package(X11)
    if(X11_FOUND)
      add_definitions(-DX11)
      INCLUDE_DIRECTORIES ( ${X11_INCLUDE_DIR} )
      LINK_LIBRARIES ( ${X11_LIBRARIES} )
      MESSAGE ( STATUS " X11_INCLUDE_DIR: " ${X11_INCLUDE_DIR} )
      MESSAGE ( STATUS " X11_LIBRARIES: " ${X11_LIBRARIES} )
    endif()
  endif()
endmacro(define_platform)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# git describe
include(GetGitRevisionDescription)
git_describe(GIT_DESCRIBE --tags --always)

# MiniUPnPc
if (USE_MINIUPNP)
  set(UPNPC_BUILD_STATIC ON CACHE INTERNAL "" FORCE)
  set(UPNPC_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)
  set(UPNPC_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
  set(UPNPC_INSTALL OFF CACHE INTERNAL "" FORCE)
  mark_as_advanced(FORCE UPNPC_INSTALL)
  add_subdirectory(libraries/libminiupnpc)
endif()

# Subdirectories for Odamex projects
if(BUILD_CLIENT)
	add_subdirectory(client)
endif()
if(BUILD_SERVER)
	add_subdirectory(server)
endif()
if(BUILD_MASTER)
	add_subdirectory(master)
endif()
if(BUILD_ODALAUNCH)
	add_subdirectory(odalaunch)
endif()
if(NOT BUILD_CLIENT AND NOT BUILD_SERVER AND NOT BUILD_MASTER)
	message(FATAL_ERROR "No target chosen, doing nothing.")
endif()

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
      DESTINATION ${CMAKE_INSTALL_DATADIR}/odamex
      COMPONENT common)
    install(FILES ${CONFIG_SAMPLES}
      DESTINATION ${CMAKE_INSTALL_DATADIR}/odamex/config-samples
      COMPONENT common)

    option(ODAMEX_COMPONENT_PACKAGES "Create several rpm/deb packages for repository maintainers." OFF)
    if(ODAMEX_COMPONENT_PACKAGES)
      set(CPACK_RPM_COMPONENT_INSTALL YES)
      # TODO: RPM Dependencies

      set(CPACK_DEB_COMPONENT_INSTALL YES)
      # TODO: DEB Dependencies
    else()
      # TODO: RPM Dependencies

      set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libsdl1.2debian, libsdl-mixer1.2, libwxbase2.8-0, libwxgtk2.8-0")
      set(CPACK_DEBIAN_PACKAGE_SUGGESTS "boom-wad | doom-wad, libportmidi0")
    endif()

    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A free, cross-platform modification of the Doom engine that allows players to easily join servers dedicated to playing Doom online.")
    set(CPACK_PACKAGE_VENDOR "Odamex Team")
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

    set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")

    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://odamex.net")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alex Mayfield <alexmax2742@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION Games)
  endif()
endif()

include(CPack)
