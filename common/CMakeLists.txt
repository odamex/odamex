file(GLOB COMMON_HEADERS *.h)
file(GLOB COMMON_SOURCES *.cpp)

configure_file(git_describe.h.in "${CMAKE_CURRENT_BINARY_DIR}/git_describe.h")

if (UNIX)
  include(CheckSymbolExists)
  check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)
endif()

add_library(odacommon INTERFACE)
target_sources(odacommon PUBLIC ${COMMON_SOURCES} ${COMMON_HEADERS} "${CMAKE_CURRENT_BINARY_DIR}/git_describe.h")

target_include_directories(odacommon INTERFACE . ${CMAKE_CURRENT_BINARY_DIR})
if (HAVE_BACKTRACE)
    target_compile_definitions(odacommon INTERFACE HAVE_BACKTRACE)
endif()

target_link_libraries(odacommon INTERFACE odaproto fmt::fmt tinylibs ZLIB::ZLIB minilzo cpptrace::cpptrace)

if (WIN32)
    target_link_libraries(odacommon INTERFACE winmm wsock32 shlwapi)
elseif (SOLARIS)
    target_link_libraries(odacommon INTERFACE socket nsl)
endif()
if (UNIX AND NOT APPLE)
    target_link_libraries(odacommon INTERFACE rt)
endif()

if(USE_INTERNAL_JSONCPP)
    target_link_libraries(odacommon INTERFACE jsoncpp_static)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP jsoncpp REQUIRED IMPORTED_TARGET)
    target_link_libraries(odacommon INTERFACE PkgConfig::JSONCPP)
endif()
