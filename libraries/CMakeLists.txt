# Build libraries.
#
# Here we use execute_process and file(DOWNLOAD) to download and compile
# libraries as we need them.  Some stuff to remember:
#
# - C libraries can cross Debug/Release boundaries, but C++ libraries can't.
#   You'll need to be able to select the proper debug vs release library in
#   multi-config builds - see JsonCpp for an example.
# - Xcode does not like it when CMake passes --parallel with no parameter.
#

# Parallel processor count - needed by Xcode.
include(ProcessorCount)
ProcessorCount(PARALLEL_PROC_COUNT)

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

list(APPEND FETCH_CONTENT_PACKAGES)

if (BUILD_CLIENT OR BUILD_SERVER)
    if (USE_INTERNAL_ZLIB)
        FetchContent_Declare(
            ZLIB
            URL ${CMAKE_CURRENT_SOURCE_DIR}/zlib
            OVERRIDE_FIND_PACKAGE
            )

        # We go against CMake's normal advice of doing the MakeAvailable of everything in
        # one shot for ZLIB, because the other projects expect ZLIB::ZLIB to be an
        # *actual target* at the time that they're made available, and we have to declare
        # it as an alias.
        FetchContent_MakeAvailable(ZLIB)

        if (TARGET zlibstatic AND NOT TARGET ZLIB::ZLIB)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        endif()

    else()
        find_package(ZLIB)
        if (TARGET ZLIB::ZLIB)
            set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_GLOBAL True)
        endif()
    endif()

    FetchContent_Declare(
        fmt
        URL ${CMAKE_CURRENT_SOURCE_DIR}/fmt
        OVERRIDE_FIND_PACKAGE
        )
    list(APPEND FETCH_CONTENT_PACKAGES fmt)
    set(FMT_DOC     OFF)
    set(FMT_INSTALL OFF)
    set(FMT_TEST    OFF)

    if (USE_INTERNAL_JSONCPP)
        FetchContent_Declare(
            jsoncpp
            URL ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp
            OVERRIDE_FIND_PACKAGE
            )
        list(APPEND FETCH_CONTENT_PACKAGES jsoncpp)

        set(JSONCPP_WITH_TESTS                  OFF)
        set(JSONCPP_WITH_POST_BUILD_UNITTEST    OFF)
        set(JSONCPP_WITH_WARNING_AS_ERROR       OFF)
        set(JSONCPP_WITH_PKGCONFIG_SUPPORT      OFF)
        set(JSONCPP_WITH_CMAKE_PACKAGE           ON)
    endif()

    FetchContent_Declare(
        Protobuf
        URL ${CMAKE_CURRENT_SOURCE_DIR}/protobuf
        OVERRIDE_FIND_PACKAGE
        )
    list(APPEND FETCH_CONTENT_PACKAGES Protobuf)

    set(protobuf_BUILD_SHARED_LIBS   OFF)
    set(protobuf_BUILD_TESTS         OFF)
    set(protobuf_MSVC_STATIC_RUNTIME OFF)
    set(protobuf_INSTALL             OFF)

    if (USE_INTERNAL_CPPTRACE)
        FetchContent_Declare(
            cpptrace
            URL ${CMAKE_CURRENT_SOURCE_DIR}/cpptrace
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
        if (NOT USE_MINIUPNP)
            message(STATUS "Skipping MiniUPnPc...")
        elseif(USE_INTERNAL_MINIUPNP)
            FetchContent_Declare(
                miniupnp
                URL ${CMAKE_CURRENT_SOURCE_DIR}/miniupnp
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
        if (USE_INTERNAL_PNG)
            FetchContent_Declare(
                PNG
                URL ${CMAKE_CURRENT_SOURCE_DIR}/libpng
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

        if (USE_INTERNAL_CURL)
            FetchContent_Declare(
                CURL
                URL ${CMAKE_CURRENT_SOURCE_DIR}/curl
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
        endif()

        if (USE_INTERNAL_FLTK)
            FetchContent_Declare(
                FLTK
                URL ${CMAKE_CURRENT_SOURCE_DIR}/fltk
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

        if (USE_INTERNAL_LIBADLMIDI)
            FetchContent_Declare(
                libadlmidi
                URL ${CMAKE_CURRENT_SOURCE_DIR}/libadlmidi
                OVERRIDE_FIND_PACKAGE
                )
            list(APPEND FETCH_CONTENT_PACKAGES libadlmidi)
        else()
            find_package(libADLMIDI)
            if (TARGET libADLMIDI::ADLMIDI_static)
                set_target_properties(libADLMIDI::ADLMIDI_static PROPERTIES IMPORTED_GLOBAL True)
            endif()
        endif()

        FetchContent_Declare(
            SDL
            GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
            GIT_TAG         release-2.32.10
            OVERRIDE_FIND_PACKAGE
            )
        list(APPEND FETCH_CONTENT_PACKAGES SDL)

        set (SDL_SHARED OFF)
        set (SDL_STATIC ON)

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

    FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY  https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG         v3.2.10
        OVERRIDE_FIND_PACKAGE
        )
    list(APPEND FETCH_CONTENT_PACKAGES wxWidgets)

endif()

if (FETCH_CONTENT_PACKAGES)
    FetchContent_MakeAvailable(${FETCH_CONTENT_PACKAGES})
endif()

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
