# - Try to find LibAgar
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

set(LibAgar_FOUND false)

find_program(CMAKE_LibAgar_CONFIG agar-config)
if(CMAKE_LibAgar_CONFIG)

  set(LibAgar_FOUND true)
  execute_process(COMMAND ${CMAKE_LibAgar_CONFIG} --cflags OUTPUT_VARIABLE LibAgar_C_FLAGS)
  execute_process(COMMAND ${CMAKE_LibAgar_CONFIG} --libs OUTPUT_VARIABLE LibAgar_LIBRARIES)
  execute_process(COMMAND ${CMAKE_LibAgar_CONFIG} --version OUTPUT_VARIABLE LibAgar_VERSION)

  ## extract Include dirs (-I)
  ## use regular expression to match wildcard equivalent "-L*<endchar>"
  ## with <endchar> is a space or a semicolon
  string(REGEX MATCHALL "[-][I]([^ ;])+" CMAKE_LibAgar_INCLUDE_DIRS_WITH_PREFIX "${LibAgar_C_FLAGS}" )

  ## remove prefix -I because we need the pure directory for INCLUDE_DIRS
  ## replace -I by ; because the separator seems to be lost otherwise (bug or
  ## feature?)
  if(CMAKE_LibAgar_INCLUDE_DIRS_WITH_PREFIX)
  string(REGEX REPLACE "[-][I]" ";" LibAgar_INCLUDE_DIRS ${CMAKE_LibAgar_INCLUDE_DIRS_WITH_PREFIX} )
  string(STRIP "${LibAgar_INCLUDE_DIRS}" LibAgar_INCLUDE_DIRS)
  endif(CMAKE_LibAgar_INCLUDE_DIRS_WITH_PREFIX)
 
  ## extract Definitions (-D)
  ## use regular expression to match wildcard equivalent "-D*<endchar>"
  ## with <endchar> is a space or a semicolon
  string(REGEX MATCHALL "[-][D]([^ ;])+" LibAgar_DEFINITIONS "${LibAgar_C_FLAGS}" )

  if(LibAgar_DEFINITIONS)
  string(STRIP "${LibAgar_DEFINITIONS}" LibAgar_DEFINITIONS)
  endif(LibAgar_DEFINITIONS)

  ## extract linkdirs (-L) for rpath
  ## use regular expression to match wildcard equivalent "-L*<endchar>"
  ## with <endchar> is a space or a semicolon
  string(REGEX MATCHALL "[-][L]([^ ;])+" CMAKE_LibAgar_LIBRARY_DIRS_WITH_PREFIX "${LibAgar_LIBRARIES}" )
 
  ## remove prefix -L because we need the pure directory for LIBRARY_DIRS
  ## replace -L by ; because the separator seems to be lost otherwise (bug or
  ## feature?)
  if(CMAKE_LibAgar_LIBRARY_DIRS_WITH_PREFIX)
  string(REGEX REPLACE "[-][L]" ";" LibAgar_LIBRARY_DIRS ${CMAKE_LibAgar_LIBRARY_DIRS_WITH_PREFIX} )
  string(STRIP "${LibAgar_LIBRARY_DIRS}" LibAgar_LIBRARY_DIRS)
  endif(CMAKE_LibAgar_LIBRARY_DIRS_WITH_PREFIX)
 
  ## replace space separated string by semicolon separated vector
  separate_arguments(LibAgar_LIBRARIES)
  string(REPLACE "-framework;" "-framework " LibAgar_LIBRARIES "${LibAgar_LIBRARIES}")
  string(STRIP "${LibAgar_LIBRARIES}" LibAgar_LIBRARIES)
endif()
