#==========================================
#
#	ODAMEX FOR THE PLAYSTATION VITA CMAKE FILE
#	
#	Do not forget to run this before even 
#	trying to do ANYTHING (CMake/Compile)!
#
#   export VITASDK=/usr/local/vitasdk
#   export PATH=$VITASDK/bin:$PATH
#
#	Then, when starting the CMAKE presentation...
#   cmake -DPSVITA=1 -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake ../odamex
#
#==========================================
message("Generating for Playstation Vita")
message("Please make sure you read psvita.cmake first for compilation instructions !")
message("")
message("")

#Default variables to do
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