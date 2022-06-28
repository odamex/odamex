# Build libraries.
#
# Here we use execute_process and file(DOWNLOAD) to download and compile
# libraries as we need them.  Some stuff to remember:
#
# - C libraries can cross Debug/Release boundaries, but C++ libraries can't.
#   You'll need to be able to select the proper debug vs release library in
#   multi-config builds - see JsonCpp for an example.
# - Xcode does not like it when CMake passes --parallel with no parameter.
#

# Parallel processor count - needed by Xcode.
include(ProcessorCount)
ProcessorCount(PARALLEL_PROC_COUNT)

# Set our prefix path, so inter-dependencies are picked up on.
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/local")

# Shorthand, to reduce line-noise
set(libprefix "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(libsuffix "${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(dllprefix "${CMAKE_SHARED_LIBRARY_PREFIX}")
set(dllsuffix "${CMAKE_SHARED_LIBRARY_SUFFIX}")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/local/include")

function(lib_buildgen)
  cmake_parse_arguments("" "" "LIBRARY;SRCDIR" "PARAMS" ${ARGN})

  if(NOT _SRCDIR)
    set(_SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/${_LIBRARY}")
  endif()

  # Baseline library generation.
  set(_COMMAND "${CMAKE_COMMAND}"
    -S "${_SRCDIR}"
    -B "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
    -G "${CMAKE_GENERATOR}"
    "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
    "-DCMAKE_INSTALL_LIBDIR=lib") # Without this, CentOS installs into lib64

  if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND _COMMAND -A)
    list(APPEND _COMMAND ${CMAKE_GENERATOR_PLATFORM})
  endif()

  if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND _COMMAND -T)
    list(APPEND _COMMAND ${CMAKE_GENERATOR_TOOLSET})
  endif()

  if(CMAKE_C_COMPILER)
    list(APPEND _COMMAND "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
  endif()

  if(CMAKE_CXX_COMPILER)
    list(APPEND _COMMAND "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
  endif()

  if(CMAKE_LINKER)
    list(APPEND _COMMAND "-DCMAKE_LINKER=${CMAKE_LINKER}")
  endif()

  if(CMAKE_RC_COMPILER)
    list(APPEND _COMMAND "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
  endif()

  get_property(_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT _IS_MULTI_CONFIG)
    list(APPEND _COMMAND "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
  endif()

  execute_process(COMMAND ${_COMMAND} ${_PARAMS} RESULT_VARIABLE _RESVAL)
  if(_RESVAL GREATER 0)
    message(FATAL_ERROR "Library ${_LIBRARY} did not generate correctly.")
  endif()
endfunction()

function(lib_build)
  cmake_parse_arguments("" "" "LIBRARY" "" ${ARGN})

  # Compile the library.
  get_property(_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(_IS_MULTI_CONFIG)
    # Debug and Release are not compatible, so build both.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
      --config Debug --target install
      --parallel ${PARALLEL_PROC_COUNT} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library ${_LIBRARY} did not build correctly as Debug.")
    endif()

    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
      --config RelWithDebInfo --target install
      --parallel ${PARALLEL_PROC_COUNT} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library ${_LIBRARY} did not build correctly as Release.")
    endif()  
  else()
    # Single config uses the passed build type.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
      --config ${CMAKE_BUILD_TYPE} --target install
      --parallel ${PARALLEL_PROC_COUNT} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library ${_LIBRARY} did not build correctly.")
    endif()  
  endif()
endfunction()

### tiny libraries ###

add_library(tinylibs INTERFACE)
target_include_directories(tinylibs INTERFACE . "tinylibs/include")

include(zlib-lib.cmake)

include(libpng-lib.cmake)

include(curl-lib.cmake)

include(fltk-lib.cmake)

include(fmt-lib.cmake)

include(jsoncpp-lib.cmake)

include(miniupnp-lib.cmake)

include(protobuf-lib.cmake)

include(SDL-lib.cmake)

include(wxWidgets-lib.cmake)
