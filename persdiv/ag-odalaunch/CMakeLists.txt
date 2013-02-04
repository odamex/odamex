include(FindLibAgar)

# Odalaunch
set(LAUNCH_DIR src)
file(GLOB LAUNCH_HEADERS src/*.h)
file(GLOB LAUNCH_SOURCES src/*.cpp)

# AG-Odalaunch definitions
include_directories(${LAUNCH_DIR})

find_package(LibAgar)
if(LibAgar_FOUND)
  include_directories(${LibAgar_INCLUDE_DIRS})
  add_definitions(${LibAgar_DEFINITIONS})
  message("-- Agar libraries found, ag-odalaunch target generated.")
endif()

# AG-Odalaunch target
if(LibAgar_FOUND)
  add_custom_command(
    TARGET ag-odalaunch
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/res
  )
  add_executable(ag-odalaunch MACOSX_BUNDLE WIN32
    ${LAUNCH_SOURCES} ${LAUNCH_HEADERS}
  )
  target_link_libraries(ag-odalaunch ${LibAgar_LIBRARIES})

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
