function(odalaunch_copy_libs TARGET)
  set(ODAMEX_DLLS "")

  if(WIN32)
    if(MSVC_VERSION GREATER_EQUAL 1900)
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(WX_DLL_DIR "${CMAKE_BINARY_DIR}/libraries/wxWidgets/lib/vc14x_x64_dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_net_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_xml_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_core_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_html_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_xrc_vc14x_x64.dll")
      else()
        set(WX_DLL_DIR "${CMAKE_BINARY_DIR}/libraries/wxWidgets/lib/vc14x_dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_net_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase314ud_xml_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_core_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_html_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw314ud_xrc_vc14x.dll")
      endif()
    endif()
  endif()

  # Copy library files to target directory.
  foreach(ODAMEX_DLL ${ODAMEX_DLLS})
    string(REPLACE "314ud_" "314u_" ODAMEX_RELEASE_DLL "${ODAMEX_DLL}")
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      $<$<CONFIG:Debug>:${ODAMEX_DLL}>
      $<$<CONFIG:Release>:${ODAMEX_RELEASE_DLL}>
      $<$<CONFIG:RelWithDebInfo>:${ODAMEX_RELEASE_DLL}>
      $<$<CONFIG:MinSizeRel>:${ODAMEX_RELEASE_DLL}>
      $<TARGET_FILE_DIR:${TARGET}> VERBATIM)
  endforeach()
endfunction()
