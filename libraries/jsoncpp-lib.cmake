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
  if(TARGET jsoncpp_static)
    set_target_properties(jsoncpp_static PROPERTIES IMPORTED_GLOBAL TRUE)
  elseif(TARGET jsoncpp_lib_static)
    set_target_properties(jsoncpp_lib_static PROPERTIES IMPORTED_GLOBAL TRUE)
  elseif(TARGET jsoncpp_lib)
    set_target_properties(jsoncpp_lib PROPERTIES IMPORTED_GLOBAL TRUE)
  else()
    message(FATAL_ERROR "JsonCpp target not found.")
  endif()
endif()
