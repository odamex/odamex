### libwebsockets ###

if(BUILD_SERVER)
  # Figure out the correct library path to attach to our imported target
  set(MINIUPNPC_LIBRARY
    "${CMAKE_CURRENT_BINARY_DIR}/local/lib/${libprefix}miniupnpc${libsuffix}")

  lib_buildgen(
    LIBRARY libwebsockets
    PARAMS "-DLWS_ROLE_RAW_FILE=OFF"
           "-DLWS_WITH_HTTP2=OFF"
           "-DLWS_IPV6=OFF"
           "-DLWS_WITH_HTTP_BASIC_AUTH=OFF"
           "-DLWS_WITH_HTTP_DIGEST_AUTH=OFF"
           "-DLWS_WITH_HTTP_UNCOMMON_HEADERS=OFF"
           "-DLWS_WITH_SYS_STATE=OFF"
           "-DLWS_WITH_SYS_SMD=OFF"
           "-DLWS_WITH_UPNG=OFF"
           "-DLWS_WITH_GZINFLATE=OFF"
           "-DLWS_WITH_JPEG=OFF"
           "-DLWS_WITH_DLO=OFF"
           "-DLWS_WITH_SECURE_STREAMS=OFF"
           "-DLWS_WITH_SHARED=OFF"
           "-DLWS_WITH_JSONRPC=OFF"
           "-DLWS_WITH_DIR=OFF"
           "-DLWS_WITH_LEJP_CONF=OFF"
           "-DLWS_WITH_MINIMAL_EXAMPLES=OFF"
           "-DLWS_WITH_WOL=OFF"
           "-DLWS_WITHOUT_CLIENT=ON"
           "-DLWS_WITHOUT_TESTAPPS=ON"
           "-DLWS_WITHOUT_TEST_SERVER=ON"
           "-DLWS_WITHOUT_TEST_SERVER_EXTPOLL=ON"
           "-DLWS_WITHOUT_TEST_PING=ON"
           "-DLWS_WITHOUT_TEST_CLIENT=ON"
           )
  lib_build(LIBRARY libwebsockets)

  find_package(libwebsockets REQUIRED)
  set_target_properties(websockets PROPERTIES IMPORTED_GLOBAL TRUE)
endif()
