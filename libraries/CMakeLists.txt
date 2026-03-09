# Build libraries.

# We use FetchContent instead of add_subdirectory specifically because it has
# the ability to override subsequent calls to find_package for the packages
# it obtains.  This is especially critical for making sure that sub-dependencies
# such as libpng -> zlib resolve correctly.

include(FetchContent)

### tiny libraries ###

add_library(tinylibs INTERFACE)
target_include_directories(tinylibs INTERFACE . "tinylibs/include")

### minilzo ###

if(BUILD_CLIENT OR BUILD_SERVER)
  message(STATUS "Compiling minilzo...")
  add_library(minilzo STATIC
      "minilzo/lzoconf.h" "minilzo/lzodefs.h" "minilzo/minilzo.c" "minilzo/minilzo.h")
  target_include_directories(minilzo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/minilzo")
endif()

set(FETCH_CONTENT_PACKAGES)

if (BUILD_TESTS)
    # ======================================
    # GTest
    # ======================================
    set (FETCHCONTENT_SOURCE_DIR_GTEST "${CMAKE_CURRENT_SOURCE_DIR}/googletest")
    FetchContent_Declare(
        GTest
        URL "${FETCHCONTENT_SOURCE_DIR_GTEST}"
        OVERRIDE_FIND_PACKAGE
        )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Just like ZLIB, this is used by other dependencies, we have to configure it early.
    FetchContent_MakeAvailable(GTest)

endif()

if (BUILD_CLIENT OR BUILD_SERVER)

    # ======================================
    # zlib
    # ======================================
    if (USE_INTERNAL_ZLIB)
        set (FETCHCONTENT_SOURCE_DIR_ZLIB "${CMAKE_CURRENT_SOURCE_DIR}/zlib")
        FetchContent_Declare(
            ZLIB
            URL "${FETCHCONTENT_SOURCE_DIR_ZLIB}"
            OVERRIDE_FIND_PACKAGE
            )

        # We go against CMake's normal advice of doing the MakeAvailable of everything in
        # one shot for ZLIB, because the other projects expect ZLIB::ZLIB to be an
        # *actual target* at the time that they're made available, and we have to declare
        # it as an alias.
        FetchContent_MakeAvailable(ZLIB)

        if (TARGET zlibstatic AND NOT TARGET ZLIB::ZLIB)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
            set (ZLIB_INCLUDE_DIRS "${FETCHCONTENT_SOURCE_DIR_ZLIB}")
        endif()

    else()
        find_package(ZLIB)
        if (TARGET ZLIB::ZLIB)
            set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_GLOBAL True)
        endif()
    endif()

    # ======================================
    # fmt
    # ======================================
    set (FETCHCONTENT_SOURCE_DIR_FMT "${CMAKE_CURRENT_SOURCE_DIR}/fmt")
    FetchContent_Declare(
        fmt
        URL "${FETCHCONTENT_SOURCE_DIR_FMT}"
        OVERRIDE_FIND_PACKAGE
        )
    list(APPEND FETCH_CONTENT_PACKAGES fmt)
    set(FMT_DOC     OFF)
    set(FMT_INSTALL OFF)
    set(FMT_TEST    OFF)

    # ======================================
    # jsoncpp
    # ======================================
    if (USE_INTERNAL_JSONCPP)
        set (FETCHCONTENT_SOURCE_DIR_JSONCPP "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp")
        FetchContent_Declare(
            jsoncpp
            URL "${FETCHCONTENT_SOURCE_DIR_JSONCPP}"
            OVERRIDE_FIND_PACKAGE
            )
        list(APPEND FETCH_CONTENT_PACKAGES jsoncpp)

        set(JSONCPP_WITH_TESTS                  OFF)
        set(JSONCPP_WITH_POST_BUILD_UNITTEST    OFF)
        set(JSONCPP_WITH_WARNING_AS_ERROR       OFF)
        set(JSONCPP_WITH_PKGCONFIG_SUPPORT      OFF)
        set(JSONCPP_WITH_CMAKE_PACKAGE           ON)
    endif()

    # ======================================
    # Protobuf
    # ======================================
    set (FETCHCONTENT_SOURCE_DIR_PROTOBUF "${CMAKE_CURRENT_SOURCE_DIR}/protobuf")
    FetchContent_Declare(
        Protobuf
        URL "${FETCHCONTENT_SOURCE_DIR_PROTOBUF}"
        OVERRIDE_FIND_PACKAGE
        )
    list(APPEND FETCH_CONTENT_PACKAGES Protobuf)

    set(protobuf_BUILD_SHARED_LIBS   OFF)
    set(protobuf_BUILD_TESTS         OFF)
    set(protobuf_MSVC_STATIC_RUNTIME OFF)
    set(protobuf_INSTALL             OFF)

    # Protobuf sets a policy older than 3.13, which would force us to set the above
    # values in the cache explicitly.  Let's just use a modern-day policy here...
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)   # option() honors normal variables

    # ======================================
    # cpptrace
    # ======================================
    if (USE_INTERNAL_CPPTRACE)
        set (FETCHCONTENT_SOURCE_DIR_CPPTRACE "${CMAKE_CURRENT_SOURCE_DIR}/cpptrace")
        FetchContent_Declare(
            cpptrace
            URL "${FETCHCONTENT_SOURCE_DIR_CPPTRACE}"
            OVERRIDE_FIND_PACKAGE
            )
        list(APPEND FETCH_CONTENT_PACKAGES cpptrace)

        set (CPPTRACE_USE_EXTERNAL_LIBDWARF ${USE_EXTERNAL_LIBDWARF})
        if (USE_INTERNAL_ZSTD)
            set(CPPTRACE_USE_EXTERNAL_ZSTD OFF)
        else()
            set(CPPTRACE_USE_EXTERNAL_ZSTD ON)
        endif()
    else()
        find_package(cpptrace)
        if(TARGET cpptrace::cpptrace)
            set_target_properties(cpptrace::cpptrace PROPERTIES IMPORTED_GLOBAL True)
        endif()
    endif()

    if (BUILD_SERVER)

        # ======================================
        # MiniUPnP
        # ======================================
        if (NOT USE_MINIUPNP)
            message(STATUS "Skipping MiniUPnPc...")
        elseif(USE_INTERNAL_MINIUPNP)
            set(FETCHCONTENT_SOURCE_DIR_MINIUPNP "${CMAKE_CURRENT_SOURCE_DIR}/miniupnp")
            FetchContent_Declare(
                miniupnp
                URL "${FETCHCONTENT_SOURCE_DIR_MINIUPNP}"
                SOURCE_SUBDIR miniupnpc
                OVERRIDE_FIND_PACKAGE
                )
            list(APPEND FETCH_CONTENT_PACKAGES miniupnp)

            set(UPNPC_BUILD_SHARED  OFF)
            set(UPNPC_BUILD_TESTS   OFF)
            set(UPNPC_BUILD_SAMPLE  OFF)
        endif()
    endif()

    if (BUILD_CLIENT)

        # ======================================
        # PNG
        # ======================================
        if (USE_INTERNAL_PNG)
            set(FETCHCONTENT_SOURCE_DIR_PNG "${CMAKE_CURRENT_SOURCE_DIR}/libpng")
            FetchContent_Declare(
                PNG
                URL "${FETCHCONTENT_SOURCE_DIR_PNG}"
                OVERRIDE_FIND_PACKAGE
                )
            list(APPEND FETCH_CONTENT_PACKAGES PNG)

            set (PNG_SHARED OFF)
            set (PNG_STATIC  ON)
            set (PNG_TESTS  OFF)
            set (SKIP_INSTALL_ALL ON)

            if (APPLE)
                set (PNG_ARM_NEON                 OFF)
                set (PNG_ARM_NEON_CHECK_SUPPORTED OFF)
            endif()
        else()
            find_package(PNG)
            if(TARGET PNG::PNG)
                set_target_properties(PNG::PNG PROPERTIES IMPORTED_GLOBAL True)
            endif()
        endif()

        # ======================================
        # CURL
        # ======================================
        if (USE_INTERNAL_CURL)
            set(FETCHCONTENT_SOURCE_DIR_CURL "${CMAKE_CURRENT_SOURCE_DIR}/curl")
            FetchContent_Declare(
                CURL
                URL "${FETCHCONTENT_SOURCE_DIR_CURL}"
                OVERRIDE_FIND_PACKAGE
                )
            list(APPEND FETCH_CONTENT_PACKAGES CURL)

            set(BUILD_SHARED_LIBS       OFF)
            set(BUILD_STATIC_LIBS       ON)
            set(CURL_DISABLE_INSTALL    ON)
            set(BUILD_CURL_EXE          OFF)
            set(BUILD_EXAMPLES          OFF)
            set(BUILD_LIBCURL_DOCS      OFF)
            set(BUILD_MISC_DOCS         OFF)
            set(ENABLE_CURL_MANUAL      OFF)
            set(BUILD_TESTING           OFF)
            set(CURL_USE_LIBPSL         OFF)
            set(CURL_USE_LIBSSH2        OFF)
            set(CURL_ZLIB               OFF)
            set(HTTP_ONLY               ON)

            if (WIN32)
                set(CURL_USE_SCHANNEL   ON)
            endif()

            set(CURL_ENABLE_EXPORT_TARGET OFF)
        else()
            find_package(CURL)
            if (TARGET CURL::libcurl)
                set_target_properties(CURL::libcurl PROPERTIES IMPORTED_GLOBAL True)
            endif()
        endif()

        # ======================================
        # FLTK
        # ======================================
        if (USE_INTERNAL_FLTK)
            set(FETCHCONTENT_SOURCE_DIR_FLTK "${CMAKE_CURRENT_SOURCE_DIR}/fltk")
            FetchContent_Declare(
                FLTK
                URL "${FETCHCONTENT_SOURCE_DIR_FLTK}"
                OVERRIDE_FIND_PACKAGE
                )
            list(APPEND FETCH_CONTENT_PACKAGES FLTK)

            set(FLTK_USE_SYSTEM_LIBJPEG OFF)
            set(FLTK_USE_SYSTEM_LIBPNG  OFF)
            set(FLTK_USE_SYSTEM_ZLIB    OFF)
            set(FLTK_OPTION_PRINT_SUPPORT   OFF)
            set(FLTK_BUILD_GL               OFF)
            set(FLTK_USE_PANGO              OFF)
            set(FLTK_GRAPHICS_CAIRO         OFF)
            set(FLTK_OPTION_CAIRO_WINDOW    OFF)
            set(FLTK_OPTION_CAIRO_EXT       OFF)
            set(FLTK_BUILD_TEST             OFF)

            if(APPLE)
                if(NOT USE_INTERNAL_PNG)
                    set(FLTK_USE_SYSTEM_LIBPNG  ON)
                endif()
                if(NOT USE_INTERNAL_ZLIB)
                    set(FLTK_USE_SYSTEM_ZLIB    ON)
                endif()
            elseif(UNIX AND NOT APPLE)
                set(FLTK_USE_SYSTEM_LIBDECOR    OFF)
                set(FLTK_USE_LIBDECOR_GTK       OFF)
            endif()
        else()
            find_package(FLTK CONFIG)
                if (NOT TARGET fltk::fltk)
                    find_package(FLTK REQUIRED)
                endif()
        endif()

        # ======================================
        # libADLMidi
        # ======================================
        if (USE_INTERNAL_LIBADLMIDI)
            set(FETCHCONTENT_SOURCE_DIR_LIBADLMIDI "${CMAKE_CURRENT_SOURCE_DIR}/libadlmidi")
            FetchContent_Declare(
                libadlmidi
                URL "${FETCHCONTENT_SOURCE_DIR_LIBADLMIDI}"
                OVERRIDE_FIND_PACKAGE
                )
            list(APPEND FETCH_CONTENT_PACKAGES libadlmidi)
        else()
            find_package(libADLMIDI)
            if (TARGET libADLMIDI::ADLMIDI_static)
                set_target_properties(libADLMIDI::ADLMIDI_static PROPERTIES IMPORTED_GLOBAL True)
            endif()
        endif()

        # ======================================
        # SDL
        # ======================================
        FetchContent_Declare(
            SDL
            GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
            GIT_TAG         release-2.32.10
            OVERRIDE_FIND_PACKAGE
            )
        list(APPEND FETCH_CONTENT_PACKAGES SDL)

        set (SDL_SHARED OFF)
        set (SDL_STATIC ON)

        # ======================================
        # SDL_mixer
        # ======================================
        FetchContent_Declare(
            SDL_mixer
            GIT_REPOSITORY  https://github.com/libsdl-org/SDL_mixer.git
            GIT_TAG         release-2.8.1
            OVERRIDE_FIND_PACKAGE
            )
        list(APPEND FETCH_CONTENT_PACKAGES SDL_mixer)

        set (SDL2MIXER_VENDORED ON)
        set (BUILD_SHARED_LIBS OFF)
    endif()
