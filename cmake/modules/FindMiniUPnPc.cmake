# - Try to find MiniUPnPc
# Once done, this will define
#
#  MINIUPNPC_FOUND - system has MiniUPnPc
#  MINIUPNPC_INCLUDE_DIRS - the MiniUPnPc include directory
#  MINIUPNPC_LIBRARIES - link these to use MiniUPnPc
#  MINIUPNPC_STATIC_LIBRARIES - static libraries
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

find_path(MINIUPNPC_INCLUDE_DIR miniupnpc.h
  HINTS $ENV{MINIUPNPC_INCLUDE_DIR}
  PATH_SUFFIXES miniupnpc
)

find_library(MINIUPNPC_LIBRARY miniupnpc
  HINTS $ENV{MINIUPNPC_LIBRARY}
)

find_library(MINIUPNPC_STATIC_LIBRARY libminiupnpc.a
  HINTS $ENV{MINIUPNPC_STATIC_LIBRARY}
)

set(MINIUPNPC_INCLUDE_DIRS ${MINIUPNPC_INCLUDE_DIR})
set(MINIUPNPC_LIBRARIES ${MINIUPNPC_LIBRARY})
set(MINIUPNPC_STATIC_LIBRARIES ${MINIUPNPC_STATIC_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  MiniUPnPc DEFAULT_MSG
  MINIUPNPC_INCLUDE_DIR
  MINIUPNPC_LIBRARY
)

mark_as_advanced(MINIUPNPC_INCLUDE_DIR MINIUPNPC_LIBRARY MINIUPNPC_STATIC_LIBRARY)
