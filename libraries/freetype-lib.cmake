### freetype (dep: zlib) ###

if(BUILD_CLIENT)
  if(USE_INTERNAL_FREETYPE)
    message(STATUS "Compiling internal freetype...")

    # Set vars so the finder can find them.
    set(FREETYPE_INCLUDE_DIR_ft2build
      "${CMAKE_CURRENT_BINARY_DIR}/local/include/freetype2")
    set(FREETYPE_INCLUDE_DIR_freetype2
      "${CMAKE_CURRENT_BINARY_DIR}/local/include/freetype2")
    set(FREETYPE_LIBRARY
      "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}freetype${libsuffix}")

    lib_buildgen(
      LIBRARY freetype
      PARAMS "-DBUILD_SHARED_LIBS=OFF"
             "-DFT_DISABLE_HARFBUZZ=ON"
             "-DFT_DISABLE_BROTLI=ON"
             "-DFT_DISABLE_BZIP2=ON"
             "-DFT_DISABLE_PNG=ON"
             "-DFT_DISABLE_ZLIB=ON")

    lib_build(LIBRARY freetype)
  endif()

  find_package(Freetype REQUIRED)
  if(TARGET Freetype::Freetype)
    set_target_properties(Freetype::Freetype PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
