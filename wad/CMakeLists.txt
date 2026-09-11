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

  get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if (IS_MULTI_CONFIG)
      set (WAD_OUTPUT_DIR "${ODAMEX_ARTIFACT_DIR}/$<CONFIG>")
  else()
      set (WAD_OUTPUT_DIR "${ODAMEX_ARTIFACT_DIR}")
  endif()

  # CMake >= 3.20 needed so that a potential embedded $<CONFIG> can work in OUTPUT below.
  add_custom_command(
    OUTPUT "${WAD_OUTPUT_DIR}/odamex.wad"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${WAD_OUTPUT_DIR}"
    COMMAND "${Python_EXECUTABLE}" odawad.py -o "${WAD_OUTPUT_DIR}/odamex.wad" wadinfo.txt
    DEPENDS ${ODAWAD_SOURCES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM)

  add_custom_target(odawad ALL
    DEPENDS "${WAD_OUTPUT_DIR}/odamex.wad")

  if(WIN32)
    install(FILES "${WAD_OUTPUT_DIR}/odamex.wad"
      DESTINATION .
      COMPONENT common)
  else()
    install(FILES "${WAD_OUTPUT_DIR}/odamex.wad"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/odamex"
      COMPONENT common)
  endif()
else()
  message(WARNING "Could NOT find Python, ODAMEX.WAD will not be built.")
endif()
