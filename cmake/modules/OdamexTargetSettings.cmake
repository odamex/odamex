include(CheckCXXCompilerFlag)

function(checked_add_compile_flag _LIST _FLAG _VAR)
  check_cxx_compiler_flag(${_FLAG} ${_VAR})
  if(${_VAR})
    set(NEW_LIST ${${_LIST}})
    list(APPEND NEW_LIST ${_FLAG})
    set(${_LIST} ${NEW_LIST} PARENT_SCOPE)
  endif()
endfunction()

function(odamex_target_settings _TARGET)
  set(ODAMEX_DLLS "")

  if(APPLE)
    target_compile_definitions("${_TARGET}" PRIVATE OSX UNIX)
    set_target_properties("${_TARGET}" PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
  elseif(SOLARIS)
    target_compile_definitions("${_TARGET}" PRIVATE SOLARIS UNIX BSD_COMP)
    target_compile_options("${_TARGET}" PRIVATE -gstabs+)
  elseif(UNIX)
    target_compile_definitions("${_TARGET}" PRIVATE UNIX)
  endif()

  if(MSVC)
    # jsd: hide warnings about using insecure crt functions:
    target_compile_definitions("${_TARGET}" PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options("${_TARGET}" PRIVATE /MP)
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
      target_link_options("${_TARGET}" PRIVATE -fsanitize=address)
    endif()
  endif()

  # Add checked compile options - mostly taken from:
  # https://kristerw.blogspot.com/2017/09/useful-gcc-warning-options-not-enabled.html
  if(MSVC)
    checked_add_compile_flag(CHECKED_OPTIONS /wd26812 WD_26812)
  else()
    checked_add_compile_flag(CHECKED_OPTIONS -Wduplicated-cond W_DUPLICATED_COND)
    checked_add_compile_flag(CHECKED_OPTIONS -Wduplicated-branches W_DUPLICATED_BRANCHES)
    checked_add_compile_flag(CHECKED_OPTIONS -Wrestrict W_RESTRICT)
    checked_add_compile_flag(CHECKED_OPTIONS -Wnull-dereference W_NULL_DEREFERENCE)
    checked_add_compile_flag(CHECKED_OPTIONS -Wuseless-cast W_USELESS_CAST)
    checked_add_compile_flag(CHECKED_OPTIONS -Wformat=2 W_FORMAT_2)
    checked_add_compile_flag(CHECKED_OPTIONS -Wno-unused-parameter W_NO_UNUSED_PARAMETER)
  endif()
  target_compile_options("${_TARGET}" PRIVATE ${CHECKED_OPTIONS})
  
  # Ensure we get a useful stack trace on Linux.
  if(UNIX AND NOT APPLE)
    target_link_options("${_TARGET}" PRIVATE -rdynamic)
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
    add_custom_command(TARGET "${_TARGET}" POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${ODAMEX_DLL}" $<TARGET_FILE_DIR:${_TARGET}> VERBATIM)
  endforeach()
endfunction()
