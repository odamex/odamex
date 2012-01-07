# Setup tools
SHELL = /bin/sh
OBJDIR = obj
# denis - fixme - remove mkdir -p
MKDIR = mkdir -p
CC = g++
LD = g++
INSTALL = cp
DEUTEX = deutex

# Detect OS
win32     := false
freebsd   := false
linux     := false
mingw	  := false
bigendian := false
cygwin	  := false
osx     := false
solaris	:= false

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "FreeBSD")),)
 freebsd = true
endif

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "Linux")),)
 linux = true
endif

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "linux")),)
 linux = true
endif

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "mingw")),)
 win32 = true
 mingw = true
endif

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "target=powerpc")),)
 bigendian = true
endif

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "Apple Computer, Inc")),)
 osx = true
endif

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "cygming")),)
 cygwin = true
endif

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "solaris")),)
 solaris = true
endif

# warn if using non-gnu make

ifeq ($(strip $(shell make -v |grep "GNU")),)
 $(warning WARNING: You are using non-gnu make, try "gmake" if build fails)
endif

# Flags to deutex

DEUTEX_FLAGS = -rgb 0 255 255 

# Platform and sdl-config flags
X11_LFLAGS = -L/usr/X11R6/lib -lX11

SDL_CFLAGS_COMMAND = sdl-config --cflags
SDL_LFLAGS_COMMAND = sdl-config --libs

SDL_DETECT = sdl-config --version
SDL_CFLAGS = $(shell $(SDL_CFLAGS_COMMAND))
SDL_LFLAGS = $(shell $(SDL_LFLAGS_COMMAND)) $(X11_LFLAGS)

CFLAGS_PLATFORM = -DUNIX
LFLAGS_PLATFORM =

ifeq ($(strip $(osx)), true)
# osx does not use X11 for copy&paste, it uses Carbon
CFLAGS_PLATFORM = -DUNIX -DOSX
LFLAGS_PLATFORM =
OSX_FRAMEWORKS = -framework Carbon -framework AudioToolbox -framework AudioUnit
SDL_LFLAGS := $(SDL_LFLAGS) $(OSX_FRAMEWORKS)
X11_LFLAGS = 
endif

ifeq ($(strip $(freebsd)), true)
ifeq ($(SDL_CFLAGS),)
SDL_CFLAGS_COMMAND = sdl11-config --cflags
SDL_LFLAGS_COMMAND = sdl11-config --libs
SDL_CFLAGS = $(shell $(SDL_CFLAGS_COMMAND))
SDL_LFLAGS = $(shell $(SDL_LFLAGS_COMMAND)) $(X11_LFLAGS)
endif
endif

ifeq ($(strip $(cygwin)), true)
SDL_CFLAGS = $(shell $(SDL_CFLAGS_COMMAND))
SDL_LFLAGS = $(shell $(SDL_LFLAGS_COMMAND))
LFLAGS_PLATFORM = -mno-cygwin -lwsock32 -lwinmm
CFLAGS_PLATFORM = -mno-cygwin -DWIN32 -D_WIN32
endif

ifeq ($(strip $(win32)), true)
LFLAGS_PLATFORM = -lwsock32
CFLAGS_PLATFORM = -DWIN32 -D_WIN32
endif

ifeq ($(strip $(linux)), true)
CFLAGS_PLATFORM = -DUNIX -DLINUX
endif

ifeq ($(strip $(solaris)), true)
LFLAGS_PLATFORM = -lsocket -lnsl
CFLAGS_PLATFORM = -DSOLARIS -DUNIX -DBSD_COMP -gstabs+
endif

ifeq ($(strip $(bigendian)), true)
CFLAGS_PLATFORM := $(CFLAGS_PLATFORM) -D__BIG_ENDIAN__
endif

# Directories
BINDIR = .
INSTALLDIR = /usr/local/bin
RESDIR = /usr/local/share

