### zlib ###

if(BUILD_CLIENT OR BUILD_SERVER)
  if(USE_INTERNAL_ZLIB)
    message(STATUS "Compiling internal ZLIB...")

    # Set vars so the finder can find them.
    set(ZLIB_INCLUDE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/local/include")
    set(ZLIB_LIBRARY
      "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}zlibstatic${libsuffix}")

    # Generate the build.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      -S "${CMAKE_CURRENT_SOURCE_DIR}/zlib"
      -B "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
      -G "${CMAKE_GENERATOR}"
      -A "${CMAKE_GENERATOR_PLATFORM}"
      -T "${CMAKE_GENERATOR_TOOLSET}"
      "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
      "-DCMAKE_LINKER=${CMAKE_LINKER}"
      "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
      "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local")
    unset(_COMPILE_CURL_WINSSL)

    # Compile the library.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
      --config RelWithDebInfo --target install)
  endif()

  find_package(ZLIB)
  if(TARGET ZLIB::ZLIB)
    set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
