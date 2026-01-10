### FLTK (dep: libpng) ###

if(BUILD_CLIENT AND USE_INTERNAL_FLTK)
  set(_FLTK_BUILDGEN_PARAMS
    "-DOPTION_USE_SYSTEM_LIBJPEG=OFF"
    "-DOPTION_USE_SYSTEM_LIBPNG=OFF"
    "-DOPTION_USE_SYSTEM_ZLIB=OFF"
    "-DOPTION_PRINT_SUPPORT=OFF"
    "-DOPTION_USE_GL=OFF"
    "-DFLTK_BUILD_TEST=OFF")
  if(APPLE)
    if(NOT USE_INTERNAL_PNG)
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DOPTION_USE_SYSTEM_LIBPNG=ON")
    endif()
    if(NOT USE_INTERNAL_ZLIB)
      list(APPEND _FLTK_BUILDGEN_PARAMS
        "-DOPTION_USE_SYSTEM_ZLIB=ON")
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

  lib_buildgen(LIBRARY fltk PARAMS ${_FLTK_BUILDGEN_PARAMS})
  lib_build(LIBRARY fltk)

  if(APPLE)
    set(_FLTK_LOCAL_LIBDIR "${CMAKE_CURRENT_BINARY_DIR}/local/lib")
    set(_FLTK_LOCAL_INCDIR "${CMAKE_CURRENT_BINARY_DIR}/local/include")
    add_library(fltk::fltk INTERFACE IMPORTED GLOBAL)
    set_target_properties(fltk::fltk PROPERTIES
      INTERFACE_LINK_LIBRARIES
        "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk${libsuffix};${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_forms${libsuffix};-framework Cocoa;-framework ApplicationServices"
      INTERFACE_INCLUDE_DIRECTORIES "${_FLTK_LOCAL_INCDIR}")
    add_library(fltk::images INTERFACE IMPORTED GLOBAL)
    set_target_properties(fltk::images PROPERTIES
      INTERFACE_LINK_LIBRARIES
        "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_images${libsuffix}"
      INTERFACE_INCLUDE_DIRECTORIES "${_FLTK_LOCAL_INCDIR}")
    if(EXISTS "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_png${libsuffix}")
      set_property(TARGET fltk::images APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_png${libsuffix}")
    endif()
    if(EXISTS "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_z${libsuffix}")
      set_property(TARGET fltk::images APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_z${libsuffix}")
    endif()
  else()
    set(FLTK_SKIP_OPENGL TRUE)
    set(_FLTK_LOCAL_LIBDIR "${CMAKE_CURRENT_BINARY_DIR}/local/lib")
    set(_FLTK_LOCAL_INCDIR "${CMAKE_CURRENT_BINARY_DIR}/local/include")
    if(EXISTS "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk${libsuffix}")
      add_library(fltk::fltk INTERFACE IMPORTED GLOBAL)
      set_target_properties(fltk::fltk PROPERTIES
        INTERFACE_LINK_LIBRARIES
          "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk${libsuffix};${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_forms${libsuffix}"
        INTERFACE_INCLUDE_DIRECTORIES "${_FLTK_LOCAL_INCDIR}"
        IMPORTED_GLOBAL True)
      add_library(fltk::images INTERFACE IMPORTED GLOBAL)
      set_target_properties(fltk::images PROPERTIES
        INTERFACE_LINK_LIBRARIES
          "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_images${libsuffix}"
        INTERFACE_INCLUDE_DIRECTORIES "${_FLTK_LOCAL_INCDIR}"
        IMPORTED_GLOBAL True)
      if(EXISTS "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_png${libsuffix}")
        set_property(TARGET fltk::images APPEND PROPERTY
          INTERFACE_LINK_LIBRARIES "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_png${libsuffix}")
      endif()
      if(EXISTS "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_z${libsuffix}")
        set_property(TARGET fltk::images APPEND PROPERTY
          INTERFACE_LINK_LIBRARIES "${_FLTK_LOCAL_LIBDIR}/${libprefix}fltk_z${libsuffix}")
      endif()
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
