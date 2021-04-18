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

### zlib ###

if(BUILD_CLIENT)
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
      --config RelWithDebInfo --target install
      --parallel ${PARALLEL_PROC_COUNT})
  endif()

  find_package(ZLIB)
  if(TARGET ZLIB::ZLIB)
    set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_GLOBAL True)
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
      --config RelWithDebInfo --target install
      --parallel ${PARALLEL_PROC_COUNT})
  endif()

  find_package(CURL)
  if(TARGET CURL::libcurl)
    set_target_properties(CURL::libcurl PROPERTIES IMPORTED_GLOBAL True)
    if(WIN32)
      target_link_libraries(CURL::libcurl INTERFACE ws2_32 crypt32)
    endif()
  endif()
endif()

### JsonCpp ###

if(BUILD_SERVER)
  message(STATUS "Compiling JsonCpp...")

  # Figure out the correct library path to attach to our imported target
  set(JSONCPP_LIBRARY_DEBUG
    "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}jsoncppd${libsuffix}")
  set(JSONCPP_LIBRARY_RELEASE
    "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}jsoncpp${libsuffix}")

  # Generate the build.
  execute_process(COMMAND "${CMAKE_COMMAND}"
    -S "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp"
    -B "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp-build"
    -G "${CMAKE_GENERATOR}"
    -A "${CMAKE_GENERATOR_PLATFORM}"
    -T "${CMAKE_GENERATOR_TOOLSET}"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_LINKER=${CMAKE_LINKER}"
    "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
    "-DCMAKE_INSTALL_LIBDIR=lib") # Without this, CentOS installs into lib64

  # Compile the library.
  get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(IS_MULTI_CONFIG)
    # Debug and Release are not compatible, so build both.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp-build"
      --config Debug --target install
      --parallel ${PARALLEL_PROC_COUNT})
    execute_process(COMMAND "${CMAKE_COMMAND}"
    --build "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp-build"
    --config RelWithDebInfo --target install
    --parallel ${PARALLEL_PROC_COUNT})
  else()
    # Single config uses the passed build type.
    execute_process(COMMAND "${CMAKE_COMMAND}"
      --build "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp-build"
      --target install --parallel ${PARALLEL_PROC_COUNT})
  endif()

  # Synthesize an imported target that can be linked against.
  add_library(jsoncpp STATIC IMPORTED GLOBAL)
  set_target_properties(jsoncpp PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS JSON_IS_AMALGAMATION
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/local/include"
    IMPORTED_LOCATION ${JSONCPP_LIBRARY_RELEASE}
    IMPORTED_LOCATION_DEBUG ${JSONCPP_LIBRARY_DEBUG}
    IMPORTED_LOCATION_RELEASE ${JSONCPP_LIBRARY_RELEASE}
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
endif()

### libpng ###

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
      --config RelWithDebInfo --target install
      --parallel ${PARALLEL_PROC_COUNT})
  endif()

  set(ZLIB_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/local/include" CACHE PATH "" FORCE)

  find_package(PNG)
  if(TARGET PNG::PNG)
    set_target_properties(PNG::PNG PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()

### MiniUPnPc ###

if(BUILD_SERVER)
  message(STATUS "Compiling MiniUPnPc...")

  # Figure out the correct library path to attach to our imported target
  set(MINIUPNPC_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/local/include/miniupnpc")
  set(MINIUPNPC_LIBRARY
    "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}miniupnpc${libsuffix}")

  # This must exist before the target is synthesized.
  file(MAKE_DIRECTORY ${MINIUPNPC_INCLUDE_DIR})

  # Generate the build.
  execute_process(COMMAND "${CMAKE_COMMAND}"
    -S "${CMAKE_CURRENT_SOURCE_DIR}/libminiupnpc"
    -B "${CMAKE_CURRENT_BINARY_DIR}/libminiupnpc-build"
    -G "${CMAKE_GENERATOR}"
    -A "${CMAKE_GENERATOR_PLATFORM}"
    -T "${CMAKE_GENERATOR_TOOLSET}"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_LINKER=${CMAKE_LINKER}"
    "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
    "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
    "-DCMAKE_INSTALL_LIBDIR=lib" # Without this, CentOS installs into lib64
    "-DUPNPC_BUILD_SHARED=No"
    "-DUPNPC_BUILD_TESTS=No")

  # Compile the library.
  execute_process(COMMAND "${CMAKE_COMMAND}"
    --build "${CMAKE_CURRENT_BINARY_DIR}/libminiupnpc-build"
    --config RelWithDebInfo --target install --parallel ${PARALLEL_PROC_COUNT})

  # Synthesize an imported target that can be linked against.
  add_library(upnpc-static STATIC IMPORTED GLOBAL)
  set_target_properties(upnpc-static PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${MINIUPNPC_INCLUDE_DIR}"
    IMPORTED_LOCATION ${MINIUPNPC_LIBRARY})
  if(WIN32)
    set_target_properties(upnpc-static PROPERTIES INTERFACE_LINK_LIBRARIES "ws2_32;iphlpapi")
  endif()
endif()

### SDL libraries ###

if(BUILD_CLIENT)

  ### SDL2 ###

  if(NOT USE_SDL12)
    if(WIN32)
      if(MSVC)
        file(DOWNLOAD
          "https://www.libsdl.org/release/SDL2-devel-2.0.12-VC.zip"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
          EXPECTED_HASH SHA256=00c55a597cebdb9a4eb2723f2ad2387a4d7fd605e222c69b46099b15d5d8b32d)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.12")
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
          "https://www.libsdl.org/release/SDL2-devel-2.0.12-mingw.tar.gz"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
          EXPECTED_HASH SHA256=e614a60f797e35ef9f3f96aef3dc6a1d786de3cc7ca6216f97e435c0b6aafc46)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.12/x86_64-w64-mingw32")
        else()
          set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.12/i686-w64-mingw32")
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
        "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxWidgets-3.1.4-headers.7z"
        "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.4-headers.7z"
        EXPECTED_HASH SHA256=9fa6087f24a4282cd883748d79f9b60b7ab54aced7af5aa616af810d09bd8f39)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.4-headers.7z"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

      if(MSVC_VERSION GREATER_EQUAL 1900)
        # Visual Studio 2015/2017/2019
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxMSW-3.1.4_vc14x_x64_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_x64_Dev.7z"
            EXPECTED_HASH SHA256=4f26915e90a9bb58807c6b716a1cb7399d3d2eb2ae46d2ed7af172ebc8f55390)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_x64_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxMSW-3.1.4_vc14x_x64_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_x64_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=7e3f14b7745d3520f256cc6b9f14788f9265fd857b758e55315482f45a6cc14c)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_x64_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_x64_dll/wxrc.exe"
            CACHE FILEPATH "")
        else()
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxMSW-3.1.4_vc14x_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_Dev.7z"
            EXPECTED_HASH SHA256=812c7e603799debe8d3bb486598d8585c953ade01bba14573d32485fd82a6651)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxMSW-3.1.4_vc14x_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=d2f20c0f612fd38d76b6380aa105a4132545b95ebe5e4cbc20f637766221283d)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_vc14x_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_dll/wxrc.exe"
            CACHE FILEPATH "")
        endif()
      elseif(MINGW)
        # MinGW
        file(DOWNLOAD
          "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxMSW-3.1.4_gcc810_x64_Dev.7z"
          "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_gcc810_x64_Dev.7z"
          EXPECTED_HASH SHA256=ad531472c6843114c8d4e8cad790c3a3050d6b6f0d6951b1d7b63025d9b8629f)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.4_gcc810_x64_Dev.7z"
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
