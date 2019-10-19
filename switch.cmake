#==========================================
#
#	ODAMEX FOR NINTENDO SWITCH CMAKE FILE
#	
#	Do not forget to run this before even 
#	trying to do ANYTHING (CMake/Compile)!
#
#	source $DEVKITPRO/switchvars.sh
#
#==========================================
message("Generating for Nintendo Switch")
message("MAKE SURE you entered this command BEFORE either compiling OR CMAKEing!")
message("source $DEVKITPRO/switchvars.sh")
message("")
message("")

#Default variables to do
include("$ENV{DEVKITPRO}/switch.cmake" REQUIRED)

set (PKG_CONFIG_EXECUTABLE, "$ENV{DEVKITPRO}/portlibs/switch/bin/aarch64-none-elf-pkg-config")
set (CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)

# Build type (Release/Debug)
set (CMAKE_BUILD_TYPE, "Release")

# Odamex specific settings
set (BUILD_CLIENT 1)
set (BUILD_SERVER 0)
set (BUILD_MASTER 0)
set (BUILD_ODALAUNCH 0)
set (USE_MINIUPNP 0)
set (ENABLE_PORTMIDI 0)


# This is a flag meaning we're compiling for a console
set (GCONSOLE 1)