# Common
set(COMMON_DIR ../common)
file(GLOB COMMON_HEADERS ${COMMON_DIR}/*.h)
file(GLOB COMMON_SOURCES ${COMMON_DIR}/*.cpp)

# Textscreen
set(TEXTSCREEN_DIR ../textscreen)
file(GLOB TEXTSCREEN_HEADERS ${TEXTSCREEN_DIR}/*.h)
file(GLOB TEXTSCREEN_SOURCES ${TEXTSCREEN_DIR}/*.cpp)

# Client
set(CLIENT_DIR sdl src)
file(GLOB CLIENT_HEADERS sdl/*.h src/*.h)
file(GLOB CLIENT_SOURCES sdl/*.cpp src/*.cpp)
if(WIN32)
  set(CLIENT_WIN32_RESOURCES sdl/client.rc)
endif()
if(APPLE)
  set(CLIENT_MAC_OBJC_SOURCES sdl/SDLMain.m)
endif()

# Client definitions
add_definitions(-DNOASM)
include_directories(${COMMON_DIR} ${TEXTSCREEN_DIR} ${CLIENT_DIR})

# SDL configuration
set(SDLDIR $ENV{SDLDIR} CACHE PATH
  "Path to SDL (and optionally SDL_mixer) base directory.  Only necessary if you get errors about SDL not being found, or if you want to use a different installation of SDL.")
set(ENV{SDLDIR} ${SDLDIR})
find_package(SDL)
if(SDL_FOUND)
  include_directories(${SDL_INCLUDE_DIR})
else()
  message(WARNING "SDL not found, client target not generated.")
endif()

# SDL_Mixer configuration
set(SDLMIXERDIR $ENV{SDLMIXERDIR} CACHE PATH
  "Path to SDL_mixer base directory.  Only necessary if you get errors about SDL not being found, or if you want to use a different installation of SDL_mixer.  If SDL_mixer is in the location specified by SDLMAIN then you can leave this blank.")
set(ENV{SDLMIXERDIR} ${SDLMIXERDIR})
find_package(SDL_mixer)
if(SDLMIXER_FOUND)
  include_directories(${SDLMIXER_INCLUDE_DIR})
else()
  message(WARNING "SDL_mixer not found, client target not generated.")
endif()

# Find Mac frameworks
if(APPLE)
  find_library(CARBON_LIBRARY Carbon)
  find_library(AUDIOTBOX_LIBRARY AudioToolbox)
  find_library(AUDIOUNIT_LIBRARY AudioUnit)
  set(APPLE_FRAMEWORKS ${CARBON_LIBRARY} ${AUDIOTBOX_LIBRARY}
    ${AUDIOUNIT_LIBRARY})
endif()

# Client target
if(SDL_FOUND AND SDLMIXER_FOUND)
  add_executable(odamex MACOSX_BUNDLE WIN32
    ${COMMON_SOURCES} ${COMMON_HEADERS}
    ${TEXTSCREEN_SOURCES} ${TEXTSCREEN_HEADERS}
    ${CLIENT_SOURCES} ${CLIENT_HEADERS} ${CLIENT_WIN32_RESOURCES}
    ${CLIENT_MAC_OBJC_SOURCES})
  target_link_libraries(odamex ${SDL_LIBRARY} ${SDLMIXER_LIBRARY})
  if(WIN32)
    target_link_libraries(odamex winmm wsock32)
  elseif(APPLE)
    target_link_libraries(odamex ${APPLE_FRAMEWORKS})
  endif()

  set(MACOSX_BUNDLE_BUNDLE_NAME ${CMAKE_PROJECT_NAME})
  set(MACOSX_BUNDLE_INFO_STRING "ODAMEX and its likeness © ${PROJECT_COPYRIGHT} Odamex team.")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_ICON_FILE odamex.icns)

  if(APPLE)
    # Install into bundle under client directory
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

    string(REPLACE "-framework Cocoa" "" SDL_DIR ${SDL_LIBRARY})
    string(REPLACE "-framework Cocoa" "" SDLMIXER_DIR ${SDLMIXER_LIBRARY})

    install(DIRECTORY ${SDL_DIR} DESTINATION odamex.app/Contents/Frameworks)
    install(DIRECTORY ${SDLMIXER_DIR} DESTINATION odamex.app/Contents/Frameworks)
    install(FILES ${CMAKE_SOURCE_DIR}/media/${MACOSX_BUNDLE_ICON_FILE} DESTINATION odamex.app/Contents/Resources)
    install(FILES ${CMAKE_SOURCE_DIR}/odamex.wad DESTINATION odamex.app/Contents/MacOS)
    install(CODE "execute_process(COMMAND 
      install_name_tool -change @rpath/SDL.framework/Versions/A/SDL
      @executable_path/../Frameworks/SDL.framework/Versions/A/SDL
      odamex.app/Contents/MacOS/odamex)
      " )
  endif()
endif()
