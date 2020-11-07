function(odamex_copy_wad TARGET)
  if(TARGET odawad)
    add_dependencies(${TARGET} odawad)
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_BINARY_DIR}/wad/odamex.wad" $<TARGET_FILE_DIR:${TARGET}> VERBATIM)
  else()
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_SOURCE_DIR}/wad/odamex.wad" $<TARGET_FILE_DIR:${TARGET}> VERBATIM)
  endif()
endfunction()
