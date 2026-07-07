### wxWidgets ###

if(BUILD_LAUNCHER)
  if(USE_INTERNAL_WXWIDGETS)
    if(WIN32)
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
      set(wxWidgets_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets" CACHE PATH "")

      if(MSVC_VERSION GREATER_EQUAL 1900)
        # Visual Studio 2015/2017/2019
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.3.2/wxWidgets-3.3.2-headers.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.3.2-headers.7z"
            EXPECTED_HASH SHA256=6d0c866b4a4612f6a667194d1574049639181181f29dd6abceb50c5e5b2baa29)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.3.2-headers.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.3.2/wxMSW-3.3.2_vc14x_x64_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.3.2_vc14x_x64_Dev.7z"
            EXPECTED_HASH SHA256=6ef8686e8c0a9f466a6940dd0a52e09ead25bd3ee38733af6085762632d06c5d)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.3.2_vc14x_x64_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.3.2/wxMSW-3.3.2_vc14x_x64_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.3.2_vc14x_x64_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=7e7efa327e0f9dcb95b90069b7aa9fe7a6a764210bfb58f71b06e6c26255437b)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.3.2_vc14x_x64_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_x64_dll/wxrc.exe"
            CACHE FILEPATH "")
        else()
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.10/wxWidgets-3.2.10-headers.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.2.10-headers.7z"
            EXPECTED_HASH SHA256=f50af8b5415edb42ad223ee127fb48658a1c189e73b6bc678ab350e7396a18bf)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.2.10-headers.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.10/wxMSW-3.2.10_vc14x_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.2.10_vc14x_Dev.7z"
            EXPECTED_HASH SHA256=e89a1a1ce701a4b0194b3ba4635ef03e0106ec5b83e9cc9a957822337fba56a1)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.2.10_vc14x_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.10/wxMSW-3.2.10_vc14x_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.2.10_vc14x_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=79fc5c3edbbbb670d3d9213a4530c90a73eed0cfa8635522c76c954fca5b80c5)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.2.10_vc14x_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_dll/wxrc.exe"
            CACHE FILEPATH "")
        endif()
      elseif(MINGW)
        # MinGW
        file(DOWNLOAD
          "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxWidgets-3.1.5-headers.7z"
          "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.5-headers.7z"
          EXPECTED_HASH SHA256=5BEF630B59CBE515152EBAABC2B5BB83BBB908B798ACCBF28E4F3D79480EC0E2)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.5-headers.7z"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
        file(DOWNLOAD
          "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_gcc810_x64_Dev.7z"
          "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_gcc810_x64_Dev.7z"
          EXPECTED_HASH SHA256=65ED68EF72C5E9807B64FE664EBA561D4C33F494D71DCDF21D39110C601FD327)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
          "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_gcc810_x64_Dev.7z"
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

        # Move the lib directory to where FindwxWidgets.cmake can find it.
        if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc_dll")
          file(RENAME
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc810_x64_dll"
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc_dll")
        endif()

        set(wxWidgets_wxrc_EXECUTABLE
          "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/gcc_dll/wxrc.exe"
          CACHE FILEPATH "")
      endif()
    else()
      if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wxWidgets-3.0.5")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/wxWidgets-3.0.5"
          DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        # Compile wxWidgets and copy resources if exists in /wxWidgets/
        execute_process(COMMAND sh configure --enable-unicode --with-gtk=3
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.0.5")
        execute_process(COMMAND make -j3
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.0.5")
        file(COPY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.0.5/utils/wxrc/wxrc"
          DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.0.5/lib/")
        set(wxWidgets_wxrc_EXECUTABLE
          "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.0.5/lib/wxrc"
          CACHE FILEPATH "")
        set(wxWidgets_CONFIG_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.0.5/wx-config" CACHE PATH "")
        set(wxWidgets_USE_STATIC ON)
      endif()
    endif()
  endif()
endif()
