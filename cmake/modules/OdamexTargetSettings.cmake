function(odamex_target_settings TARGET)
  set(ODAMEX_DLLS "")

  if(MINGW)
    # MinGW builds require a bunch of extra libraries.
    get_filename_component(MINGW_DLL_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    # Chocolatey tries to be cute and wraps all the binaries so they don't need
    # their DLL files, so we special-case that here.
    if(NOT EXISTS "${MINGW_DLL_DIR}/libgcc_s_seh-1.dll")
      get_filename_component(MINGW_DLL_DIR
        "${MINGW_DLL_DIR}/../lib/mingw/tools/install/mingw64/bin"
        REALPATH BASE_DIR "${CMAKE_BINARY_DIR}"
      )
    endif()

    list(APPEND ODAMEX_DLLS
      "${MINGW_DLL_DIR}/libgcc_s_seh-1.dll"
      "${MINGW_DLL_DIR}/libstdc++-6.dll"
      "${MINGW_DLL_DIR}/libwinpthread-1.dll"
    )
  endif()

  # Copy library files to target directory.
  foreach(ODAMEX_DLL ${ODAMEX_DLLS})
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${ODAMEX_DLL}" $<TARGET_FILE_DIR:${TARGET}> VERBATIM)
  endforeach()
endfunction()
