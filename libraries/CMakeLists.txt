# Build libraries.
#
# Here we prefer use of ExternalProject to add_subdirectory.  ExternalProject
# builds the library exactly the way we want it without polluting the parent
# CMake cache.
#
# Some stuff to remember about ExternalProject
#
# - BUILD_BYPRODUCTS is necessary for Ninja to pick up on the dependency.
# - In custom build steps, be explicit about both DEPENDEES and DEPENDERS or
#   else the build step might not be executed after a clean.

# Shorthand, to reduce line-noise
set(libprefix "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(libsuffix "${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(dllprefix "${CMAKE_SHARED_LIBRARY_PREFIX}")
set(dllsuffix "${CMAKE_SHARED_LIBRARY_SUFFIX}")

include(ExternalProject)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/local/include")

# Figure out the correct library path to attach to our imported target
set(JSONCPP_LIBRARY
  "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}jsoncpp${libsuffix}")

# Build the library as an ExternalProject.
ExternalProject_Add(jsoncpp-project
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp"
  CMAKE_ARGS
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
  BUILD_BYPRODUCTS ${JSONCPP_LIBRARY}
  EXCLUDE_FROM_ALL ON)

# Synthesize an imported target that can be linked against.
add_library(jsoncpp STATIC IMPORTED GLOBAL)
add_dependencies(jsoncpp jsoncpp-project)
set_target_properties(jsoncpp PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/local/include"
  IMPORTED_LOCATION ${JSONCPP_LIBRARY})
