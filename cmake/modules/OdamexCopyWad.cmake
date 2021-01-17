function(odamex_copy_wad _TARGET)
  if(TARGET odawad)
    add_dependencies(${_TARGET} odawad)
    add_custom_command(TARGET ${_TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_BINARY_DIR}/wad/odamex.wad" $<TARGET_FILE_DIR:${_TARGET}> VERBATIM)
  else()
    message(WARNING "Unable to copy ODAMEX.WAD to ${_TARGET} without anything to copy")
  endif()
endfunction()
