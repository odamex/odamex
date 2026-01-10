### JsonCpp ###

if((BUILD_CLIENT OR BUILD_SERVER) AND USE_INTERNAL_JSONCPP)
  lib_buildgen(
    LIBRARY jsoncpp
    PARAMS "-DJSONCPP_WITH_TESTS=OFF"
           "-DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF"
           "-DJSONCPP_WITH_WARNING_AS_ERROR=OFF"
           "-DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF"
           "-DJSONCPP_WITH_CMAKE_PACKAGE=ON"
           "-DCMAKE_DEBUG_POSTFIX=d"
           "-DCCACHE_EXECUTABLE=CCACHE_EXECUTABLE-NOTFOUND")
  lib_build(LIBRARY jsoncpp)

  find_package(jsoncpp REQUIRED)
  set(_JSONCPP_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/local/include/jsoncpp")
  if(NOT EXISTS "${_JSONCPP_INCLUDE_DIR}")
    set(_JSONCPP_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/local/include")
  endif()
  if(TARGET jsoncpp_static)
    set_target_properties(jsoncpp_static PROPERTIES IMPORTED_GLOBAL TRUE)
    set_property(TARGET jsoncpp_static APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES "${_JSONCPP_INCLUDE_DIR}")
  elseif(TARGET jsoncpp_lib_static)
    set_target_properties(jsoncpp_lib_static PROPERTIES IMPORTED_GLOBAL TRUE)
    set_property(TARGET jsoncpp_lib_static APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES "${_JSONCPP_INCLUDE_DIR}")
    add_library(jsoncpp_static INTERFACE IMPORTED GLOBAL)
    set_target_properties(jsoncpp_static PROPERTIES
      INTERFACE_LINK_LIBRARIES jsoncpp_lib_static
      INTERFACE_INCLUDE_DIRECTORIES "${_JSONCPP_INCLUDE_DIR}")
  elseif(TARGET jsoncpp_lib)
    set_target_properties(jsoncpp_lib PROPERTIES IMPORTED_GLOBAL TRUE)
    set_property(TARGET jsoncpp_lib APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES "${_JSONCPP_INCLUDE_DIR}")
    add_library(jsoncpp_static INTERFACE IMPORTED GLOBAL)
    set_target_properties(jsoncpp_static PROPERTIES
      INTERFACE_LINK_LIBRARIES jsoncpp_lib
      INTERFACE_INCLUDE_DIRECTORIES "${_JSONCPP_INCLUDE_DIR}")
  else()
    message(FATAL_ERROR "JsonCpp target not found.")
  endif()
endif()
