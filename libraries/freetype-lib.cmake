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

    set(_FREETYPE_GEN_ARGS
      -S "${CMAKE_CURRENT_SOURCE_DIR}/freetype"
      -B "${CMAKE_CURRENT_BINARY_DIR}/freetype-build"
      -G "${CMAKE_GENERATOR}"
      "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
      "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
      "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
      "-DCMAKE_INSTALL_LIBDIR=lib"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DFT_DISABLE_HARFBUZZ=ON"
      "-DFT_DISABLE_BROTLI=ON"
      "-DFT_DISABLE_BZIP2=ON"
      "-DFT_DISABLE_PNG=ON"
      "-DFT_DISABLE_ZLIB=ON")
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND _FREETYPE_GEN_ARGS -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
      list(APPEND _FREETYPE_GEN_ARGS -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    if(CMAKE_C_COMPILER)
      list(APPEND _FREETYPE_GEN_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
    endif()
    if(CMAKE_LINKER)
      list(APPEND _FREETYPE_GEN_ARGS "-DCMAKE_LINKER=${CMAKE_LINKER}")
    endif()
    if(CMAKE_RC_COMPILER)
      list(APPEND _FREETYPE_GEN_ARGS "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
    endif()
    if(CMAKE_OSX_ARCHITECTURES)
      string(REPLACE ";" "\\;" _OSX_ARCHS "${CMAKE_OSX_ARCHITECTURES}")
      list(APPEND _FREETYPE_GEN_ARGS "-DCMAKE_OSX_ARCHITECTURES=${_OSX_ARCHS}")
    endif()
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
      list(APPEND _FREETYPE_GEN_ARGS "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    lib_buildgen(
      LIBRARY freetype
      PARAMS ${_FREETYPE_GEN_ARGS})

		lib_build(LIBRARY freetype)
  endif()

  find_package(Freetype REQUIRED)
  if(TARGET Freetype::Freetype)
    set_target_properties(Freetype::Freetype PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
