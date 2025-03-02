vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO fltk/fltk
    REF "release-${VERSION}"
    SHA512 7630d0b02ff09c277d5641e10631514e5e3d8087e81f5254f38a8adb05c39ed0092ae81697085ed0dd859f0b826f94626d698090153c5e9a655f5e36263b2915)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DFLTK_BUILD_EXAMPLES=OFF
        -DFLTK_BUILD_FLUID=OFF
        -DFLTK_BUILD_TEST=OFF
        -DFLTK_USE_SYSTEM_LIBJPEG=ON
        -DFLTK_USE_SYSTEM_LIBPNG=ON
        -DFLTK_USE_SYSTEM_ZLIB=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_Doxygen=1)

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_copy_tools(TOOL_NAMES fltk-options fltk-options-cmd AUTO_CLEAN)
vcpkg_cmake_config_fixup(CONFIG_PATH "CMake")
