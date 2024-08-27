### wxWidgets ###

if(BUILD_LAUNCHER)
  if(USE_INTERNAL_WXWIDGETS)
    if(WIN32)
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
      set(wxWidgets_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets" CACHE PATH "")

      # Cross-compiler headers
      file(DOWNLOAD
        "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxWidgets-3.1.5-headers.7z"
        "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.5-headers.7z"
        EXPECTED_HASH SHA256=5BEF630B59CBE515152EBAABC2B5BB83BBB908B798ACCBF28E4F3D79480EC0E2)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets-3.1.5-headers.7z"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

      if(MSVC_VERSION GREATER_EQUAL 1900)
        # Visual Studio 2015/2017/2019
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_x64_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_Dev.7z"
            EXPECTED_HASH SHA256=7FC34D32030A6BF84B5C3B00D3CECA12683ECAB514BDBED91723AB67B22E8FBA)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_x64_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=29CA27A2CEE48EB8B07D9F9B5318AF4068AD5D26344F021A311F8557060B1433)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_x64_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_x64_dll/wxrc.exe"
            CACHE FILEPATH "")
        else()
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_Dev.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_Dev.7z"
            EXPECTED_HASH SHA256=751C00CCEB1FC5243C8FDA45F678732AEEFB10E0A2E348DBD3A7131C8A475898)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_Dev.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")
          file(DOWNLOAD
            "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.5/wxMSW-3.1.5_vc14x_ReleaseDLL.7z"
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_ReleaseDLL.7z"
            EXPECTED_HASH SHA256=B9EC5AF60CE0E489AB6D23CB75004CBD10281932EF353FD44FF51A51143D776D)
          execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
            "${CMAKE_CURRENT_BINARY_DIR}/wxMSW-3.1.5_vc14x_ReleaseDLL.7z"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets")

          set(wxWidgets_wxrc_EXECUTABLE
            "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets/lib/vc14x_dll/wxrc.exe"
            CACHE FILEPATH "")
        endif()
      elseif(MINGW)
        # MinGW
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
    endif()
  endif()
endif()