endif()

if (BUILD_LAUNCHER)

    # ======================================
    # wxWidgets
    # ======================================
    FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY  https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG         v3.2.10
        OVERRIDE_FIND_PACKAGE
        )
    list(APPEND FETCH_CONTENT_PACKAGES wxWidgets)

endif()

# ----------==================----------
# Now bring it all in!
# ----------==================----------

if (FETCH_CONTENT_PACKAGES)
    FetchContent_MakeAvailable(${FETCH_CONTENT_PACKAGES})
endif()

# Now do some patching to smooth-over any disagreements between the
# target names as established by building the libs vs. finding them.

if (TARGET libminiupnpc-static AND NOT miniupnpc::miniupnpc)
    add_library(miniupnpc::miniupnpc ALIAS libminiupnpc-static)
    target_compile_definitions(libminiupnpc-static INTERFACE INTERNAL_MINIUPNP_STATICLIB)
endif()

if (TARGET libcurl_static AND NOT TARGET CURL::libcurl)
    add_library(CURL::libcurl ALIAS libcurl_static)
endif()

if (TARGET wxrc AND NOT wxWidgets_wxrc_EXECUTABLE)
    set(wxWidgets_wxrc_EXECUTABLE wxrc PARENT_SCOPE)
endif()

if (TARGET png_static AND NOT TARGET PNG::PNG)
    add_library(PNG::PNG ALIAS png_static)
endif()

if (TARGET ADLMIDI_static AND NOT TARGET libADLMIDI::ADLMIDI_static)
    add_library(libADLMIDI::ADLMIDI_static ALIAS ADLMIDI_static)
endif()

#find_package(Protobuf)

#include(zlib-lib.cmake)

#include(libpng-lib.cmake)

#include(curl-lib.cmake)

#include(fltk-lib.cmake)

#include(fmt-lib.cmake)

#include(jsoncpp-lib.cmake)

#include(miniupnp-lib.cmake)

#include(protobuf-lib.cmake)

#include(SDL-lib.cmake)

#include(wxWidgets-lib.cmake)

#include(libadlmidi-lib.cmake)

#include(cpptrace-lib.cmake)
