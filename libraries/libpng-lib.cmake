### libpng (dep: zlib) ###

if(BUILD_CLIENT)
  if(USE_INTERNAL_PNG)
    message(STATUS "Compiling internal libpng...")

    # Set vars so the finder can find them.
    set(PNG_PNG_INCLUDE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/local/include")
    if(NOT MSVC)
      set(PNG_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/local/lib/libpng16${libsuffix}")
    else()
      set(PNG_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/local/lib/libpng16_static${libsuffix}")
    endif()

    # Generate the build.
    set(_LIBPNG_GEN_ARGS
      -S "${CMAKE_CURRENT_SOURCE_DIR}/libpng"
      -B "${CMAKE_CURRENT_BINARY_DIR}/libpng-build"
      -G "${CMAKE_GENERATOR}"
      "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
      "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
      "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
      "-DPNG_SHARED=OFF"
      "-DPNG_TESTS=OFF")
    if(APPLE AND CMAKE_OSX_ARCHITECTURES)
      list(APPEND _LIBPNG_GEN_ARGS "-DPNG_ARM_NEON=off")
    endif()
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND _LIBPNG_GEN_ARGS -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
      list(APPEND _LIBPNG_GEN_ARGS -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    if(CMAKE_C_COMPILER)
      list(APPEND _LIBPNG_GEN_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
    endif()
    if(CMAKE_LINKER)
      list(APPEND _LIBPNG_GEN_ARGS "-DCMAKE_LINKER=${CMAKE_LINKER}")
    endif()
    if(CMAKE_RC_COMPILER)
      list(APPEND _LIBPNG_GEN_ARGS "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
    endif()
    if(CMAKE_OSX_ARCHITECTURES)
      string(REPLACE ";" "\\;" _OSX_ARCHS "${CMAKE_OSX_ARCHITECTURES}")
      list(APPEND _LIBPNG_GEN_ARGS "-DCMAKE_OSX_ARCHITECTURES=${_OSX_ARCHS}")
    endif()
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
      list(APPEND _LIBPNG_GEN_ARGS "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    execute_process(COMMAND "${CMAKE_COMMAND}" ${_LIBPNG_GEN_ARGS} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library libpng did not generate correctly.")
    endif()

    # Compile the library.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/libpng-build"
      --config RelWithDebInfo --target install
      RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library libpng did not build correctly.")
    endif()
  endif()

  if (USE_INTERNAL_ZLIB)
    set(ZLIB_INCLUDE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/local/include" CACHE PATH "" FORCE)
  endif()

  find_package(PNG)
  if(TARGET PNG::PNG)
    set_target_properties(PNG::PNG PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
