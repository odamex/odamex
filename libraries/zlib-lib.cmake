### zlib ###

if(BUILD_CLIENT OR BUILD_SERVER)
  if(USE_INTERNAL_ZLIB)
    message(STATUS "Compiling internal ZLIB...")

    # Set vars so the finder can find them.
    set(ZLIB_INCLUDE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/local/include")
    if(WIN32)
      set(ZLIB_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}zlibstatic${libsuffix}")
    else()
      set(ZLIB_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}z${libsuffix}")
    endif()

    # Generate the build.
    set(_ZLIB_GEN_ARGS
      -S "${CMAKE_CURRENT_SOURCE_DIR}/zlib"
      -B "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
      -G "${CMAKE_GENERATOR}"
      "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local")
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND _ZLIB_GEN_ARGS -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
      list(APPEND _ZLIB_GEN_ARGS -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    if(CMAKE_C_COMPILER)
      list(APPEND _ZLIB_GEN_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
    endif()
    if(CMAKE_LINKER)
      list(APPEND _ZLIB_GEN_ARGS "-DCMAKE_LINKER=${CMAKE_LINKER}")
    endif()
    if(CMAKE_RC_COMPILER)
      list(APPEND _ZLIB_GEN_ARGS "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
    endif()
    if(CMAKE_OSX_ARCHITECTURES)
      list(APPEND _ZLIB_GEN_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
    endif()
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
      list(APPEND _ZLIB_GEN_ARGS "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    execute_process(COMMAND "${CMAKE_COMMAND}" ${_ZLIB_GEN_ARGS} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library zlib did not generate correctly.")
    endif()

    # Compile the library.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/zlib-build"
      --config RelWithDebInfo --target install
      RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library zlib did not build correctly.")
    endif()
  endif()

  find_package(ZLIB)
  if(TARGET ZLIB::ZLIB)
    set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
