# Build libraries.
#
# Here we use execute_process and file(DOWNLOAD) to download and compile
# libraries as we need them.  Some stuff to remember:
#
# - C libraries can cross Debug/Release boundaries, but C++ libraries can't.
#   You'll need to be able to select the proper debug vs release library in
#   multi-config builds - see JsonCpp for an example.
# - Xcode does not like it when CMake passes --parallel with no parameter.
#

# Parallel processor count - needed by Xcode.
include(ProcessorCount)
ProcessorCount(PARALLEL_PROC_COUNT)

# Set our prefix path, so inter-dependencies are picked up on.
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/local")

# Shorthand, to reduce line-noise
set(libprefix "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(libsuffix "${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(dllprefix "${CMAKE_SHARED_LIBRARY_PREFIX}")
set(dllsuffix "${CMAKE_SHARED_LIBRARY_SUFFIX}")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/local/include")

function(lib_buildgen)
  cmake_parse_arguments("" "" "LIBRARY;SRCDIR" "PARAMS" ${ARGN})

  if(NOT _SRCDIR)
    set(_SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/${_LIBRARY}")
  endif()

  # Baseline library generation.
  set(_COMMAND "${CMAKE_COMMAND}"
    -S "${_SRCDIR}"
    -B "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
    -G "${CMAKE_GENERATOR}"
    "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
    "-DCMAKE_INSTALL_LIBDIR=lib") # Without this, CentOS installs into lib64

  if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND _COMMAND -A)
    list(APPEND _COMMAND ${CMAKE_GENERATOR_PLATFORM})
  endif()

  if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND _COMMAND -T)
    list(APPEND _COMMAND ${CMAKE_GENERATOR_TOOLSET})
  endif()

  if(CMAKE_C_COMPILER)
    list(APPEND _COMMAND "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
  endif()

  if(CMAKE_CXX_COMPILER)
    list(APPEND _COMMAND "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
  endif()

  if(CMAKE_LINKER)
    list(APPEND _COMMAND "-DCMAKE_LINKER=${CMAKE_LINKER}")
  endif()

  if(CMAKE_RC_COMPILER)
    list(APPEND _COMMAND "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
  endif()

  get_property(_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT _IS_MULTI_CONFIG)
    list(APPEND _COMMAND "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
  endif()

  execute_process(COMMAND ${_COMMAND} ${_PARAMS} RESULT_VARIABLE _RESVAL)
  if(_RESVAL GREATER 0)
    message(FATAL_ERROR "Library ${_LIBRARY} did not generate correctly.")
  endif()
endfunction()

function(lib_build)
  cmake_parse_arguments("" "" "LIBRARY" "" ${ARGN})

  # Compile the library.
  get_property(_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(_IS_MULTI_CONFIG)
    # Debug and Release are not compatible, so build both.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
      --config Debug --target install
      --parallel ${PARALLEL_PROC_COUNT} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library ${_LIBRARY} did not build correctly as Debug.")
    endif()

    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
      --config RelWithDebInfo --target install
      --parallel ${PARALLEL_PROC_COUNT} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library ${_LIBRARY} did not build correctly as Release.")
    endif()  
  else()
    # Single config uses the passed build type.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY}-build"
      --config ${CMAKE_BUILD_TYPE} --target install
      --parallel ${PARALLEL_PROC_COUNT} RESULT_VARIABLE _RESVAL)
    if(_RESVAL GREATER 0)
      message(FATAL_ERROR "Library ${_LIBRARY} did not build correctly.")
    endif()  
  endif()
endfunction()

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

### FLTK (dep: libpng) ###

if(BUILD_CLIENT)
  message(STATUS "Compiling FLTK...")

  set(_FLTK_BUILDGEN_PARAMS
    "-DOPTION_USE_SYSTEM_LIBJPEG=OFF"
    "-DOPTION_PRINT_SUPPORT=OFF"
    "-DOPTION_USE_GL=OFF"
    "-DFLTK_BUILD_TEST=OFF")

  if(USE_INTERNAL_ZLIB)
    # FLTK defaults to the dynamic library, but we want the static lib.
    list(APPEND _FLTK_BUILDGEN_PARAMS
      "-DZLIB_LIBRARY_RELEASE=${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}zlibstatic${libsuffix}")
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

  find_package(FLTK CONFIG)
  if(NOT TARGET fltk)
    message(FATAL_ERROR "FLTK target not found.")
  endif()

  set_target_properties(fltk PROPERTIES IMPORTED_GLOBAL True)
  set_target_properties(fltk_images PROPERTIES IMPORTED_GLOBAL True)
  if(WIN32)
    target_link_libraries(fltk INTERFACE gdiplus)
  endif()
endif()

### fmtlib ###

if(BUILD_CLIENT OR BUILD_SERVER)
  message(STATUS "Compiling {fmt}...")

  lib_buildgen(
    LIBRARY fmt
    PARAMS "-DFMT_DOC=OFF"
           "-DFMT_INSTALL=ON"
           "-DFMT_TEST=OFF"
           "-DFMT_USE_CPP11=OFF")
  lib_build(LIBRARY fmt)

  find_package(fmt REQUIRED)
  set_target_properties(fmt::fmt PROPERTIES IMPORTED_GLOBAL TRUE)
endif()

### JsonCpp ###

if(BUILD_SERVER AND USE_INTERNAL_JSONCPP)
  message(STATUS "Compiling JsonCpp...")

  lib_buildgen(
    LIBRARY jsoncpp
    PARAMS "-DJSONCPP_WITH_TESTS=OFF"
           "-DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF"
           "-DJSONCPP_WITH_WARNING_AS_ERROR=OFF"
           "-DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF"
           "-DJSONCPP_WITH_CMAKE_PACKAGE=ON"
           "-DCMAKE_DEBUG_POSTFIX=d"
           "-DCCACHE_EXECUTABLE=CCACHE_EXECUTABLE-NOTFOUND")
  lib_build(LIBRARY jsoncpp)

  find_package(jsoncpp REQUIRED)
  set_target_properties(jsoncpp_lib_static PROPERTIES IMPORTED_GLOBAL TRUE)
endif()

### MiniUPnPc ###

if(BUILD_SERVER AND NOT USE_MINIUPNP)
  message(STATUS "Skipping MiniUPnPc...")
elseif(BUILD_SERVER AND USE_MINIUPNP AND USE_INTERNAL_MINIUPNP)
  message(STATUS "Compiling MiniUPnPc...")

  # Figure out the correct library path to attach to our imported target
  set(MINIUPNPC_LIBRARY
    "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}miniupnpc${libsuffix}")

  lib_buildgen(
    LIBRARY miniupnpc
    SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/miniupnp/miniupnpc"
    PARAMS "-DUPNPC_BUILD_SHARED=OFF"
           "-DUPNPC_BUILD_TESTS=OFF"
           "-DUPNPC_BUILD_SAMPLE=OFF")
  lib_build(LIBRARY miniupnpc)

  find_package(miniupnpc REQUIRED)
  set_target_properties(miniupnpc::miniupnpc PROPERTIES IMPORTED_GLOBAL TRUE)
endif()

### Protocol Buffers ###

if(BUILD_CLIENT OR BUILD_SERVER)
  message(STATUS "Compiling protobuf...")

  set(_PROTOBUF_BUILDGEN_PARAMS
    "-Dprotobuf_BUILD_SHARED_LIBS=OFF"
    "-Dprotobuf_BUILD_TESTS=OFF"
    "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF")

  lib_buildgen(
    LIBRARY protobuf
    SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/cmake"
    PARAMS ${_PROTOBUF_BUILDGEN_PARAMS})
  lib_build(LIBRARY protobuf)
endif()

### SDL libraries ###

if(BUILD_CLIENT)

  ### SDL2 ###

  if(NOT USE_SDL12)
    if(WIN32)
      if(MSVC)
        file(DOWNLOAD
          "https://www.libsdl.org/release/SDL2-devel-2.0.20-VC.zip"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
          EXPECTED_HASH SHA256=5b1512ca6c9d2427bd2147da01e5e954241f8231df12f54a7074dccde416df18)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.20")
        set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include" CACHE PATH "")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(SDL2_LIBRARY "${SDL2_DIR}/lib/x64/SDL2.lib" CACHE FILEPATH "")
          set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x64/SDL2main.lib" CACHE FILEPATH "")
        else()
          set(SDL2_LIBRARY "${SDL2_DIR}/lib/x86/SDL2.lib" CACHE FILEPATH "")
          set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x86/SDL2main.lib" CACHE FILEPATH "")
        endif()
      else()
        file(DOWNLOAD
          "https://www.libsdl.org/release/SDL2-devel-2.0.20-mingw.tar.gz"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
          EXPECTED_HASH SHA256=38094d82a857d6c62352e5c5cdec74948c5b4d25c59cbd298d6d233568976bd1)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.20/x86_64-w64-mingw32")
        else()
          set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.20/i686-w64-mingw32")
        endif()
        set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include/SDL2" CACHE PATH "")
        set(SDL2_LIBRARY "${SDL2_DIR}/lib/libSDL2.dll.a" CACHE FILEPATH "")
        set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/libSDL2main.a" CACHE FILEPATH "")
      endif()
    endif()

    find_package(SDL2)

    if(SDL2_FOUND)
      # SDL2 target.
      add_library(SDL2::SDL2 UNKNOWN IMPORTED GLOBAL)
      set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
        IMPORTED_LOCATION "${SDL2_LIBRARY}")

      if(SDL2MAIN_LIBRARY)
        # SDL2main target.
        if(MINGW)
          # Gross hack to get mingw32 first in the linker order.
          add_library(SDL2::_SDL2main_detail UNKNOWN IMPORTED GLOBAL)
          set_target_properties(SDL2::_SDL2main_detail PROPERTIES
            IMPORTED_LOCATION "${SDL2MAIN_LIBRARY}")

          # Ensure that SDL2main comes before SDL2 in the linker order.  CMake
          # isn't smart enough to keep proper ordering for indirect dependencies
          # so we have to spell it out here.
          target_link_libraries(SDL2::_SDL2main_detail INTERFACE SDL2::SDL2)

          add_library(SDL2::SDL2main INTERFACE IMPORTED GLOBAL)
          set_target_properties(SDL2::SDL2main PROPERTIES
            IMPORTED_LIBNAME mingw32)
          target_link_libraries(SDL2::SDL2main INTERFACE SDL2::_SDL2main_detail)
        else()
          add_library(SDL2::SDL2main UNKNOWN IMPORTED GLOBAL)
          set_target_properties(SDL2::SDL2main PROPERTIES
            IMPORTED_LOCATION "${SDL2MAIN_LIBRARY}")
        endif()
      endif()
    endif()
  endif()

  ### SDL2_mixer ###

  if(NOT USE_SDL12)
    if(WIN32)
      if(MSVC)
        file(DOWNLOAD
          "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
          EXPECTED_HASH SHA256=258788438b7e0c8abb386de01d1d77efe79287d9967ec92fbb3f89175120f0b0)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.0.4")
        set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_DIR}/include" CACHE PATH "")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x64/SDL2_mixer.lib" CACHE FILEPATH "")
        else()
          set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x86/SDL2_mixer.lib" CACHE FILEPATH "")
        endif()
      else()
        file(DOWNLOAD
          "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-mingw.tar.gz"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-mingw.tar.gz"
          EXPECTED_HASH SHA256=14250b2ade20866c7b17cf1a5a5e2c6f3920c443fa3744f45658c8af405c09f1)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-mingw.tar.gz"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.0.4/x86_64-w64-mingw32")
        else()
          set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.0.4/i686-w64-mingw32")
        endif()
        set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_DIR}/include/SDL2" CACHE PATH "")
        set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/libSDL2_mixer.dll.a" CACHE FILEPATH "")
      endif()
    endif()

    find_package(SDL2_mixer)

    if(SDL2_MIXER_FOUND)
      # SDL2_mixer target.
      add_library(SDL2::mixer UNKNOWN IMPORTED GLOBAL)
      set_target_properties(SDL2::mixer PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES SDL2::SDL2
        IMPORTED_LOCATION "${SDL2_MIXER_LIBRARY}")
    endif()
  endif()

  # We only get SDL 1.2 if the builder explicitly asks for it, and we do not
  # provide it ourselves, since SDL2 is preferred on most platforms and we
  # assume the builder knows what they're doing asking for 1.2.
  if(USE_SDL12)

    ### SDL ###

    find_package(SDL)
    if(SDL_FOUND)
      message(STATUS "Using SDL version ${SDL_VERSION_STRING}")

      # [AM] FindSDL.cmake is kind of a hot mess, this is my best attempt at
      #      turning it into a neat and tidy target.
      if(UNIX AND NOT APPLE)
        # On Linux, CMake rolls all the link libraries into one list - the main
        # library, the actual library, and pthread (which sdl-config says is
        # unnecessary).
        list(POP_FRONT SDL_LIBRARY)
        list(POP_FRONT SDL_LIBRARY SDL_ACTUAL_LIBRARY)
        set(SDL_LIBRARY "${SDL_ACTUAL_LIBRARY}")
        unset(SDL_ACTUAL_LIBRARY)
      else()
        message(FATAL_ERROR "Unknown platform for SDL 1.2")
      endif()

      if(TARGET SDL::SDL)
        # Ensure that the client can see the target.
        set_target_properties(SDL::SDL PROPERTIES IMPORTED_GLOBAL True)
      else()
        # Synthesize SDL target if it doesn't exist.
        add_library(SDL::SDL UNKNOWN IMPORTED GLOBAL)
        set_target_properties(SDL::SDL PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${SDL_INCLUDE_DIR}"
          IMPORTED_LOCATION "${SDL_LIBRARY}")

        if(SDLMAIN_LIBRARY)
          # SDLmain target.
          add_library(SDL::SDLmain UNKNOWN IMPORTED GLOBAL)
          set_target_properties(SDL::SDLmain PROPERTIES
            IMPORTED_LOCATION "${SDLMAIN_LIBRARY}")
        endif()
      endif()
    endif()

    ### SDL_mixer ###

    find_package(SDL_mixer)
    if(SDL_FOUND)
      message(STATUS "Using SDL_mixer version ${SDL_MIXER_VERSION_STRING}")

      # SDL_mixer target.
      add_library(SDL::mixer UNKNOWN IMPORTED GLOBAL)
      set_target_properties(SDL::mixer PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL_MIXER_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES SDL::SDL
        IMPORTED_LOCATION "${SDL_MIXER_LIBRARY}")
    endif()
  endif()
