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
    execute_process(COMMAND "${CMAKE_COMMAND}"
      -S "${CMAKE_CURRENT_SOURCE_DIR}/libpng"
      -B "${CMAKE_CURRENT_BINARY_DIR}/libpng-build"
      -G "${CMAKE_GENERATOR}"
      -A "${CMAKE_GENERATOR_PLATFORM}"
      -T "${CMAKE_GENERATOR_TOOLSET}"
      "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
      "-DCMAKE_LINKER=${CMAKE_LINKER}"
      "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
      "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
      "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
      "-DPNG_SHARED=OFF"
      "-DPNG_TESTS=OFF")

    # Compile the library.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/libpng-build"
      --config RelWithDebInfo --target install)
  endif()

  set(ZLIB_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/local/include" CACHE PATH "" FORCE)

  find_package(PNG)
  if(TARGET PNG::PNG)
    set_target_properties(PNG::PNG PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
