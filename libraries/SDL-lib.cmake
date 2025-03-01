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
          "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.6.2-VC.zip"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
          EXPECTED_HASH SHA256=7f050663ccc7911bb9c57b11e32ca79578b712490186b8645ddbbe4e7d2fe1c9)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.6.2")
        set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_DIR}/include" CACHE PATH "")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x64/SDL2_mixer.lib" CACHE FILEPATH "")
        else()
          set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x86/SDL2_mixer.lib" CACHE FILEPATH "")
        endif()
      else()
        file(DOWNLOAD
          "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.6.2-mingw.tar.gz"
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-mingw.tar.gz"
          EXPECTED_HASH SHA256=6c414d05a3b867e0d59e0f9b28ea7e5e64527e612ccf961735dc2478391315b3)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-mingw.tar.gz"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.6.2/x86_64-w64-mingw32")
        else()
          set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.6.2/i686-w64-mingw32")
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