endif()

### wxWidgets ###

if(BUILD_LAUNCHER)
  if(USE_INTERNAL_WXWIDGETS)
    if(WIN32)
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
      set(wxWidgets_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets" CACHE PATH "")

      # Cross-compiler headers
      file(DOWNLOAD
        "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxWidgets-3.1.5-headers.7z"
        "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.5-headers.7z"
        EXPECTED_HASH SHA256=5BEF630B59CBE515152EBAABC2B5BB83BBB908B798ACCBF28E4F3D79480EC0E2)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.5-headers.7z"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

      if(MSVC_VERSION GREATER_EQUAL 1900)
        # Visual Studio 2015/2017/2019
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_x64_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_Dev.7z"
            EXPECTED_HASH SHA256=7FC34D32030A6BF84B5C3B00D3CECA12683ECAB514BDBED91723AB67B22E8FBA)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_x64_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=29CA27A2CEE48EB8B07D9F9B5318AF4068AD5D26344F021A311F8557060B1433)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_x64_dll/wxrc.exe"
            CACHE FILEPATH "")
        else()
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_Dev.7z"
            EXPECTED_HASH SHA256=751C00CCEB1FC5243C8FDA45F678732AEEFB10E0A2E348DBD3A7131C8A475898)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=B9EC5AF60CE0E489AB6D23CB75004CBD10281932EF353FD44FF51A51143D776D)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_dll/wxrc.exe"
            CACHE FILEPATH "")
        endif()
      elseif(MINGW)
        # MinGW
        file(DOWNLOAD
          "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_gcc810_x64_Dev.7z"
          "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_gcc810_x64_Dev.7z"
          EXPECTED_HASH SHA256=65ED68EF72C5E9807B64FE664EBA561D4C33F494D71DCDF21D39110C601FD327)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_gcc810_x64_Dev.7z"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

        # Move the lib directory to where FindwxWidgets.cmake can find it.
        if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc_dll")
          file(RENAME
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc810_x64_dll"
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc_dll")
        endif()

        set(wxWidgets_wxrc_EXECUTABLE
          "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc_dll/wxrc.exe"
          CACHE FILEPATH "")
      endif()
    endif()
  endif()
endif()
