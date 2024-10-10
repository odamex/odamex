### FLTK (dep: libpng) ###

if(BUILD_CLIENT)
  message(STATUS "Compiling FLTK...")

  set(_FLTK_BUILDGEN_PARAMS
    "-DOPTION_USE_SYSTEM_LIBJPEG=OFF"
    "-DOPTION_PRINT_SUPPORT=OFF"
    "-DOPTION_USE_GL=OFF"
    "-DFLTK_BUILD_TEST=OFF")

  if(USE_INTERNAL_ZLIB)
    # FLTK defaults to the dynamic library, but we want the static lib.
    list(APPEND _FLTK_BUILDGEN_PARAMS
      "-DZLIB_LIBRARY_RELEASE=${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}zlibstatic${libsuffix}")
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
  if(NOT TARGET fltk)
    message(FATAL_ERROR "FLTK target not found.")
  endif()

  set_target_properties(fltk PROPERTIES IMPORTED_GLOBAL True)
  set_target_properties(fltk_images PROPERTIES IMPORTED_GLOBAL True)
  if(WIN32)
    target_link_libraries(fltk INTERFACE gdiplus)
  endif()
endif()
