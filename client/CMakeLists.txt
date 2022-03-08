include(OdamexCopyLibs)
include(OdamexCopyWad)
include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

# Enable default SIMD settings for standard builds.
option(USE_DEFAULT_SIMD "Use default SIMD optimization settings" ON)

# #define CLIENT_APP for use in the source code
add_definitions(-DCLIENT_APP)

# Client
set(CLIENT_DIR sdl src)
file(GLOB CLIENT_SOURCES src/*.cpp src/*.h)
file(GLOB CLIENT_SDL_SOURCES sdl/*.cpp sdl/*.h)
if(WIN32)
  configure_file(sdl/client.rc.in "${CMAKE_CURRENT_BINARY_DIR}/client.rc" @ONLY)
  list(APPEND CLIENT_SDL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/client.rc")
endif()

# Include console folders if compiling 
if (GCONSOLE)
  if (NSWITCH)
    set(CLIENT_DIR ${CLIENT_DIR} switch)  # Nintendo Switch
    file(GLOB CLIENT_HEADERS ${CLIENT_HEADERS} switch/*.h)
    file(GLOB CLIENT_SOURCES ${CLIENT_SOURCES} switch/*.cpp)
    odamex_target_settings_nx(CLIENT_DIR GLOB)
  elseif (VWII)
    set(CLIENT_DIR ${CLIENT_DIR} vwii)  # Nintendo Wii/vWII
    file(GLOB CLIENT_HEADERS ${CLIENT_HEADERS} vwii/*.h)
    file(GLOB CLIENT_SOURCES ${CLIENT_SOURCES} vwii/*.cpp)
  endif()
endif()

if(NOT GCONSOLE)
  # Only non-consoles can use the client GUI.
  file(GLOB CLIENT_GUI_SOURCES gui/*.cpp gui/*.h)
endif()

# Tag source files with correct directories
get_target_property(COMMON_SOURCES odamex-common INTERFACE_SOURCES)
source_group("Client" FILES ${CLIENT_SOURCES})
source_group("Common" FILES ${COMMON_SOURCES})
source_group("SDL" FILES ${CLIENT_SDL_SOURCES})
source_group("GUI" FILES ${CLIENT_GUI_SOURCES})

# Client definitions
include_directories(${COMMON_DIR} ${CLIENT_DIR})

# Make sure we don't find libraries that are in Program Files/Odamex
if(WIN32)
  list(REMOVE_ITEM CMAKE_SYSTEM_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})
endif()

# X11 configuration
if(UNIX AND NOT APPLE)
  find_package(X11)
  if(X11_FOUND)
    add_definitions(-DX11)
    include_directories(${X11_INCLUDE_DIR})
    link_libraries(${X11_LIBRARIES})
  endif()
endif()

# PortMidi configuration

# PortMidi
if(ENABLE_PORTMIDI)
  find_package(PortMidi)
  if(NOT PORTMIDI_FOUND)
    # build the PortMidi library
    set(PORTMIDI_LIBRARY "portmidi")
    set(PORTMIDI_LIBRARIES ${PORTMIDI_LIBRARY})
    set(PORTMIDI_DIR ../libraries/portmidi)
    set(PORTMIDI_INCLUDE_DIR ${PORTMIDI_DIR}/pm_common/ ${PORTMIDI_DIR}/porttime/)  

    if(APPLE)
      find_library(COREFOUNDATION_LIBRARY CoreFoundation)
      find_library(COREAUDIO_LIB CoreAudio)
      find_library(COREMIDI_LIB CoreMIDI)
      find_library(CORESERVICES_LIB CoreServices)

      set(PM_NEEDED_LIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB} ${COREMIDI_LIB} ${CORESERVICES_LIB} CACHE INTERNAL "")
      file(GLOB PORTMIDI_PLATFORM_HEADERS ${PORTMIDI_DIR}/pm_mac/*.h)
      file(GLOB PORTMIDI_PLATFORM_SOURCES ${PORTMIDI_DIR}/pm_mac/*.c)
      list(APPEND PORTMIDI_PLATFORM_SOURCES ${PORTMIDI_DIR}/porttime/ptmacosx_mach.c)
    elseif(WIN32)
      set(PM_NEEDED_LIBS winmm.lib)
      file(GLOB PORTMIDI_PLATFORM_HEADERS ${PORTMIDI_DIR}/pm_win/*.h)
      file(GLOB PORTMIDI_PLATFORM_SOURCES ${PORTMIDI_DIR}/pm_win/*.c)
      list(APPEND PORTMIDI_PLATFORM_SOURCES ${PORTMIDI_DIR}/porttime/ptwinmm.c)
    elseif(UNIX)
      set(LINUX_FLAGS "-DPMALSA")
      set(PM_NEEDED_LIBS pthread asound)
      file(GLOB PORTMIDI_PLATFORM_HEADERS ${PORTMIDI_DIR}/pm_linux/*.h)
      file(GLOB PORTMIDI_PLATFORM_SOURCES ${PORTMIDI_DIR}/pm_linux/*.c)
      list(APPEND PORTMIDI_PLATFORM_SOURCES ${PORTMIDI_DIR}/porttime/ptlinux.c)
    endif()

    file(GLOB PORTMIDI_HEADERS ${PORTMIDI_DIR}/pm_common/*.h)
    list(APPEND PORTMIDI_HEADERS ${PORTMIDI_DIR}/porttime/porttime.h)
    list(APPEND PORTMIDI_HEADERS ${PORTMIDI_PLATFORM_HEADERS})

    file(GLOB PORTMIDI_SOURCES ${PORTMIDI_DIR}/pm_common/*.c)
    list(APPEND PORTMIDI_SOURCES ${PORTMIDI_DIR}/porttime/porttime.c)
    list(APPEND PORTMIDI_SOURCES ${PORTMIDI_PLATFORM_SOURCES})

    add_library(${PORTMIDI_LIBRARY} STATIC ${PORTMIDI_SOURCES} ${PORTMIDI_HEADERS})
    target_link_libraries(${PORTMIDI_LIBRARY} ${PM_NEEDED_LIBS})
    message(STATUS "portmidi will be built and staticaly linked when compiling the client application.")
  endif()

  add_definitions(-DPORTMIDI)
  include_directories(${PORTMIDI_INCLUDE_DIR})
endif()

# Find Mac frameworks
if(APPLE)
  find_library(APPKIT_LIBRARY AppKit)
  find_library(CARBON_LIBRARY Carbon)
  find_library(AUDIOTBOX_LIBRARY AudioToolbox)
  find_library(AUDIOUNIT_LIBRARY AudioUnit)
  set(APPLE_FRAMEWORKS ${APPKIT_LIBRARY} ${CARBON_LIBRARY} ${AUDIOTBOX_LIBRARY}
    ${AUDIOUNIT_LIBRARY})
endif()

# Set up FHS installation path
if(NOT APPLE AND NOT WIN32)
  add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
endif()

# Client target
if(TARGET SDL2::SDL2 OR TARGET SDL::SDL)

  if (NOT GCONSOLE)
    # Textscreen
    set(TEXTSCREEN_LIBRARY "textscreen")
    set(TEXTSCREEN_DIR ../libraries/textscreen)
    set(TEXTSCREEN_HEADERS ${TEXTSCREEN_DIR}/txt_sdl.h ${TEXTSCREEN_DIR}/txt_main.h)
    set(TEXTSCREEN_SOURCES ${TEXTSCREEN_DIR}/txt_sdl.c)

    include_directories(${TEXTSCREEN_DIR})
    add_library(${TEXTSCREEN_LIBRARY} STATIC ${TEXTSCREEN_SOURCES} ${TEXTSCREEN_HEADERS})
  endif()

  if(TARGET SDL::SDL)
    target_link_libraries(${TEXTSCREEN_LIBRARY} SDL::SDL)
  else()
    target_link_libraries(${TEXTSCREEN_LIBRARY} SDL2::SDL2)
  endif()

  if (NOT GCONSOLE)
    target_include_directories(${TEXTSCREEN_LIBRARY} PRIVATE ../common)
  else()
    include_directories(../common)
  endif()

  add_executable(odamex MACOSX_BUNDLE WIN32
    ${CLIENT_SOURCES} ${CLIENT_SDL_SOURCES} ${COMMON_SOURCES} ${CLIENT_GUI_SOURCES})
  odamex_target_settings(odamex)
  set_property(TARGET odamex PROPERTY CXX_STANDARD 98)

  if(USE_DEFAULT_SIMD)
    if(ODAMEX_TARGET_ARCH STREQUAL "amd64")
      message(STATUS "Default SIMD flags not touched for AMD64")
    elseif(ODAMEX_TARGET_ARCH STREQUAL "i386")
      if(NOT MSVC)
        # Pentium M has SSE2.
        target_compile_definitions(odamex PRIVATE -march=pentium-m)
      else()
        target_compile_definitions(odamex PRIVATE /arch:SSE2)
      endif()
      message(STATUS "Default SIMD flags set to SSE2")
    elseif(ODAMEX_TARGET_ARCH MATCHES "ppc")
      target_compile_definitions(odamex PRIVATE -faltivec)
      message(STATUS "Default SIMD flags set to AltiVec")
    endif()
  else()
    message(STATUS "Default SIMD flags not used on user request")
  endif()

  if(MSVC)
    # We supply our own manifest, don't use the default one.
    set_target_properties(odamex PROPERTIES LINK_FLAGS "/MANIFEST:NO")
  endif()

  target_link_libraries(odamex ${TEXTSCREEN_LIBRARY})
  target_link_libraries(odamex odamex-common odaproto)
  if(TARGET SDL::SDL)
    target_link_libraries(odamex SDL::SDL)
    if(TARGET SDL::SDLmain)
      target_link_libraries(odamex SDL::SDLmain)
    endif()
    target_link_libraries(odamex SDL::mixer)
  else()
    target_link_libraries(odamex SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
      target_link_libraries(odamex SDL2::SDL2main)
    endif()
    target_link_libraries(odamex SDL2::mixer)
  endif()
  target_link_libraries(odamex ${PNG_LIBRARY} ${ZLIB_LIBRARY} CURL::libcurl)
  if(NOT GCONSOLE)
    target_include_directories(odamex PRIVATE gui)
    target_link_libraries(odamex fltk fltk_images)
  endif()

  if(ENABLE_PORTMIDI)
    target_link_libraries(odamex ${PORTMIDI_LIBRARY})
  endif()

  target_link_libraries(odamex ZLIB::ZLIB PNG::PNG fmt::fmt)

  if(WIN32)
    target_link_libraries(odamex winmm wsock32 shlwapi)
  elseif(APPLE)
    target_link_libraries(odamex ${APPLE_FRAMEWORKS})
  elseif(NSWITCH)
    target_link_libraries(odamex SDL2 mpg123 modplug vorbisfile vorbis opusfile opus ogg EGL glapi drm_nouveau nx m)
  elseif(VWII)
    # Ch0wW : this is UGLY, please fix it ASAP !!
    target_link_libraries(odamex -L$ENV{DEVKITPRO}/libogc/lib/wii -L$ENV{DEVKITPRO}/portlibs/ppc/lib -L$ENV{DEVKITPRO}/portlibs/wii/lib)
    target_link_libraries(odamex SDL_mixer SDL smpeg png png16 vorbisidec z fat wiiuse bte ogc ogg m wiikeyboard)
  elseif(SOLARIS)
    target_link_libraries(odamex socket nsl)
  endif()

  if(UNIX AND NOT APPLE)
    target_link_libraries(odamex rt)
    if(X11_FOUND)
      target_link_libraries(odamex X11)
    endif()
  endif()

  odamex_copy_libs(odamex)
  odamex_copy_wad(odamex)

  set(MACOSX_BUNDLE_BUNDLE_NAME ${CMAKE_PROJECT_NAME})
  set(MACOSX_BUNDLE_INFO_STRING "ODAMEX and its likeness © ${PROJECT_COPYRIGHT} Odamex team.")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_ICON_FILE odamex.icns)

  if(APPLE)
    add_custom_command(
      TARGET odamex
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/Resources
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/media/${MACOSX_BUNDLE_ICON_FILE} ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/Resources
    )
    if(TARGET odawad)
      add_custom_command(
        TARGET odamex
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/wad/odamex.wad ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/MacOS)
    endif()

    set_directory_properties(
      PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
      "${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE};${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/MacOS/odamex.wad"
    )

    #****
    # The remaining commands are issued on 'make install'. They make the app
    # bundle "stand-alone" by packaging dependencies inside.
    #****

    # Install into bundle under client directory
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

    # if(SDL2_FOUND AND SDL2_MIXER_FOUND)
    #     string(REPLACE "-framework Cocoa" "" SDL_DIR ${SDL2_LIBRARY})
    #     string(REPLACE "-framework Cocoa" "" SDLMIXER_DIR ${SDL2_MIXER_LIBRARY})
    # else(SDL2_FOUND AND SDL2_MIXER_FOUND)
    #     string(REPLACE "-framework Cocoa" "" SDL_DIR ${SDL_LIBRARY})
    #     string(REPLACE "-framework Cocoa" "" SDLMIXER_DIR ${SDLMIXER_LIBRARY})
    # endif()

    # Install the SDL framework into the bundle
    if(IS_DIRECTORY ${SDL_DIR})
      install(DIRECTORY ${SDL_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/Frameworks)
      install(CODE "execute_process(COMMAND
        install_name_tool -change @rpath/SDL.framework/Versions/A/SDL
        @executable_path/../Frameworks/SDL.framework/Versions/A/SDL
        ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/MacOS/odamex)
        " )
    endif()

    # Install the SDL_mixer framework into the bundle
    if(IS_DIRECTORY ${SDLMIXER_DIR})
      install(DIRECTORY ${SDLMIXER_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/Frameworks)
    endif()

    # If PortMidi was built in install the PortMidi library into the bundle
    if(PortMidi_FOUND)
      install(FILES ${PORTMIDI_LIBRARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/Frameworks/)
      install(CODE "execute_process(COMMAND
        install_name_tool -change libportmidi.dylib
        @executable_path/../Frameworks/libportmidi.dylib
        ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/MacOS/odamex)
        " )
    endif()
  elseif(NSWITCH)
    odamex_target_postcompile_nx(odamex)
  elseif(VWII)
    message(${CMAKE_BINARY_DIR})
    add_custom_command( TARGET odamex POST_BUILD
    COMMAND elf2dol ${CMAKE_BINARY_DIR}/client/odamex ${CMAKE_BINARY_DIR}/boot.dol )
  elseif(WIN32)
    install(TARGETS odamex
      RUNTIME DESTINATION .
      COMPONENT client)
  else()
    install(TARGETS odamex
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT client)
  endif()
endif()

if(BUILD_OR_FAIL AND NOT TARGET odamex)
  message(FATAL_ERROR "Odamex target could not be generated")
endif()
