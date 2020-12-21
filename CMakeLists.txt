cmake_minimum_required(VERSION 3.1)

include(CMakeDependentOption)

# CMAKE_INSTALL_BINDIR and CMAKE_INSTALL_DATADIR will be changed if GNUInstallDirs is availible
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_DATADIR "share")
include(GNUInstallDirs OPTIONAL)

add_definitions(-DINSTALL_BINDIR="${CMAKE_INSTALL_BINDIR}")
add_definitions(-DINSTALL_DATADIR="${CMAKE_INSTALL_DATADIR}")

if(WIN32)
  set(USE_INTERNAL_LIBS 1)
else()
  set(USE_INTERNAL_LIBS 0)
endif()

# options
option(BUILD_CLIENT "Build client target" 1)
option(BUILD_SERVER "Build server target" 1)
option(BUILD_LAUNCHER "Build launcher target" 1)
option(BUILD_MASTER "Build master server target" 0)
option(BUILD_OR_FAIL "Must build the BUILD_* targets or else generation will fail" 0)
cmake_dependent_option( USE_INTERNAL_ZLIB "Use internal zlib" ${USE_INTERNAL_LIBS} BUILD_CLIENT 0 )
cmake_dependent_option( USE_INTERNAL_PNG "Use internal libpng" ${USE_INTERNAL_LIBS} BUILD_CLIENT 0 )
cmake_dependent_option( USE_INTERNAL_CURL "Use internal libcurl" ${USE_INTERNAL_LIBS} BUILD_CLIENT 0 )
cmake_dependent_option( USE_INTERNAL_WXWIDGETS "Use internal wxWidgets" ${USE_INTERNAL_LIBS} BUILD_LAUNCHER 0 )
cmake_dependent_option( ENABLE_PORTMIDI "Enable portmidi support" 1 BUILD_CLIENT 0 )
cmake_dependent_option( USE_MINIUPNP "Build with UPnP support" 1 BUILD_SERVER 0 )

project(Odamex VERSION 0.9.0)

set(PROJECT_COPYRIGHT "2006-2020")
set(PROJECT_RC_VERSION "0,9,0,0")
set(PROJECT_COMPANY "The Odamex Team")

# Use C++ 98/03 for all targets
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure that we can use folders in projects.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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
  set(USE_COLOR_DIAGNOSTICS "No" CACHE BOOL
    "Force the use of color diagnostics, necessary to get color output with Ninja.")
  set(USE_STATIC_STDLIB "No" CACHE BOOL
    "Statically link against the C and C++ Standard Library.")
  set(USE_SANITIZE_ADDRESS "No" CACHE BOOL
    "Turn on Address Sanitizer in Debug builds, requires GCC >= 4.8 or Clang >= 3.1")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES SunOS )
  set(SOLARIS 1)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# git describe
#
# Grabs the git hash and branch a few different ways.
include(GetGitRevisionDescription)
include(GetGitRevisionNumber)
get_git_head_revision(HEAD GIT_HASH)
git_describe(GIT_DESCRIBE --all --long --abbrev=4)
git_rev_count(HEAD GIT_REV_COUNT)
string(REGEX REPLACE "^(heads\/|tags\/)(.+)(-0-g)([0-9a-f]+)"
  "\\2" GIT_BRANCH "${GIT_DESCRIBE}")
string(REGEX REPLACE "^(heads\/|tags\/)(.+)(-0-g)([0-9a-f]+)"
  "\\4" GIT_SHORT_HASH "${GIT_DESCRIBE}")

# Libraries
add_subdirectory(libraries)

# Subdirectories for Odamex projects
if(BUILD_CLIENT OR BUILD_SERVER)
  add_subdirectory(common)
endif()
if(BUILD_CLIENT)
  add_subdirectory(client)
endif()
if(BUILD_SERVER)
  add_subdirectory(server)
endif()
if(BUILD_MASTER)
  add_subdirectory(master)
endif()
if(BUILD_LAUNCHER)
  add_subdirectory(odalaunch)
endif()
if(NOT BUILD_CLIENT AND NOT BUILD_SERVER AND NOT BUILD_MASTER AND NOT BUILD_ODALAUNCH)
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

  add_subdirectory(wad)

  file(GLOB CONFIG_SAMPLES config-samples/*.cfg)
  if(WIN32)
    install(FILES LICENSE README
      DESTINATION .
      COMPONENT common)
    install(FILES ${CONFIG_SAMPLES}
      DESTINATION config-samples
      COMPONENT common)

    # Windows ZIP packages are "tarbombs" by default.
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  else()
    install(FILES LICENSE README
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
    set(CPACK_PACKAGE_VENDOR "Odamex Development Team")
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

    set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")

    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://odamex.net")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alex Mayfield <alexmax2742@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION Games)
  endif()
endif()

include(CPack)
