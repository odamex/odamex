function(odamex_copy_libs TARGET)
  set(ODAMEX_DLLS "")

  if(WIN32)
    if(MSVC)
      set(SDL2_DLL_DIR "$<TARGET_FILE_DIR:SDL2::SDL2>")
      set(SDL2_MIXER_DLL_DIR "$<TARGET_FILE_DIR:SDL2::mixer>")
    else()
      set(SDL2_DLL_DIR "$<TARGET_FILE_DIR:SDL2::SDL2>/../bin")
      set(SDL2_MIXER_DLL_DIR "$<TARGET_FILE_DIR:SDL2::mixer>/../bin")
    endif()

    # SDL2
    list(APPEND ODAMEX_DLLS "${SDL2_DLL_DIR}/SDL2.dll")

    # SDL2_mixer
    list(APPEND ODAMEX_DLLS "${SDL2_MIXER_DLL_DIR}/optional/libwavpack-1.dll")
    list(APPEND ODAMEX_DLLS "${SDL2_MIXER_DLL_DIR}/optional/libgme.dll")
    list(APPEND ODAMEX_DLLS "${SDL2_MIXER_DLL_DIR}/optional/libxmp.dll")
    list(APPEND ODAMEX_DLLS "${SDL2_MIXER_DLL_DIR}/optional/libogg-0.dll")
    list(APPEND ODAMEX_DLLS "${SDL2_MIXER_DLL_DIR}/optional/libopus-0.dll")
    list(APPEND ODAMEX_DLLS "${SDL2_MIXER_DLL_DIR}/optional/libopusfile-0.dll")
    list(APPEND ODAMEX_DLLS "${SDL2_MIXER_DLL_DIR}/SDL2_mixer.dll")
  endif()

  # Copy library files to target directory.
  foreach(ODAMEX_DLL ${ODAMEX_DLLS})
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${ODAMEX_DLL}" $<TARGET_FILE_DIR:${TARGET}> VERBATIM)
  endforeach()
endfunction()
