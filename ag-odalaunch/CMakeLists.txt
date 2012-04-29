cmake_policy(SET CMP0004 OLD)

# Odalaunch
set(LAUNCH_DIR src)
file(GLOB LAUNCH_HEADERS src/*.h)
file(GLOB LAUNCH_SOURCES src/*.cpp)

# AG-Odalaunch definitions
include_directories(${LAUNCH_DIR})

FIND_PROGRAM(AGAR_CONFIG agar-config)
if(AGAR_CONFIG)
  execute_process(COMMAND ${AGAR_CONFIG} --cflags OUTPUT_VARIABLE AGAR_CFLAGS)
  execute_process(COMMAND ${AGAR_CONFIG} --libs OUTPUT_VARIABLE AGAR_LIBRARIES)
  add_definitions(${AGAR_CFLAGS})
else()
  message(WARNING "Agar libraries not found, ag-odalaunch target not generated.")
endif()

find_package(SDL_image)
if(SDLIMAGE_FOUND)
  include_directories(${SDLIMAGE_INCLUDE_DIR})
else()
  message(WARNING "SDL_image not found, ag-odalaunch target not generated.")
endif()

# AG-Odalaunch target
if(AGAR_CONFIG AND SDLIMAGE_FOUND)
  add_custom_command(
    TARGET ag-odalaunch
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/res
  )
  add_executable(ag-odalaunch MACOSX_BUNDLE WIN32
    ${LAUNCH_SOURCES} ${LAUNCH_HEADERS}
  )
  target_link_libraries(ag-odalaunch ${AGAR_LIBRARIES} ${SDLIMAGE_LIBRARY})

  set(MACOSX_BUNDLE_BUNDLE_NAME AG-OdaLaunch)
  set(MACOSX_BUNDLE_INFO_STRING "ODAMEX and its likeness © ${PROJECT_COPYRIGHT} Odamex team.")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_ICON_FILE odalaunch.icns)

  if(APPLE)
    add_custom_command(
      TARGET ag-odalaunch
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/ag-odalaunch.app/Contents/Resources
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_SOURCE_DIR}/media/${MACOSX_BUNDLE_ICON_FILE} ${CMAKE_CURRENT_BINARY_DIR}/ag-odalaunch.app/Contents/Resources/
    )

    set_directory_properties(
      PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
      "${CMAKE_CURRENT_BINARY_DIR}/ag-odalaunch.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE}"
    )
  endif()
endif()
