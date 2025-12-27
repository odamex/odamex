file(GLOB_RECURSE ODAWAD_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/wadinfo.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/bootstrap/doom2.wad
  ${CMAKE_CURRENT_SOURCE_DIR}/flats/-noflat-.gif
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/*.gif
  ${CMAKE_CURRENT_SOURCE_DIR}/lumps/*.lmp
  ${CMAKE_CURRENT_SOURCE_DIR}/sprites/*.gif)

if(USE_INTERNAL_DEUTEX)
  if(WIN32)
    # In our case, our "interal" DeuTex is downloaded from github.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/deutex")
    file(DOWNLOAD
      "https://github.com/Doom-Utils/deutex/releases/download/v5.2.3/deutex-5.2.3_w32.zip"
      "${CMAKE_CURRENT_BINARY_DIR}/deutex-5.2.3_w32.zip"
      EXPECTED_HASH SHA256=9c07f354fd81384a0e91844b7c152114da1a3566343de846db443de38b82a1ad)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
      "${CMAKE_CURRENT_BINARY_DIR}/deutex-5.2.3_w32.zip"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/deutex")
    find_program(DEUTEX_EXECUTABLE deutex
      PATHS "${CMAKE_CURRENT_BINARY_DIR}/deutex"
      NO_DEFAULT_PATH)
  else()
    # Check if deutex is unzipped in the source dir and if it is, bring it over
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deutex")
      file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/deutex"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
      execute_process(COMMAND sh bootstrap
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/deutex")
      execute_process(COMMAND sh configure
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/deutex")
      execute_process(COMMAND make
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/deutex")
      find_program(DEUTEX_EXECUTABLE deutex
        PATHS "${CMAKE_CURRENT_BINARY_DIR}/deutex/src"
        NO_DEFAULT_PATH)
    endif()
  endif()
else()
  find_program(DEUTEX_EXECUTABLE deutex
    PATHS "/usr/games") # Debian installation location
endif()

# If DeuTex is available, use it to build the WAD.
if(DEUTEX_EXECUTABLE)
  message(STATUS "Found DeuTex: ${DEUTEX_EXECUTABLE}")

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad"
    COMMAND "${DEUTEX_EXECUTABLE}" -overwrite -rgb 0 255 255 -doom2 bootstrap -build wadinfo.txt "${CMAKE_CURRENT_BINARY_DIR}/odamex.wad"
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
  message(WARNING "Could NOT find DeuTex, ODAMEX.WAD will not be built.")
endif()
