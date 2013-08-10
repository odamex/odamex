
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
macro(gloabl_compile_options)
  if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
    if(USE_STATIC_STDLIB)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
    if(USE_SANITIZE_ADDRESS)
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    endif()
  endif()
endmacro(gloabl_compile_options)

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
add_subdirectory(libminiupnpc)

# PortMidi
add_subdirectory(portmidi)

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
