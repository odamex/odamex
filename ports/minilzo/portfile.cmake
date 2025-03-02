set(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/minilzo-2.10")

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/minilzo")
vcpkg_copy_pdbs()
