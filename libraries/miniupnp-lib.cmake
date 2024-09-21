### MiniUPnPc ###

if(BUILD_SERVER AND NOT USE_MINIUPNP)
  message(STATUS "Skipping MiniUPnPc...")
elseif(BUILD_SERVER AND USE_MINIUPNP AND USE_INTERNAL_MINIUPNP)
  message(STATUS "Compiling MiniUPnPc...")

  # Figure out the correct library path to attach to our imported target
  set(MINIUPNPC_LIBRARY
    "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}miniupnpc${libsuffix}")

  lib_buildgen(
    LIBRARY miniupnpc
    SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/miniupnp/miniupnpc"
    PARAMS "-DUPNPC_BUILD_SHARED=OFF"
           "-DUPNPC_BUILD_TESTS=OFF"
           "-DUPNPC_BUILD_SAMPLE=OFF")
  lib_build(LIBRARY miniupnpc)

  find_package(miniupnpc REQUIRED)
  set_target_properties(miniupnpc::miniupnpc PROPERTIES IMPORTED_GLOBAL TRUE)
endif()
