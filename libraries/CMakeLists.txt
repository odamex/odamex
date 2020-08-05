# Build libraries.
#
# Here we prefer use of ExternalProject to add_subdirectory.  ExternalProject
# builds the library exactly the way we want it without polluting the parent
# CMake cache.
#
# Some stuff to remember about ExternalProject
#
# - BUILD_BYPRODUCTS is necessary for Ninja to pick up on the dependency.
# - In custom build steps, be explicit about both DEPENDEES and DEPENDERS or
#   else the build step might not be executed after a clean.

# Shorthand, to reduce line-noise
set(libprefix "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(libsuffix "${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(dllprefix "${CMAKE_SHARED_LIBRARY_PREFIX}")
set(dllsuffix "${CMAKE_SHARED_LIBRARY_SUFFIX}")

include(ExternalProject)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/local/include")

### libcurl ###

set(CURL_LIBRARY
  "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}curl-d${libsuffix}")

# Build the library as an ExternalProject.
ExternalProject_Add(curl-project
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/curl"
  CMAKE_ARGS
  "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
  "-DCMAKE_LINKER=${CMAKE_LINKER}"
  "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
  "-DBUILD_CURL_EXE=OFF"
  "-DBUILD_SHARED_LIBS=OFF"
  "-DCURL_ZLIB=OFF"
  "-DHTTP_ONLY=ON"
  BUILD_BYPRODUCTS ${CURL_LIBRARY}
  EXCLUDE_FROM_ALL ON)

# Synthesize an imported target that can be linked against.
add_library(curl STATIC IMPORTED GLOBAL)
add_dependencies(curl curl-project)
set_target_properties(curl PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/local/include"
  IMPORTED_LOCATION ${CURL_LIBRARY})

### JsonCpp ###

# Figure out the correct library path to attach to our imported target
set(JSONCPP_LIBRARY
  "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}jsoncpp${libsuffix}")

# Build the library as an ExternalProject.
ExternalProject_Add(jsoncpp-project
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp"
  CMAKE_ARGS
  "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
  "-DCMAKE_LINKER=${CMAKE_LINKER}"
  "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
  BUILD_BYPRODUCTS ${JSONCPP_LIBRARY}
  EXCLUDE_FROM_ALL ON)

# Synthesize an imported target that can be linked against.
add_library(jsoncpp STATIC IMPORTED GLOBAL)
add_dependencies(jsoncpp jsoncpp-project)
set_target_properties(jsoncpp PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS JSON_IS_AMALGAMATION
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/local/include"
  IMPORTED_LOCATION ${JSONCPP_LIBRARY})

### MiniUPnPc ###

# Figure out the correct library path to attach to our imported target
set(MINIUPNPC_LIBRARY
  "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}miniupnpc${libsuffix}")

# Build the library as an ExternalProject.
ExternalProject_Add(miniupnpc-project
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libminiupnpc"
  CMAKE_ARGS
  "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
  "-DCMAKE_LINKER=${CMAKE_LINKER}"
  "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}"
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/local"
  "-DUPNPC_BUILD_SHARED=No"
  "-DUPNPC_BUILD_TESTS=No"
  BUILD_BYPRODUCTS ${MINIUPNPC_LIBRARY}
  EXCLUDE_FROM_ALL ON)

# Ensure the destination include directory exists, or else CMake complains.
set(MINIUPNPC_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/local/include/miniupnpc")
file(MAKE_DIRECTORY "${MINIUPNPC_INCLUDE_DIR}")

# Synthesize an imported target that can be linked against.
add_library(upnpc-static STATIC IMPORTED GLOBAL)
add_dependencies(upnpc-static miniupnpc-project)
set_target_properties(upnpc-static PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${MINIUPNPC_INCLUDE_DIR}"
  IMPORTED_LOCATION ${MINIUPNPC_LIBRARY})
if(WIN32)
  set_target_properties(upnpc-static PROPERTIES INTERFACE_LINK_LIBRARIES "ws2_32;iphlpapi")
endif()

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

    # SDL target.
    add_library(SDL::SDL UNKNOWN IMPORTED GLOBAL)
    set_target_properties(SDL::SDL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SDL_INCLUDE_DIR}"
      IMPORTED_LOCATION "${SDL_LIBRARY}")

    # SDLmain target.
    add_library(SDL::SDLmain UNKNOWN IMPORTED GLOBAL)
    set_target_properties(SDL::SDLmain PROPERTIES
      IMPORTED_LOCATION "${SDLMAIN_LIBRARY}")
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
