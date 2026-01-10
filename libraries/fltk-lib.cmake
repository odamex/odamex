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
  if(APPLE)
    if(NOT USE_INTERNAL_PNG)
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DFLTK_USE_SYSTEM_LIBPNG=ON")
    endif()
    if(NOT USE_INTERNAL_ZLIB)
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DFLTK_USE_SYSTEM_ZLIB=ON")
    endif()
  elseif(UNIX AND NOT APPLE)
    list(APPEND _FLTK_BUILDGEN_PARAMS
      "-DFLTK_USE_SYSTEM_LIBDECOR=OFF"
      "-DFLTK_USE_LIBDECOR_GTK=OFF")
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
    if(EXISTS "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk${libsuffix}")
      odamex_define_fltk_targets(
        "${_FLTK_LOCAL_LIBDIR}"
        "${_FLTK_LOCAL_INCDIR}"
        "")
    else()
      function(odamex_sanitize_fltk_libs input_list output_var)
        set(result "")
        set(mode "general")
        foreach(item IN LISTS input_list)
          if(item STREQUAL "optimized")
            set(mode "optimized")
          elseif(item STREQUAL "debug")
            set(mode "debug")
          elseif(item STREQUAL "general")
            set(mode "general")
          elseif(item STREQUAL "FLTK_LIBRARIES" OR item STREQUAL "FLTK_IMAGES_LIBRARIES"
              OR item STREQUAL "-lFLTK_LIBRARIES" OR item STREQUAL "-lFLTK_IMAGES_LIBRARIES")
            set(mode "general")
          else()
            if(WIN32)
              if(mode STREQUAL "debug")
                list(APPEND result "$<$<CONFIG:Debug>:${item}>")
              elseif(mode STREQUAL "optimized")
                list(APPEND result "$<$<NOT:$<CONFIG:Debug>>:${item}>")
              else()
                list(APPEND result "${item}")
              endif()
            else()
              list(APPEND result "${item}")
            endif()
            set(mode "general")
          endif()
        endforeach()
        set(${output_var} "${result}" PARENT_SCOPE)
      endfunction()

      find_package(FLTK CONFIG)
      if(NOT TARGET fltk::fltk)
        find_package(FLTK REQUIRED)
      endif()
      if(NOT TARGET fltk::fltk)
        if(FLTK_LIBRARIES)
          list(REMOVE_ITEM FLTK_LIBRARIES "FLTK_GL_LIBRARY-NOTFOUND" "-lFLTK_GL_LIBRARY-NOTFOUND")
          odamex_sanitize_fltk_libs(FLTK_LIBRARIES FLTK_LIBRARIES_SANITIZED)
          add_library(fltk::fltk INTERFACE IMPORTED GLOBAL)
          set_target_properties(fltk::fltk PROPERTIES
            INTERFACE_LINK_LIBRARIES "${FLTK_LIBRARIES_SANITIZED}"
            INTERFACE_INCLUDE_DIRECTORIES "${FLTK_INCLUDE_DIRS}"
            IMPORTED_GLOBAL True)
        else()
          message(FATAL_ERROR "FLTK target not found.")
        endif()
      else()
        set_target_properties(fltk::fltk PROPERTIES IMPORTED_GLOBAL True)
      endif()

      if(NOT TARGET fltk::images)
        add_library(fltk::images INTERFACE IMPORTED GLOBAL)
        if(FLTK_IMAGES_LIBRARIES)
          list(REMOVE_ITEM FLTK_IMAGES_LIBRARIES "FLTK_GL_LIBRARY-NOTFOUND" "-lFLTK_GL_LIBRARY-NOTFOUND")
          odamex_sanitize_fltk_libs(FLTK_IMAGES_LIBRARIES FLTK_IMAGES_LIBRARIES_SANITIZED)
          set_target_properties(fltk::images PROPERTIES
            INTERFACE_LINK_LIBRARIES "${FLTK_IMAGES_LIBRARIES_SANITIZED}"
            INTERFACE_INCLUDE_DIRECTORIES "${FLTK_INCLUDE_DIRS}"
            IMPORTED_GLOBAL True)
        else()
          set_target_properties(fltk::images PROPERTIES
            INTERFACE_LINK_LIBRARIES "${FLTK_LIBRARIES_SANITIZED}"
            INTERFACE_INCLUDE_DIRECTORIES "${FLTK_INCLUDE_DIRS}"
            IMPORTED_GLOBAL True)
        endif()
      else()
        set_target_properties(fltk::images PROPERTIES IMPORTED_GLOBAL True)
      endif()
    endif()
  endif()
  if(UNIX AND NOT APPLE)
    find_library(_FLTK_XFT_LIB Xft)
    find_library(_FLTK_FONTCONFIG_LIB fontconfig)
    find_library(_FLTK_XRENDER_LIB Xrender)
    find_library(_FLTK_XFIXES_LIB Xfixes)
    find_library(_FLTK_XCURSOR_LIB Xcursor)
    find_library(_FLTK_XINERAMA_LIB Xinerama)
    find_library(_FLTK_WAYLAND_CLIENT_LIB wayland-client)
    find_library(_FLTK_WAYLAND_CURSOR_LIB wayland-cursor)
    find_library(_FLTK_XKBCOMMON_LIB xkbcommon)
    find_library(_FLTK_CAIRO_LIB cairo)
    find_library(_FLTK_GOBJECT_LIB gobject-2.0)
    find_library(_FLTK_GLIB_LIB glib-2.0)
    find_library(_FLTK_PANGO_LIB pango-1.0)
    find_library(_FLTK_PANGOCAIRO_LIB pangocairo-1.0)
    find_library(_FLTK_PANGOXFT_LIB pangoxft-1.0)
    if(_FLTK_XFT_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_XFT_LIB}")
    endif()
    if(_FLTK_FONTCONFIG_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_FONTCONFIG_LIB}")
    endif()
    if(_FLTK_XRENDER_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_XRENDER_LIB}")
    endif()
    if(_FLTK_XFIXES_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_XFIXES_LIB}")
    endif()
    if(_FLTK_XCURSOR_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_XCURSOR_LIB}")
    endif()
    if(_FLTK_XINERAMA_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_XINERAMA_LIB}")
    endif()
    if(_FLTK_WAYLAND_CLIENT_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_WAYLAND_CLIENT_LIB}")
    endif()
    if(_FLTK_WAYLAND_CURSOR_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_WAYLAND_CURSOR_LIB}")
    endif()
    if(_FLTK_XKBCOMMON_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_XKBCOMMON_LIB}")
    endif()
    if(_FLTK_CAIRO_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_CAIRO_LIB}")
    endif()
    if(_FLTK_GOBJECT_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_GOBJECT_LIB}")
    endif()
    if(_FLTK_GLIB_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_GLIB_LIB}")
    endif()
    if(_FLTK_PANGO_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_PANGO_LIB}")
    endif()
    if(_FLTK_PANGOCAIRO_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_PANGOCAIRO_LIB}")
    endif()
    if(_FLTK_PANGOXFT_LIB)
      set_property(TARGET fltk::fltk APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${_FLTK_PANGOXFT_LIB}")
    endif()
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}fltk_png${libsuffix}")
      set_property(TARGET fltk::images APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}fltk_png${libsuffix}")
    endif()
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}fltk_z${libsuffix}")
      set_property(TARGET fltk::images APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}fltk_z${libsuffix}")
    endif()
  endif()
  if(WIN32)
    target_link_libraries(fltk::fltk INTERFACE gdiplus ole32 uuid comctl32 ws2_32)
  endif()
endif()
