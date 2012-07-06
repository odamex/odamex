# - Try to find LibAgar
# This module finds if libAgar is installed and selects a default
# configuration to use. libAgar is a modular library. To specify the
# modules that you will use, you need to name them as components to
# the package:
# 
# FIND_PACKAGE(LibAgar COMPONENTS core gui ...)
#
# The default configuration includes the gui module.
# Valid modules are: core, gui, math, dev, vg, rg
#
# Once done, this will define
#
#  LibAgar_FOUND - system has LibAgar
#  LibAgar_INCLUDE_DIRS - the LibAgar include directories
#  LibAgar_DEFINITIONS - Contains defines required to compile/link
#  LibAgar_LIBRARY_DIRS - the LibAgar library directories
#  LibAgar_LIBRARIES - link these to use LibAgar
#  LibAgar_VERSION - detected version of LibAgar
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

unset(LibAgar_FOUND)
unset(LibAgar_INCLUDE_DIRS)
unset(LibAgar_DEFINITIONS)
unset(LibAgar_LIBRARY_DIRS)
unset(LibAgar_LIBRARIES)
unset(LibAgar_VERSION)

set(LibAgar_FOUND false)

find_program(CMAKE_LibAgar_CONFIG agar-config)
if(CMAKE_LibAgar_CONFIG)

  set(LibAgar_FOUND true)

  if(NOT LibAgar_FIND_COMPONENTS)
    set(LibAgar_FIND_COMPONENTS gui) # default
  endif()

  foreach(COMPONENT ${LibAgar_FIND_COMPONENTS})
    if(${COMPONENT} STREQUAL "gui")
      set(${COMPONENT}_CONFIG ${CMAKE_LibAgar_CONFIG})
    else()
      find_program(${COMPONENT}_CONFIG agar-${COMPONENT}-config)
    endif()

    if(${COMPONENT}_CONFIG)
      execute_process(COMMAND ${${COMPONENT}_CONFIG} --cflags OUTPUT_VARIABLE CMAKE_LibAgar_${COMPONENT}_C_FLAGS)
      execute_process(COMMAND ${${COMPONENT}_CONFIG} --libs OUTPUT_VARIABLE CMAKE_LibAgar_${COMPONENT}_LIBRARIES)

      string(REPLACE "\n" " " CMAKE_LibAgar_${COMPONENT}_C_FLAGS "${CMAKE_LibAgar_${COMPONENT}_C_FLAGS}")
      set(LibAgar_C_FLAGS "${LibAgar_C_FLAGS} ${CMAKE_LibAgar_${COMPONENT}_C_FLAGS}")

      string(REPLACE "\n" " " CMAKE_LibAgar_${COMPONENT}_LIBRARIES "${CMAKE_LibAgar_${COMPONENT}_LIBRARIES}")
      set(LibAgar_LIBRARIES "${LibAgar_LIBRARIES} ${CMAKE_LibAgar_${COMPONENT}_LIBRARIES}")

      ## extract Include dirs (-I)
      ## use regular expression to match wildcard equivalent "-L*<endchar>"
      ## with <endchar> is a space or a semicolon
      string(REGEX MATCHALL "[-][I]([^ ;])+" CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS_WITH_PREFIX "${CMAKE_LibAgar_${COMPONENT}_C_FLAGS}" )

      ## remove prefix -I because we need the pure directory for INCLUDE_DIRS
      ## replace -I by ; because the separator seems to be lost otherwise (bug or
      ## feature?)
      if(CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS_WITH_PREFIX)
      string(REGEX REPLACE "[-][I]" ";" CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS ${CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS_WITH_PREFIX} )
      string(STRIP "${CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS}" CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS)
      set(LibAgar_INCLUDE_DIRS ${LibAgar_INCLUDE_DIRS} ${CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS})
      endif(CMAKE_LibAgar_${COMPONENT}_INCLUDE_DIRS_WITH_PREFIX)
 
      ## extract Definitions (-D)
      ## use regular expression to match wildcard equivalent "-D*<endchar>"
      ## with <endchar> is a space or a semicolon
      string(REGEX MATCHALL "[-][D]([^ ;])+" CMAKE_LibAgar_${COMPONENT}_DEFINITIONS "${CMAKE_LibAgar_${COMPONENT}_C_FLAGS}" )

      if(CMAKE_LibAgar_${COMPONENT}_DEFINITIONS)
      string(STRIP "${CMAKE_LibAgar_${COMPONENT}_DEFINITIONS}" CMAKE_LibAgar_${COMPONENT}_DEFINITIONS)
      set(LibAgar_DEFINITIONS ${LibAgar_DEFINITIONS} ${CMAKE_LibAgar_${COMPONENT}_DEFINITIONS})
      endif(CMAKE_LibAgar_${COMPONENT}_DEFINITIONS)

      ## extract linkdirs (-L) for rpath
      ## use regular expression to match wildcard equivalent "-L*<endchar>"
      ## with <endchar> is a space or a semicolon
      string(REGEX MATCHALL "[-][L]([^ ;])+" CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS_WITH_PREFIX "${CMAKE_LibAgar_${COMPONENT}_LIBRARIES}" )
 
      ## remove prefix -L because we need the pure directory for LIBRARY_DIRS
      ## replace -L by ; because the separator seems to be lost otherwise (bug or
      ## feature?)
      if(CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS_WITH_PREFIX)
      string(REGEX REPLACE "[-][L]" ";" CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS ${CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS_WITH_PREFIX} )
      string(STRIP "${CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS}" CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS)
      set(LibAgar_LIBRARY_DIRS ${LibAgar_LIBRARY_DIRS} ${CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS})
      endif(CMAKE_LibAgar_${COMPONENT}_LIBRARY_DIRS_WITH_PREFIX)

    endif()
  endforeach()

  execute_process(COMMAND ${CMAKE_LibAgar_CONFIG} --version OUTPUT_VARIABLE LibAgar_VERSION)

  ## replace space separated string by semicolon separated vector
  separate_arguments(LibAgar_LIBRARIES)
  string(REPLACE "-framework;" "-framework " LibAgar_LIBRARIES "${LibAgar_LIBRARIES}")
  string(STRIP "${LibAgar_LIBRARIES}" LibAgar_LIBRARIES)

  if(LibAgar_C_FLAGS)
    list(REMOVE_DUPLICATES LibAgar_C_FLAGS)
  endif()
  if(LibAgar_LIBRARIES)
    list(REMOVE_DUPLICATES LibAgar_LIBRARIES)
  endif()
  if(LibAgar_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES LibAgar_INCLUDE_DIRS)
  endif()
  if(LibAgar_LIBRARY_DIRS)
    list(REMOVE_DUPLICATES LibAgar_LIBRARY_DIRS)
  endif()
  if(LibAgar_DEFINITIONS)
    list(REMOVE_DUPLICATES LibAgar_DEFINITIONS)
  endif()

endif()
