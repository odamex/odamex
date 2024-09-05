include(OdamexTargetSettings)

# Odalaunch
file(GLOB LAUNCHER_SOURCES "*.cpp" "*.h")
file(GLOB LAUNCHER_GUI_SOURCES "gui/*.cpp" "gui/*.h")

if(WIN32)
    list(APPEND LAUNCHER_SOURCES "win32/plat_win32.cpp")
else()
    list(APPEND LAUNCHER_SOURCES "posix/plat_posix.cpp")
endif()

# Tag source files
get_target_property(ODALPAPI_SOURCES odalpapi INTERFACE_SOURCES)
get_target_property(SQLITE_SOURCES sqlite-amalgamation INTERFACE_SOURCES)
source_group("Launcher" FILES ${LAUNCHER_SOURCES})
source_group("Launcher/GUI" FILES ${LAUNCHER_GUI_SOURCES})
source_group("Odalpapi" FILES ${ODALPAPI_SOURCES})
source_group("SQLite" FILES ${SQLITE_SOURCES})

add_executable(odalaunch-fltk
    MACOSX_BUNDLE WIN32
    ${LAUNCHER_SOURCES} ${LAUNCHER_GUI_SOURCES})
odamex_target_settings(odalaunch-fltk)
target_compile_definitions(odalaunch-fltk PUBLIC _UNICODE UNICODE)
target_include_directories(odalaunch-fltk PUBLIC "${CMAKE_BINARY_DIR}/libraries/local/include")
target_include_directories(odalaunch-fltk PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(odalaunch-fltk
    odalpapi fmt::fmt JsonCpp::JsonCpp sqlite-amalgamation fltk::fltk)
