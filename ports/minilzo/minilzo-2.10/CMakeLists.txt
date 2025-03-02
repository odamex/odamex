#
# (C) 2025 Lexi Mayfield
#
# This is a bare-minimum CMake file that builds this library in-tree and
# then exports config files that the parent build can pick up on using
# find_package(CONFIG).
#

cmake_minimum_required(VERSION 3.23)

include(CMakePackageConfigHelpers)

project(minilzo VERSION 2.10 LANGUAGES C)

add_library(minilzo STATIC)

target_sources(minilzo
    PRIVATE
        minilzo.c
    PUBLIC FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}" FILES
        lzoconf.h
        lzodefs.h
        minilzo.h)

target_include_directories(minilzo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(minilzo INTERFACE "$<INSTALL_INTERFACE:include>")

install(TARGETS minilzo EXPORT minilzoTargets FILE_SET HEADERS)

# Target
set(PACKAGE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/minilzo")

export(
    EXPORT minilzoTargets
    FILE "${CMAKE_BINARY_DIR}/minilzoTargets.cmake"
    NAMESPACE minilzo::)

install(
    EXPORT minilzoTargets
    FILE minilzoTargets.cmake
    NAMESPACE minilzo::
    DESTINATION "${PACKAGE_DIR}")

# Config file
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/minilzoConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/minilzoConfig.cmake"
    INSTALL_DESTINATION "${PACKAGE_DIR}")

# Version file
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/minilzoVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion)

install(
    FILES
        "${CMAKE_BINARY_DIR}/minilzoConfig.cmake"
        "${CMAKE_BINARY_DIR}/minilzoVersion.cmake"
    DESTINATION
        "${PACKAGE_DIR}")
