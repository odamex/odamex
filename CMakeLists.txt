
project(Odamex)
cmake_minimum_required(VERSION 2.8)

set(PROJECT_VERSION 0.6)
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
endif()

# Platform definitions
if(APPLE)
  add_definitions(-DOSX -DUNIX)
elseif(UNIX)
  add_definitions(-DUNIX)
elseif(WIN32)
  add_definitions(-DWIN32)
endif()

# Subdirectories for individual projects
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(master)
add_subdirectory(odalaunch)
