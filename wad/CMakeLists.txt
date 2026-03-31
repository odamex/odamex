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

macro(odamex_copy_wad _target)
  if (TARGET ${_target})
    add_custom_command(
        APPEND
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad"
        DEPENDS ${_target}
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad" $<TARGET_FILE_DIR:${_target}>
        )
  endif()
endmacro()

# If Python is available, use it to build the WAD.
if(Python_FOUND)
  message(STATUS "Found Python: ${Python_EXECUTABLE}")

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad"
    COMMAND "${Python_EXECUTABLE}" odawad.py -o "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad" wadinfo.txt
    DEPENDS ${ODAWAD_SOURCES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM)

  odamex_copy_wad(odamex)
  odamex_copy_wad(odasrv)

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
