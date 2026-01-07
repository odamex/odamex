### FLTK (dep: libpng) ###

if(BUILD_CLIENT AND USE_INTERNAL_FLTK)
  set(_FLTK_BUILDGEN_PARAMS
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
    "-DFLTK_USE_SYSTEM_LIBJPEG=OFF"
    "-DFLTK_USE_SYSTEM_LIBPNG=OFF"
    "-DFLTK_USE_SYSTEM_ZLIB=OFF"
    "-DFLTK_OPTION_PRINT_SUPPORT=OFF"
    "-DFLTK_BUILD_GL=OFF"

    "-DFLTK_BUILD_TEST=OFF")
  if(APPLE)
    list(APPEND _FLTK_BUILDGEN_PARAMS
      "-DFLTK_USE_SYSTEM_LIBPNG=ON"
      "-DFLTK_USE_SYSTEM_ZLIB=ON")
  endif()

  if(USE_INTERNAL_ZLIB)
    # FLTK defaults to the dynamic library, but we want the static lib.
    if(WIN32)
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DZLIB_LIBRARY_RELEASE=${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}libzstatic${libsuffix}")
    else()
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DZLIB_LIBRARY_RELEASE=${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}z${libsuffix}"
        "-DZLIB_INCLUDE_DIR=${CMAKE_CURRENT_BINARY_DIR}/local/include")
    endif()
  endif()

  if(USE_INTERNAL_PNG)
    # FLTK looks in the wrong places for libpng headers, so it needs extra
    # help.
    list(APPEND _FLTK_BUILDGEN_PARAMS
      "-DPNG_PNG_INCLUDE_DIR=${CMAKE_CURRENT_BINARY_DIR}/local/include/libpng16"
      "-DHAVE_PNG_H=ON")
  endif()

  lib_buildgen(LIBRARY fltk PARAMS ${_FLTK_BUILDGEN_PARAMS})
  lib_build(LIBRARY fltk)

  find_package(FLTK CONFIG)
  if(NOT TARGET fltk::fltk)
    find_package(FLTK REQUIRED)
  endif()
  if(NOT TARGET fltk::fltk)
    if(FLTK_LIBRARIES)
      add_library(fltk::fltk INTERFACE IMPORTED GLOBAL)
      set_target_properties(fltk::fltk PROPERTIES
        INTERFACE_LINK_LIBRARIES "${FLTK_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${FLTK_INCLUDE_DIRS}")
    else()
      message(FATAL_ERROR "FLTK target not found.")
    endif()
  endif()

  set_target_properties(fltk::fltk PROPERTIES IMPORTED_GLOBAL True)
  if(NOT TARGET fltk::images)
    add_library(fltk::images INTERFACE IMPORTED GLOBAL)
    if(FLTK_IMAGES_LIBRARIES)
      set_target_properties(fltk::images PROPERTIES
        INTERFACE_LINK_LIBRARIES "${FLTK_IMAGES_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${FLTK_INCLUDE_DIRS}")
    else()
      set_target_properties(fltk::images PROPERTIES
        INTERFACE_LINK_LIBRARIES "${FLTK_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${FLTK_INCLUDE_DIRS}")
    endif()
  endif()
  set_target_properties(fltk::images PROPERTIES IMPORTED_GLOBAL True)
  if(WIN32)
    target_link_libraries(fltk::fltk INTERFACE gdiplus)
  endif()
endif()
