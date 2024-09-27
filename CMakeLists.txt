# CMake 3.13 needed for -S and -B params in library compilation.
#
# Note that if you are running Linux, there are many ways to get newer
# versions of CMake.
# 
# - Kitware offers binary downloads direct from their website, which you can
#   extract to /usr/local or ~/.local.
# - Ubuntu LTS users can install the CMake snap, and Kitware also runs an
#   official Ubuntu CMake apt repository.
# - Debian users can get a new version through backports.
# - CentOS users can get a new version through EPEL.
# - If you have Python installed, you can install CMake through pip.
#

cmake_minimum_required(VERSION 3.13)

project(Odamex VERSION 10.6.0)

include(CMakeDependentOption)

# CMAKE_INSTALL_BINDIR and CMAKE_INSTALL_DATADIR will be changed if GNUInstallDirs is availible
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_DATADIR "share")
include(GNUInstallDirs OPTIONAL)

add_definitions(-DINSTALL_BINDIR="${CMAKE_INSTALL_BINDIR}")
add_definitions(-DINSTALL_DATADIR="${CMAKE_INSTALL_DATADIR}")

# Set up FHS installation path
if(NOT APPLE AND NOT WIN32)
  add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

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
option(USE_INTERNAL_DEUTEX "Use internal DeuTex" ${USE_INTERNAL_LIBS})
option(USE_LTO "Build Release builds with Link Time Optimization" 1)
cmake_dependent_option( USE_INTERNAL_ZLIB "Use internal zlib" ${USE_INTERNAL_LIBS} BUILD_CLIENT 0 )
cmake_dependent_option( USE_INTERNAL_PNG "Use internal libpng" ${USE_INTERNAL_LIBS} BUILD_CLIENT 0 )
cmake_dependent_option( USE_INTERNAL_CURL "Use internal libcurl" ${USE_INTERNAL_LIBS} BUILD_CLIENT 0 )
cmake_dependent_option( USE_INTERNAL_JSONCPP "Use internal JsonCpp" 1 BUILD_SERVER 0 )
cmake_dependent_option( USE_INTERNAL_WXWIDGETS "Use internal wxWidgets" ${USE_INTERNAL_LIBS} BUILD_LAUNCHER 0 )
cmake_dependent_option( ENABLE_PORTMIDI "Enable portmidi support" 1 BUILD_CLIENT 0 )
cmake_dependent_option( USE_MINIUPNP "Build with UPnP support" 1 BUILD_SERVER 0 )
cmake_dependent_option( USE_INTERNAL_MINIUPNP "Use internal MiniUPnP" 1 USE_MINIUPNP 0 )

set(PROJECT_COPYRIGHT "2006-2024")
set(PROJECT_RC_VERSION "10,6,0,0")
set(PROJECT_COMPANY "The Odamex Team")

# Include early required commands for specific systems
if (NSWITCH)
	include("switch.cmake" REQUIRED)  # Nintendo Switch
elseif(VWII)
  include("wii.cmake" REQUIRED)     # Wii/vWii
endif()

# Ensure that we can use folders in projects.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# identify the target CPU
# adapted from the FindJNI.cmake module included with the CMake distribution
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(ODAMEX_TARGET_ARCH "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-9]86$")
  set(ODAMEX_TARGET_ARCH "i386")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^alpha")
  set(ODAMEX_TARGET_ARCH "alpha")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  set(ODAMEX_TARGET_ARCH "arm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
  set(ODAMEX_TARGET_ARCH "ppc64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")
  set(ODAMEX_TARGET_ARCH "ppc")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^sparc")
  # Both flavors can run on the same processor
  set(ODAMEX_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}" "sparc" "sparcv9")
else()
  set(ODAMEX_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

list(REMOVE_DUPLICATES ODAMEX_TARGET_ARCH)
message(STATUS "Target architecture: ${ODAMEX_TARGET_ARCH}")

# Default build type
if(NOT CMAKE_CONFIGURATION_TYPES)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()
  message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
  set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif()

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  # Export compile commands unless we're generating a modern project.
  if(CMAKE_GENERATOR MATCHES "Make" OR CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  else()
    set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
  endif()
endif()
message(STATUS "Export Compile Commands: ${CMAKE_EXPORT_COMPILE_COMMANDS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS "${CMAKE_EXPORT_COMPILE_COMMANDS}" CACHE BOOL
  "Export compile commands for use in supported editors."
  FORCE)

# Global compile options as shown in a GUI.
if(NOT MSVC)
  set(USE_COLOR_DIAGNOSTICS ON CACHE BOOL
    "Force the use of color diagnostics, necessary to get color output with Ninja.")
  set(USE_STATIC_STDLIB OFF CACHE BOOL
    "Statically link against the C and C++ Standard Library.")
  set(USE_SANITIZE_ADDRESS OFF CACHE BOOL
    "Turn on Address Sanitizer in Debug builds, requires GCC >= 4.8 or Clang >= 3.1")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES SunOS )
  set(SOLARIS 1)
endif()

if(USE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT HAS_LTO OUTPUT HAS_LTO_ERROR)
  if(HAS_LTO)
    message(STATUS "Link Time Optimization: ON")
  else()
    message(STATUS "Link Time Optimization: OFF")
  endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# git describe
#
# Grabs the git hash and branch a few different ways.
include(GetGitBranch)
include(GetGitRevisionDescription)
include(GetGitRevisionNumber)
get_git_head_revision(HEAD GIT_HASH)
git_describe(GIT_DESCRIBE --all --long --abbrev=4)
git_rev_count(HEAD GIT_REV_COUNT)
git_branch(HEAD GIT_BRANCH)
string(REGEX REPLACE "^(heads\/|tags\/)(.+)(-0-g)([0-9a-f]+)"
  "\\4" GIT_SHORT_HASH "${GIT_DESCRIBE}")

# Libraries
add_subdirectory(libraries)

# WAD building
add_subdirectory(wad)

# Subdirectories for Odamex projects
if(BUILD_CLIENT OR BUILD_SERVER)
  add_subdirectory(common)
  add_subdirectory(odaproto)
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
if(NOT BUILD_CLIENT AND NOT BUILD_SERVER AND NOT BUILD_MASTER AND NOT BUILD_LAUNCHER)
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
