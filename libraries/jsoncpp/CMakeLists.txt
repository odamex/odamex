cmake_minimum_required(VERSION 3.1)
project(JSONCPP LANGUAGES CXX)

set(JSONCPP_SOURCES
  jsoncpp.cpp
  json/json.h
  json/json-forwards.h)

add_library(jsoncpp STATIC ${JSONCPP_SOURCES})
set_target_properties(jsoncpp PROPERTIES
  PUBLIC_HEADER "json/json.h;json/json-forwards.h")
target_compile_definitions(jsoncpp PUBLIC JSON_IS_AMALGAMATION)
target_include_directories(jsoncpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

include(GNUInstallDirs)
install(TARGETS jsoncpp
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/json")
