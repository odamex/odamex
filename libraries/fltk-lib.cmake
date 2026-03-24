### FLTK (dep: libpng) ###

if(BUILD_CLIENT AND USE_INTERNAL_FLTK)
  set(_FLTK_BUILDGEN_PARAMS
    "-DFLTK_USE_SYSTEM_LIBJPEG=OFF"
    "-DFLTK_USE_SYSTEM_LIBPNG=OFF"
    "-DFLTK_USE_SYSTEM_ZLIB=OFF"
    "-DFLTK_OPTION_PRINT_SUPPORT=OFF"
    "-DFLTK_BUILD_GL=OFF"
    "-DFLTK_USE_PANGO=OFF"
    "-DFLTK_GRAPHICS_CAIRO=OFF"
    "-DFLTK_OPTION_CAIRO_WINDOW=OFF"
    "-DFLTK_OPTION_CAIRO_EXT=OFF"
    "-DFLTK_BUILD_TEST=OFF")
  if(UNIX)
    if(NOT USE_INTERNAL_PNG)
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DFLTK_USE_SYSTEM_LIBPNG=ON")
    endif()
    if(NOT USE_INTERNAL_ZLIB)
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DFLTK_USE_SYSTEM_ZLIB=ON")
    endif()
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

  function(odamex_define_fltk_targets libdir incdir extra_libs)
    add_library(fltk::fltk INTERFACE IMPORTED GLOBAL)
    set(_fltk_core_libs
      "${libdir}/${libprefix}fltk${libsuffix};${libdir}/${libprefix}fltk_forms${libsuffix}")
    if(extra_libs)
      set(_fltk_core_libs "${_fltk_core_libs};${extra_libs}")
    endif()
    set_target_properties(fltk::fltk PROPERTIES
      INTERFACE_LINK_LIBRARIES "${_fltk_core_libs}"
      INTERFACE_INCLUDE_DIRECTORIES "${incdir}"
      IMPORTED_GLOBAL True)

    add_library(fltk::images INTERFACE IMPORTED GLOBAL)
    set_target_properties(fltk::images PROPERTIES
      INTERFACE_LINK_LIBRARIES "${libdir}/${libprefix}fltk_images${libsuffix}"
      INTERFACE_INCLUDE_DIRECTORIES "${incdir}"
      IMPORTED_GLOBAL True)
    if(EXISTS "${libdir}/${libprefix}fltk_png${libsuffix}")
      set_property(TARGET fltk::images APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${libdir}/${libprefix}fltk_png${libsuffix}")
    endif()
    if(EXISTS "${libdir}/${libprefix}fltk_z${libsuffix}")
      set_property(TARGET fltk::images APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${libdir}/${libprefix}fltk_z${libsuffix}")
    endif()
  endfunction()

  lib_buildgen(LIBRARY fltk PARAMS ${_FLTK_BUILDGEN_PARAMS})
  lib_build(LIBRARY fltk)

  set(_FLTK_LOCAL_LIBDIR "${CMAKE_CURRENT_BINARY_DIR}/local/lib")
  set(_FLTK_LOCAL_INCDIR "${CMAKE_CURRENT_BINARY_DIR}/local/include")

  if(APPLE)
    odamex_define_fltk_targets(
      "${_FLTK_LOCAL_LIBDIR}"
      "${_FLTK_LOCAL_INCDIR}"
      "-framework Cocoa;-framework ApplicationServices;-framework UniformTypeIdentifiers")
  else()
    find_package(FLTK CONFIG REQUIRED)
    set_target_properties(fltk::fltk PROPERTIES IMPORTED_GLOBAL True)
    set_target_properties(fltk::images PROPERTIES IMPORTED_GLOBAL True)
  endif()
  if(WIN32)
    target_link_libraries(fltk::fltk INTERFACE gdiplus)
  endif()
endif()
