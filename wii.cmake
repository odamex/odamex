#==========================================
#
#	ODAMEX FOR NINTENDO WII/vWII CMAKE FILE
#	
#	Do not forget to run this before even 
#	trying to do ANYTHING (CMake/Compile)!
#
#   Please read "client/vwii/readme.md" for 
#   more informations on how to compile for 
#   the Nintendo Wii/vWii.
#
#==========================================
message("Generating Makefile for Nintendo Wii/vWii")
message("Please make sure you read client/vwii/readme.md first for compilation instructions !")
message("")
message("")

#Default variables to do
set (PKG_CONFIG_EXECUTABLE, "$ENV{DEVKITPRO}/portlibs/ppc/bin/powerpc-eabi-pkg-config")
set (CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)

# Build type (Release/Debug)
set (CMAKE_BUILD_TYPE, "Release")

# Odamex specific settings
set (BUILD_CLIENT 1)
set (BUILD_SERVER 0)
set (BUILD_MASTER 0)
set (BUILD_LAUNCHER 0)
set (USE_MINIUPNP 0)
set (ENABLE_PORTMIDI 0)

# Since we use SDL1.2 only, force this option
set (USE_SDL12 1)

# This is a flag meaning we're compiling for a console
set (GCONSOLE 1)