function(odalaunch_copy_libs TARGET)
  set(ODAMEX_DLLS "")

  if(WIN32)
    if(MSVC_VERSION GREATER_EQUAL 1900)
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(WX_DLL_DIR "${CMAKE_BINARY_DIR}/libraries/wxWidgets/lib/vc14x_x64_dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase332ud_net_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase332ud_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase332ud_xml_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw332ud_core_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw332ud_html_vc14x_x64.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw332ud_xrc_vc14x_x64.dll")
      else()
        set(WX_DLL_DIR "${CMAKE_BINARY_DIR}/libraries/wxWidgets/lib/vc14x_dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase32ud_net_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase32ud_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxbase32ud_xml_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw32ud_core_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw32ud_html_vc14x.dll")
        list(APPEND ODAMEX_DLLS "${WX_DLL_DIR}/wxmsw32ud_xrc_vc14x.dll")
      endif()
    endif()
  endif()

  # Copy library files to target directory.
  foreach(ODAMEX_DLL ${ODAMEX_DLLS})
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      string(REPLACE "332ud_" "332u_" ODAMEX_RELEASE_DLL "${ODAMEX_DLL}")
    else()
      string(REPLACE "32ud_" "32u_" ODAMEX_RELEASE_DLL "${ODAMEX_DLL}")
    endif()
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      $<$<CONFIG:Debug>:${ODAMEX_DLL}>
      $<$<CONFIG:Release>:${ODAMEX_RELEASE_DLL}>
      $<$<CONFIG:RelWithDebInfo>:${ODAMEX_RELEASE_DLL}>
      $<$<CONFIG:MinSizeRel>:${ODAMEX_RELEASE_DLL}>
      $<TARGET_FILE_DIR:${TARGET}> VERBATIM)
  endforeach()
endfunction()
