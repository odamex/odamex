file(GLOB COMMON_HEADERS *.h)
file(GLOB COMMON_SOURCES *.cpp)

configure_file(git_describe.h.in "${CMAKE_CURRENT_BINARY_DIR}/git_describe.h")

add_library(odamex-common INTERFACE)
target_sources(odamex-common INTERFACE ${COMMON_SOURCES} ${COMMON_HEADERS})
target_include_directories(odamex-common INTERFACE . ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
  include(CheckSymbolExists)
  check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)

  if(HAVE_BACKTRACE)
    target_compile_definitions(odamex-common INTERFACE HAVE_BACKTRACE)
  endif()
endif()
