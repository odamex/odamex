function(odalaunch_copy_libs TARGET)
  set(ODAMEX_DLLS "")

  if(WIN32)
    if(MSVC_VERSION GREATER_EQUAL 1900)
      set(WX_DLL_DIR "${CMAKE_BINARY_DIR}/libraries/wxWidgets/lib/vc14x_x64_dll")
      list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_net_vc14x_x64.dll")
      list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_vc14x_x64.dll")
      list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_xml_vc14x_x64.dll")
      list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_core_vc14x_x64.dll")
      list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_html_vc14x_x64.dll")
      list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_xrc_vc14x_x64.dll")
    endif()
  endif()

  # Copy library files to target directory.
  foreach(ODAMEX_DLL ${ODAMEX_DLLS})
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${ODAMEX_DLL}" $<TARGET_FILE_DIR:${TARGET}> VERBATIM)
  endforeach()
endfunction()
