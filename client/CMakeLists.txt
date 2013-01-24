# Common
set(COMMON_DIR ../common)
file(GLOB COMMON_HEADERS ${COMMON_DIR}/*.h)
file(GLOB COMMON_SOURCES ${COMMON_DIR}/*.cpp)

# Textscreen
set(TEXTSCREEN_DIR ../textscreen)
file(GLOB TEXTSCREEN_HEADERS ${TEXTSCREEN_DIR}/*.h)
file(GLOB TEXTSCREEN_SOURCES ${TEXTSCREEN_DIR}/*.c)

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

# Platform definitions
define_platform()

# Client definitions
add_definitions(-DNOASM)
include_directories(${COMMON_DIR} ${TEXTSCREEN_DIR} ${CLIENT_DIR})

# SDL configuration
find_package(SDL)
if(SDL_FOUND)
  include_directories(${SDL_INCLUDE_DIR})
else()
  message(WARNING "SDL not found, client target not generated.")
endif()

# SDL_Mixer configuration
find_package(SDL_mixer)
if(SDLMIXER_FOUND)
  include_directories(${SDLMIXER_INCLUDE_DIR})
else()
  message(WARNING "SDL_mixer not found, client target not generated.")
endif()

# Portmidi configuration
find_package(PortMidi)
if(PORTMIDI_FOUND)
  include_directories(${PORTMIDI_INCLUDE_DIR})
  add_definitions(-DPORTMIDI)
else()
  message(WARNING "PortMidi not found, client will not be built with PortMidi support.")
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
  if(PORTMIDI_FOUND)
    target_link_libraries(odamex ${PORTMIDI_LIBRARIES})
  endif()
  if(WIN32)
    target_link_libraries(odamex winmm wsock32 Shlwapi)
  elseif(APPLE)
    target_link_libraries(odamex ${APPLE_FRAMEWORKS})
  elseif(SOLARIS)
    target_link_libraries(odamex socket nsl)
  endif()

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
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/odamex.wad ${CMAKE_CURRENT_BINARY_DIR}/odamex.app/Contents/MacOS
    )

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

    string(REPLACE "-framework Cocoa" "" SDL_DIR ${SDL_LIBRARY})
    string(REPLACE "-framework Cocoa" "" SDLMIXER_DIR ${SDLMIXER_LIBRARY})

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

  endif()
endif()
