### cpptrace ###

if(BUILD_CLIENT OR BUILD_SERVER)
  if(USE_INTERNAL_CPPTRACE)
    if(USE_INTERNAL_ZSTD)
      set(CPPTRACE_USE_EXTERNAL_ZSTD OFF)
    else()
      set(CPPTRACE_USE_EXTERNAL_ZSTD ON)
    endif()
    lib_buildgen(
      LIBRARY cpptrace
      PARAMS "-DCMAKE_DEBUG_POSTFIX=d"
             "-DCPPTRACE_USE_EXTERNAL_LIBDWARF=${USE_EXTERNAL_LIBDWARF}"
             "-DCPPTRACE_USE_EXTERNAL_ZSTD=${CPPTRACE_USE_EXTERNAL_ZSTD}")
    lib_build(LIBRARY cpptrace)
  endif()

  if(USE_INTERNAL_ZLIB)
    # Set vars so the finder can find them.
    set(ZLIB_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/local/include" CACHE PATH "" FORCE)
    if(WIN32)
      set(ZLIB_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}zlibstatic${libsuffix}"  CACHE PATH "" FORCE)
    else()
      set(ZLIB_LIBRARY
        "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}z${libsuffix}" CACHE PATH "" FORCE)
    endif()
  endif()

  find_package(cpptrace)
  if(TARGET cpptrace::cpptrace)
    set_target_properties(cpptrace::cpptrace PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
