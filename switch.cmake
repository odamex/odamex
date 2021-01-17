#==========================================
#
#	ODAMEX FOR NINTENDO SWITCH CMAKE FILE
#	
#	Do not forget to run this before even 
#	trying to do ANYTHING (CMake/Compile)!
#
#   Please read "client/switch/readme.md" for 
#   more informations on how to compile for 
#   the Nintendo Switch.
#
#==========================================
message("Generating for Nintendo Switch")
message("MAKE SURE you entered this command BEFORE either compiling OR CMAKEing!")
message("source $DEVKITPRO/switchvars.sh")
message("")
message("")

set (PKG_CONFIG_EXECUTABLE, "$ENV{DEVKITPRO}/portlibs/switch/bin/aarch64-none-elf-pkg-config")
set (CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)

# Build type (Release/Debug)
set (CMAKE_BUILD_TYPE, "Release")

# Since 0.8.3, Odamex is forced to used C++98 standards.
# Force it back to C++11 to allow the Switch to properly compile.
set(CMAKE_CXX_STANDARD 11)

# Odamex specific settings
set (BUILD_CLIENT 1)
set (BUILD_SERVER 0)
set (BUILD_MASTER 0)
set (BUILD_LAUNCHER 0)
set (USE_MINIUPNP 0)
set (USE_DISCORDRPC 0)
set (ENABLE_PORTMIDI 0)

# This is a flag meaning we're compiling for a console
set (GCONSOLE 1)

# Functions & Stuff

function (odamex_target_settings_nx _DIRECTORY _FILES)
#    set(${_DIRECTORY} ${CLIENT_DIR} switch)  # Nintendo Switch
#    file(${_FILES} CLIENT_HEADERS ${CLIENT_HEADERS} switch/*.h)
#    file(${_FILES} CLIENT_SOURCES ${CLIENT_SOURCES} switch/*.cpp)
    add_definitions("-DUNIX -DGCONSOLE")
endfunction()

function(odamex_target_postcompile_nx _TARGET)
    add_custom_command( TARGET ${_TARGET} POST_BUILD
        COMMAND aarch64-none-elf-strip -o ${CMAKE_BINARY_DIR}/odamex_stripped.elf ${CMAKE_BINARY_DIR}/client/odamex
        COMMAND elf2nro ${CMAKE_BINARY_DIR}/odamex_stripped.elf ${CMAKE_BINARY_DIR}/odamex.nro --icon=${CMAKE_SOURCE_DIR}/client/switch/assets/odamex.jpg --nacp=${CMAKE_SOURCE_DIR}/client/switch/assets/odamex.nacp )
endfunction()