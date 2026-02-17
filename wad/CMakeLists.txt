find_package(Python)

file(GLOB_RECURSE ODAWAD_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/wadinfo.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/bootstrap/doom2.wad
  ${CMAKE_CURRENT_SOURCE_DIR}/flats/*.png
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/*.png
  ${CMAKE_CURRENT_SOURCE_DIR}/lumps/*.lmp
  ${CMAKE_CURRENT_SOURCE_DIR}/lumps/*.ogg
  ${CMAKE_CURRENT_SOURCE_DIR}/lumps/*.wav
  ${CMAKE_CURRENT_SOURCE_DIR}/sprites/*.png)

# If Python is available, use it to build the WAD.
if(Python_FOUND)
  message(STATUS "Found Python: ${Python_EXECUTABLE}")

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad"
    COMMAND "${Python_EXECUTABLE}" odawad.py -o "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad" wadinfo.txt
    DEPENDS ${ODAWAD_SOURCES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM)

  add_custom_target(odawad ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad")

  if(WIN32)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad"
      DESTINATION .
      COMPONENT common)
  else()
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/odamex"
      COMPONENT common)
  endif()
else()
  message(WARNING "Could NOT find Python, ODAMEX.WAD will not be built.")
endif()
