### libcurl ###

if(BUILD_CLIENT)
  if(USE_INTERNAL_CURL)
    # [AM] Don't put an early return in this block, otherwise you run the risk
    #      of changes in the build cache not percolating down to the library.

    message(STATUS "Compiling internal CURL...")

    # Set vars so the finder can find them.
    set(CURL_INCLUDE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/local/include")
    set(CURL_LIBRARY
      "${CMAKE_CURRENT_BINARY_DIR}/local/lib/libcurl${libsuffix}")

    if(WIN32)
      set(_COMPILE_CURL_WINSSL ON)
    else()
      set(_COMPILE_CURL_WINSSL OFF)
    endif()

    # Generate the build.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      -S "${CMAKE_CURRENT_SOURCE_DIR}/curl"
      -B "${CMAKE_CURRENT_BINARY_DIR}/curl-build"
      -G "${CMAKE_GENERATOR}"
      -A "${CMAKE_GENERATOR_PLATFORM}"
      -T "${CMAKE_GENERATOR_TOOLSET}"
      "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
      "-DCMAKE_LINKER=${CMAKE_LINKER}"
      "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
      "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
      "-DBUILD_CURL_EXE=OFF"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DCMAKE_USE_LIBSSH2=OFF"
      "-DCMAKE_USE_WINSSL=${_COMPILE_CURL_WINSSL}"
      "-DCURL_ZLIB=OFF"
      "-DHTTP_ONLY=ON")
    unset(_COMPILE_CURL_WINSSL)

    # Compile the library.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/curl-build"
      --config RelWithDebInfo --target install)
  endif()

  find_package(CURL)
  if(TARGET CURL::libcurl)
    set_target_properties(CURL::libcurl PROPERTIES IMPORTED_GLOBAL True)
    if(WIN32)
      target_link_libraries(CURL::libcurl INTERFACE ws2_32 crypt32)
    endif()
  endif()
endif()
