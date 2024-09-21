### JsonCpp ###

if(BUILD_SERVER AND USE_INTERNAL_JSONCPP)
  message(STATUS "Compiling JsonCpp...")

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
  set_target_properties(jsoncpp_lib_static PROPERTIES IMPORTED_GLOBAL TRUE)
endif()
