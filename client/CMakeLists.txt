include(OdamexCopyLibs)
include(OdamexCvarDoc)
include(OdamexTargetSettings)

# use unquoted #defines
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

if(APPLE)
  set(CMAKE_FIND_FRAMEWORK LAST)
  set(CMAKE_FIND_APPBUNDLE LAST)
endif()

# Enable default SIMD settings for standard builds.
option(USE_DEFAULT_SIMD "Use default SIMD optimization settings" ON)

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
  endif()
endif()

if(NOT GCONSOLE)
  # Only non-consoles can use the client GUI.
  file(GLOB CLIENT_GUI_SOURCES gui/*.cpp gui/*.h)
endif()

# Tag source files with correct directories
source_group("Client" FILES ${CLIENT_SOURCES})
source_group("SDL" FILES ${CLIENT_SDL_SOURCES})
source_group("GUI" FILES ${CLIENT_GUI_SOURCES})

get_target_property(COMMON_DIRS     odacommon INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(COMMON_SOURCES  odacommon INTERFACE_SOURCES)

source_group("Common" FILES ${COMMON_SOURCES})

# Make sure we don't find libraries that are in Program Files/Odamex
if(WIN32)
  list(REMOVE_ITEM CMAKE_SYSTEM_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})
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
      add_definitions(-DPMALSA)
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
    message(STATUS "portmidi will be built and statically linked when compiling the client application.")
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

# Client target
if(TARGET SDL2::SDL2 OR TARGET SDL::SDL)

  if (NOT GCONSOLE)
    # Textscreen
    set(TEXTSCREEN_LIBRARY "textscreen")
    set(TEXTSCREEN_DIR ../libraries/textscreen)
    set(TEXTSCREEN_HEADERS ${TEXTSCREEN_DIR}/txt_sdl.h ${TEXTSCREEN_DIR}/txt_main.h)
    set(TEXTSCREEN_SOURCES ${TEXTSCREEN_DIR}/txt_sdl.cpp)

    add_library(${TEXTSCREEN_LIBRARY} STATIC ${TEXTSCREEN_SOURCES} ${TEXTSCREEN_HEADERS})
    target_include_directories(${TEXTSCREEN_LIBRARY}
        PUBLIC
            ${TEXTSCREEN_DIR}
        PRIVATE
            ${COMMON_DIRS}
        )
  endif()

  if(TARGET SDL::SDL)
    target_link_libraries(${TEXTSCREEN_LIBRARY} SDL::SDL)
  else()
    target_link_libraries(${TEXTSCREEN_LIBRARY} SDL2::SDL2)
  endif()

  add_executable(odamex MACOSX_BUNDLE WIN32
    ${CLIENT_SOURCES} ${CLIENT_SDL_SOURCES} ${CLIENT_GUI_SOURCES})
  odamex_target_settings(TARGET odamex)
  odamex_target_errors(odamex)

  target_link_libraries(odamex odacommon)
  target_include_directories(odamex PRIVATE src sdl)
  target_compile_definitions(odamex PRIVATE CLIENT_APP)

  if(USE_DEFAULT_SIMD)
    if(ODAMEX_TARGET_ARCH STREQUAL "amd64")
      message(STATUS "Default SIMD flags not touched for AMD64")
    elseif(ODAMEX_TARGET_ARCH STREQUAL "i386")
      if(NOT MSVC)
        target_compile_options(odamex PRIVATE -msse2)
      else()
        target_compile_options(odamex PRIVATE /arch:SSE2)
      endif()
      message(STATUS "Default SIMD flags set to SSE2")
    elseif(ODAMEX_TARGET_ARCH MATCHES "ppc")
      if(APPLE)
        target_compile_options(odamex PRIVATE -faltivec)
      else()
        target_compile_options(odamex PRIVATE -maltivec)
      endif()
      message(STATUS "Default SIMD flags set to AltiVec")
    endif()
  else()
    message(STATUS "Default SIMD flags not used on user request")
  endif()

  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "GNU Detected, forcing GCC to link math.h")
    target_compile_options(odamex PRIVATE -lm)
  endif()

  if(MSVC)
    # We supply our own manifest, don't use the default one.
    set_target_properties(odamex PROPERTIES LINK_FLAGS "/MANIFEST:NO")
  endif()

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

  if(NOT GCONSOLE)
    target_include_directories(odamex PRIVATE gui)
    if(USE_INTERNAL_FLTK)
      target_link_libraries(odamex fltk::fltk fltk::images)
    else()
      find_package(FLTK 1.4 CONFIG REQUIRED)
      target_link_libraries(odamex fltk::fltk-shared fltk::images-shared)
    endif()
  endif()

  if(ENABLE_PORTMIDI)
    target_link_libraries(odamex ${PORTMIDI_LIBRARY})
  endif()

  target_link_libraries(odamex ${TEXTSCREEN_LIBRARY} PNG::PNG libADLMIDI::ADLMIDI_static curl_interface)

  if(NSWITCH)
    target_link_libraries(odamex SDL2 mpg123 modplug vorbisfile vorbis opusfile opus ogg EGL glapi drm_nouveau nx m)
  endif()

  if (TARGET odawad)
      add_dependencies(odamex odawad)
  endif()

  odamex_copy_libs(odamex)

  set(MACOSX_BUNDLE_BUNDLE_NAME ${CMAKE_PROJECT_NAME})
  set(MACOSX_BUNDLE_COPYRIGHT "ODAMEX and its likeness © ${PROJECT_COPYRIGHT} Odamex team.")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_ICON_FILE odamex.icns)
  set(MACOSX_BUNDLE_GUI_IDENTIFIER net.odamex.odamex)

  if(APPLE)
    target_link_libraries(odamex ${APPLE_FRAMEWORKS})

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
        COMMAND ${CMAKE_COMMAND} -E copy "${ODAMEX_ARTIFACT_DIR}/odamex.wad" ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/MacOS)
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
  elseif(WIN32)
    install(TARGETS odamex
      RUNTIME DESTINATION .
      COMPONENT client)
  else()
    install(TARGETS odamex
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT client)
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/net.odamex.Odamex.Client.desktop
      DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
      COMPONENT client)
    install(FILES ${CMAKE_SOURCE_DIR}/media/icon_odamex_512.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps
      RENAME net.odamex.Odamex.Client.png
      COMPONENT client)
    install(FILES ${CMAKE_SOURCE_DIR}/media/icon_odademo_512.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/mimetypes
      RENAME net.odamex.Odamex-application-odamex-demo.png
      COMPONENT client)
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/net.odamex.Odamex-mime.xml
      DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages
      COMPONENT client)
  endif()
endif()

if(TARGET odamex)
  odamex_cvardoc_refresh(odamex)
endif()

if(BUILD_OR_FAIL AND NOT TARGET odamex)
  message(FATAL_ERROR "Odamex target could not be generated")
endif()
