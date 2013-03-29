
project(Odamex)
cmake_minimum_required(VERSION 2.8)

set(PROJECT_VERSION 0.6.2)
set(PROJECT_COPYRIGHT "2006-2012")

# Default build type
if(NOT MSVC)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
  endif()
endif()

# Build type definitions
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")

  set(SANITIZE_ADDRESS "No" CACHE STRING
    "Turn on Address Sanitizer in Debug builds, requires GCC >= 4.8 or Clang >= 3.1")
  set_property(CACHE SANITIZE_ADDRESS PROPERTY STRINGS Yes No)
  if(SANITIZE_ADDRESS)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
  endif()
endif()

if( ${CMAKE_SYSTEM_NAME} MATCHES SunOS )
  set(SOLARIS 1)
endif()

macro(define_platform)
  # Platform definitions
  if(APPLE)
    add_definitions(-DOSX -DUNIX)
  elseif(SOLARIS)
    add_definitions(-DSOLARIS -DUNIX -DBSD_COMP -gstabs+)
  elseif(UNIX)
    add_definitions(-DUNIX)
  elseif(WIN32)
    add_definitions(-DWIN32)
  endif()
endmacro(define_platform)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Subdirectories for individual projects
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(master)
add_subdirectory(odalaunch)

# Disable the ag-odalaunch target completely: -DNO_AG-ODALAUNCH_TARGET
# This is only really useful when setting up a universal build.
if(NOT NO_AG-ODALAUNCH_TARGET)
  add_subdirectory(ag-odalaunch)
endif()
