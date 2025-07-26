### cpptrace ###

if(BUILD_CLIENT OR BUILD_SERVER)
  lib_buildgen(
    LIBRARY cpptrace
    PARAMS "-DCMAKE_DEBUG_POSTFIX=d"
           "-DCPPTRACE_USE_EXTERNAL_LIBDWARF=${USE_EXTERNAL_LIBDWARF}")
  lib_build(LIBRARY cpptrace)

  find_package(cpptrace)
  if(TARGET cpptrace::cpptrace)
    set_target_properties(cpptrace::cpptrace PROPERTIES IMPORTED_GLOBAL True)
  endif()
endif()