# Textscreen
TEXTSCREEN_DIR = textscreen
TEXTSCREEN_HEADERS = $(wildcard $(TEXTSCREEN_DIR)/*.h)
TEXTSCREEN_SOURCES = $(wildcard $(TEXTSCREEN_DIR)/*.cpp)
TEXTSCREEN_OBJS = $(patsubst $(TEXTSCREEN_DIR)/%.cpp,$(OBJDIR)/$(TEXTSCREEN_DIR)/%.o,$(TEXTSCREEN_SOURCES))

# JsonCpp 
JSONCPP_DIR = jsoncpp
JSONCPP_HEADERS = $(wildcard $(JSONCPP_DIR)/*.h)
JSONCPP_SOURCES = $(wildcard $(JSONCPP_DIR)/*.cpp)
JSONCPP_OBJS = $(patsubst $(JSONCPP_DIR)/%.cpp,$(OBJDIR)/$(JSONCPP_DIR)/%.o,$(JSONCPP_SOURCES))

# Common
COMMON_DIR = common
COMMON_HEADERS = $(wildcard $(COMMON_DIR)/*.h)
COMMON_SOURCES = $(wildcard $(COMMON_DIR)/*.cpp)
COMMON_OBJS = $(patsubst $(COMMON_DIR)/%.cpp,$(OBJDIR)/$(COMMON_DIR)/%.o,$(COMMON_SOURCES))
COMMON_OBJS_SERVER = $(patsubst $(COMMON_DIR)/%.cpp,$(OBJDIR)/$(COMMON_DIR)/server_%.o,$(COMMON_SOURCES))
COMMON_OBJS_CLIENT = $(patsubst $(COMMON_DIR)/%.cpp,$(OBJDIR)/$(COMMON_DIR)/client_%.o,$(COMMON_SOURCES))

# Server
SERVER_DIR = server/src
SERVER_HEADERS = $(wildcard $(SERVER_DIR)/*.h)
SERVER_SOURCES = $(wildcard $(SERVER_DIR)/*.cpp)
SERVER_OBJS = $(patsubst $(SERVER_DIR)/%.cpp,$(OBJDIR)/$(SERVER_DIR)/%.o,$(SERVER_SOURCES))
SERVER_TARGET = $(BINDIR)/odasrv
SERVER_CFLAGS = -I../server/src -Iserver/src -Ijsoncpp -DJSON_IS_AMALGAMATION
SERVER_LFLAGS =

# Client
CLIENT_DIR = client/src
CLIENT_HEADERS = $(wildcard $(CLIENT_DIR)/*.h)
CLIENT_SOURCES = $(wildcard $(CLIENT_DIR)/*.cpp)
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.cpp,$(OBJDIR)/$(CLIENT_DIR)/%.o,$(CLIENT_SOURCES))
CLIENT_TARGET = $(BINDIR)/odamex
CLIENT_CFLAGS = -I../client/src -Iclient/src
CLIENT_LFLAGS =

# Master
MASTER_DIR = master
MASTER_HEADERS = master/i_net.h common/version.h
MASTER_SOURCES = master/i_net.cpp master/main.cpp
MASTER_OBJS = $(patsubst $(MASTER_DIR)/%.cpp,$(OBJDIR)/$(MASTER_DIR)/%.o,$(MASTER_SOURCES))
MASTER_TARGET = $(BINDIR)/odamaster
MASTER_CFLAGS =

# WAD file

WADFILE_TARGET = $(BINDIR)/odamex.wad

# Fix cygwin targets, as they will be postfixed with ".EXE"
ifeq ($(strip $(cygwin)), true)
SERVER_TARGET := $(SERVER_TARGET).exe
CLIENT_TARGET := $(CLIENT_TARGET).exe
MASTER_TARGET := $(MASTER_TARGET).exe
endif

# Targets
TARGETS = $(SERVER_TARGET) $(CLIENT_TARGET) $(MASTER_TARGET) $(WADFILE_TARGET)

# denis - fixme - cflags are quite messy, but removing these is a very delicate act, also use -Wall -Werror
CFLAGS = $(CFLAGS_PLATFORM) -DNOASM -Icommon -g -Wall -O2
LFLAGS = $(LFLAGS_PLATFORM)

CFLAGS_RELEASE = $(CFLAGS_PLATFORM) -DNOASM -Icommon -O3
LFLAGS_RELEASE = $(LFLAGS_PLATFORM)

# denis - fixme - mingw32 hack
ifeq ("$(findstring mingw32, $(MAKE))","mingw32")
SERVER_HEADERS_2 = $(wildcard $(SERVER_DIR)/*.h)
SERVER_HEADERS_WIN32 = $(wildcard $(SERVER_DIR)/../win32/*.h)
SERVER_SOURCES_2 = $(wildcard $(SERVER_DIR)/*.cpp)
SERVER_SOURCES_WIN32 = $(wildcard $(SERVER_DIR)/../win32/*.cpp)
SERVER_OBJS = $(patsubst $(SERVER_DIR)/%.cpp,$(OBJDIR)/$(SERVER_DIR)/%.o,$(SERVER_SOURCES_2)) $(patsubst $(SERVER_DIR)/../win32/%.cpp,$(OBJDIR)/$(SERVER_DIR)/../win32/%.o,$(SERVER_SOURCES_WIN32))
SERVER_SOURCES = $(SERVER_SOURCES_2) $(SERVER_SOURCES_WIN32)
SERVER_HEADERS = $(SERVER_HEADERS_2) $(SERVER_HEADERS_WIN32)
SERVER_CFLAGS = -I../server/win32 -Iserver/win32 -I../server/src -Iserver/src
MKDIR = echo *** PLEASE CREATE THIS DIRECTORY: 
CFLAGS = -D_WIN32 -D_CONSOLE -DNOASM -Icommon -ggdb
LFLAGS = -lwsock32 -lwinmm 
endif
# denis - end fixme - mingw32 hack

# denis - fixme - sdl client hack for linux
CLIENT_HEADERS_2 = $(wildcard $(CLIENT_DIR)/*.h)
CLIENT_HEADERS_WIN32 = $(wildcard $(CLIENT_DIR)/../sdl/*.h)
CLIENT_SOURCES_2 = $(wildcard $(CLIENT_DIR)/*.cpp)
CLIENT_SOURCES_WIN32 = $(wildcard $(CLIENT_DIR)/../sdl/*.cpp)
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.cpp,$(OBJDIR)/$(CLIENT_DIR)/%.o,$(CLIENT_SOURCES_2)) $(patsubst $(CLIENT_DIR)/../sdl/%.cpp,$(OBJDIR)/$(CLIENT_DIR)/../sdl/%.o,$(CLIENT_SOURCES_WIN32))
CLIENT_SOURCES = $(CLIENT_SOURCES_2) $(CLIENT_SOURCES_WIN32)
CLIENT_HEADERS = $(CLIENT_HEADERS_2) $(CLIENT_HEADERS_WIN32)
CLIENT_CFLAGS = -Itextscreen -I../client/sdl -Iclient/sdl -I../client/src -Iclient/src $(SDL_CFLAGS)
CLIENT_LFLAGS =  $(SDL_LFLAGS) -lSDL_mixer
#-ldmalloc
# denis - end fixme

# All
all: $(SERVER_TARGET) $(CLIENT_TARGET) $(WADFILE_TARGET)

# Textscreen
$(OBJDIR)/$(TEXTSCREEN_DIR)/%.o: $(TEXTSCREEN_DIR)/%.cpp $(TEXTSCREEN_HEADERS) $(COMMON_HEADERS)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

# JsonCpp
$(OBJDIR)/$(JSONCPP_DIR)/%.o: $(JSONCPP_DIR)/%.cpp $(JSONCPP_HEADERS) $(COMMON_HEADERS)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(SERVER_CFLAGS) -c $< -o $@

# Common for server
$(OBJDIR)/$(COMMON_DIR)/server_%.o: $(COMMON_DIR)/%.cpp $(COMMON_HEADERS) $(SERVER_HEADERS)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(SERVER_CFLAGS) -c $< -o $@

# Common for client
$(OBJDIR)/$(COMMON_DIR)/client_%.o: $(COMMON_DIR)/%.cpp $(COMMON_HEADERS) $(CLIENT_HEADERS)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

# Client
client: $(CLIENT_TARGET)
	@echo Detected SDL: $(shell $(SDL_DETECT))

$(CLIENT_TARGET): $(TEXTSCREEN_OBJS) $(COMMON_OBJS_CLIENT) $(CLIENT_OBJS)
	$(LD) $(CLIENT_OBJS) $(TEXTSCREEN_OBJS) $(COMMON_OBJS_CLIENT) $(CLIENT_LFLAGS) $(LFLAGS) -o $(CLIENT_TARGET)

$(OBJDIR)/$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.cpp $(CLIENT_HEADERS) $(COMMON_HEADERS) $(TEXTSCREEN_HEADERS)
ifeq ($(SDL_CFLAGS),)
	@echo Make sure SDL is installed and sdl-config is accessible
	@exit 2
endif
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

# Server
server: $(SERVER_TARGET)
$(SERVER_TARGET): $(JSONCPP_OBJS) $(COMMON_OBJS_SERVER) $(SERVER_OBJS)
	$(LD) $(SERVER_OBJS) $(JSONCPP_OBJS) $(COMMON_OBJS_SERVER) $(SERVER_LFLAGS) $(LFLAGS) -o $(SERVER_TARGET)

$(OBJDIR)/$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.cpp $(SERVER_HEADERS) $(COMMON_HEADERS) $(JSONCPP_HEADERS)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(SERVER_CFLAGS) -c $< -o $@

# Master
master: $(MASTER_TARGET)
$(MASTER_TARGET): $(MASTER_OBJS)
	$(LD) $(LFLAGS) $(MASTER_OBJS) -o $(MASTER_TARGET)

$(OBJDIR)/$(MASTER_DIR)/%.o: $(MASTER_DIR)/%.cpp
	@$(MKDIR) $(dir $@)
	$(CC) $(MASTER_CFLAGS) $(CFLAGS) -c $< -o $@

$(WADFILE_TARGET) :
	(cd wad; $(DEUTEX) $(DEUTEX_FLAGS) -doom2 bootstrap -build wadinfo.txt ../$@)

odalaunch/odalaunch:
	cd odalaunch && make && cd ..

# Checker
check: test
test: server client
	tests/all.tcl

# Installer
install: $(CLIENT_TARGET) $(SERVER_TARGET) odalaunch/odalaunch
	$(MKDIR) $(INSTALLDIR)
	$(INSTALL) $(SERVER_TARGET) $(INSTALLDIR)
	$(INSTALL) $(CLIENT_TARGET) $(INSTALLDIR)
ifneq ($(wildcard $(MASTER_TARGET)),)
	$(INSTALL) $(MASTER_TARGET) $(INSTALLDIR)
endif
	$(INSTALL) odalaunch/odalaunch $(INSTALLDIR)
	$(MKDIR) $(RESDIR)/doom
	cp odamex.wad $(RESDIR)/doom

uninstall:
	rm $(INSTALLDIR)/$(CLIENT_TARGET)
	rm $(INSTALLDIR)/$(SERVER_TARGET)
ifneq ($(wildcard $(INSTALLDIR)/$(MASTER_TARGET)),)
	rm $(INSTALLDIR)/$(MASTER_TARGET)
endif
	rm $(INSTALLDIR)/odalaunch
	rm $(RESDIR)/doom/odamex.wad

install-res: 
	$(MKDIR) $(RESDIR)
	$(INSTALL) $(BINDIR)/media/icon_odamex_96.png $(RESDIR)/pixmaps/odamex.png
	$(INSTALL) $(BINDIR)/media/icon_odasrv_96.png $(RESDIR)/pixmaps/odasrv.png
	$(INSTALL) $(BINDIR)/media/icon_odalaunch_96.png $(RESDIR)/pixmaps/odalaunch.png
	$(INSTALL) $(BINDIR)/installer/arch/odamex.desktop $(RESDIR)/applications
	$(INSTALL) $(BINDIR)/installer/arch/odalaunch.desktop $(RESDIR)/applications

uninstall-res:
	rm $(RESDIR)/pixmaps/odalaunch.png
	rm $(RESDIR)/pixmaps/odamex.png
	rm $(RESDIR)/pixmaps/odasrv.png
	rm $(RESDIR)/applications/odamex.desktop
	rm $(RESDIR)/applications/odalaunch.desktop

# Clean
clean:
	@rm -rf $(COMMON_OBJS_CLIENT) $(COMMON_OBJS_SERVER) $(SERVER_OBJS) $(CLIENT_OBJS) $(MASTER_OBJS)
	@rm -rf $(CLIENT_TARGET)
	@rm -rf $(SERVER_TARGET)
	@rm -rf $(MASTER_TARGET)

# Help
help:
	@echo ============================
	@echo Odamex Makefile help
	@echo	============================
	@echo To build EVERYTHING: make
	@echo To build $(SERVER_TARGET): make server
	@echo To build $(CLIENT_TARGET): make client
	@echo To build $(MASTER_TARGET): make master
	@echo To remove built files: make clean
	@echo To install built binaries: make install
	@echo To install resources: make install-res
	@echo To uninstall resources: make uninstall-res
	@echo To uninstall binaries: make uninstall
	@echo	----------------------------
	@echo Binaries will be built in: $(BINDIR)
	@echo Object files will be located in: $(OBJDIR) 
	@echo Binaries will be installed on the system in: $(INSTALLDIR)
	@echo Resources will be installed in: $(RESDIR)

