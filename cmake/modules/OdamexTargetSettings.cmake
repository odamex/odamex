function(odamex_target_settings _TARGET)
  set(ODAMEX_DLLS "")

  if(APPLE)
    target_compile_definitions("${_TARGET}" PRIVATE OSX UNIX)
  elseif(SOLARIS)
    target_compile_definitions("${_TARGET}" PRIVATE SOLARIS UNIX BSD_COMP)
    target_compile_options("${_TARGET}" PRIVATE -gstabs+)
  elseif(UNIX)
    target_compile_definitions("${_TARGET}" PRIVATE UNIX)
  endif()

  if(MSVC)
    # jsd: hide warnings about using insecure crt functions:
    target_compile_definitions("${_TARGET}" PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_definitions("${_TARGET}" PRIVATE $<$<CONFIG:Debug>:ODAMEX_DEBUG>)
    target_compile_options("${_TARGET}" PRIVATE -Wall -Wextra)

    if(USE_GPROF)
      target_compile_options("${_TARGET}" PRIVATE -p)
    endif()

    if(USE_COLOR_DIAGNOSTICS)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options("${_TARGET}" PRIVATE -fcolor-diagnostics)
      else()
        target_compile_options("${_TARGET}" PRIVATE -fdiagnostics-color=always)
      endif()
    endif()

    if(USE_STATIC_STDLIB)
      target_link_options("${_TARGET}" PRIVATE -static-libgcc -static-libstdc++)
    endif()

    if(USE_SANITIZE_ADDRESS)
      target_compile_options("${_TARGET}" PRIVATE
        -fsanitize=address -O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
    endif()
  endif()

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
