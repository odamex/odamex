### fmtlib ###

if(BUILD_CLIENT OR BUILD_SERVER)
  message(STATUS "Compiling {fmt}...")

  lib_buildgen(
    LIBRARY fmt
    PARAMS "-DFMT_DOC=OFF"
           "-DFMT_INSTALL=ON"
           "-DFMT_TEST=OFF"
           "-DFMT_USE_CPP11=OFF")
  lib_build(LIBRARY fmt)

  find_package(fmt REQUIRED)
  set_target_properties(fmt::fmt PROPERTIES IMPORTED_GLOBAL TRUE)
endif()
