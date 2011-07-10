SOURCES = src/dlg_config.cpp \
	src/lst_custom.cpp \
	src/lst_players.cpp \
	src/lst_servers.cpp \
	src/lst_srvdetails.cpp \
	../odalpapi/net_packet.cpp \
	../odalpapi/net_utils.cpp \
	../odalpapi/net_error.cpp \
	../odalpapi/net_io.cpp \
	src/dlg_about.cpp \
	src/dlg_main.cpp \
	src/dlg_servers.cpp \
	src/main.cpp \
	src/query_thread.cpp \
	src/frm_odaget.cpp \
	src/md5.cpp \
	src/str_utils.cpp

HEADERS = src/*.h \
	../odalpapi/*.h

INCLUDES = -I. \
	-I./res \
	-I../odalpapi

CC = g++
LD = g++

WXCONFIG = wx-config
WXRC = wxrc

# Sometimes wx-config will use the lowest version available, which is what
# we do no want! -- GhostlyDeath
ifeq ($(findstring $(strip $(shell $(WXCONFIG) --version 2>&1)), 2.7),)
	WXVERSIONADDITION = --version=2.8
else ifeq ($(findstring $(strip $(shell $(WXCONFIG) --version 2>&1)), 2.6),)
	WXVERSIONADDITION = --version=2.8
else ifeq ($(findstring $(strip $(shell $(WXCONFIG) --version 2>&1)), 2.5),)
	WXVERSIONADDITION = --version=2.8
else ifeq ($(findstring $(strip $(shell $(WXCONFIG) --version 2>&1)), 2.3),)
	WXVERSIONADDITION = --version=2.8
else ifeq ($(findstring $(strip $(shell $(WXCONFIG) --version 2>&1)), 2.2),)
	WXVERSIONADDITION = --version=2.8
else
	WXVERSIONADDITION = 
endif

CFLAGS = $(shell $(WXCONFIG) $(WXVERSIONADDITION) --cflags) -g
LFLAGS = $(shell $(WXCONFIG) $(WXVERSIONADDITION) --libs) -g

# Add the universal build flags if this is wx_mac
ifneq (,$(findstring wx_mac, $(strip $(shell $(WXCONFIG) --basename 2>&1))))
	# g++-4.0 must be used to build with the 10.4 universal sdk
	CC = g++-4.0
	LD = g++-4.0
	CFLAGS += -mmacosx-version-min=10.4 -arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk
	LFLAGS += -mmacosx-version-min=10.4 -arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk
endif

RESOURCES = res/xrc_resource.xrc res/icons.xrc

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

odalaunch.app: odalaunch
	rm -rf odalaunch.app 
	mkdir odalaunch.app
	mkdir odalaunch.app/Contents
	mkdir odalaunch.app/Contents/Resources
	mkdir odalaunch.app/Contents/MacOS
	cp res/Info.plist odalaunch.app/Contents
	cp res/odalaunch.icns odalaunch.app/Contents/Resources
	echo -n "APPL????" > odalaunch.app/Contents/PkgInfo
	cp odalaunch odalaunch.app/Contents/MacOS

odalaunch: $(OBJECTS)
	$(LD) $(OBJECTS) -o odalaunch $(LFLAGS)

%.o: %.cpp res/xrc_resource.h $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

res/xrc_resource.h: $(RESOURCES)
	$(WXRC) -c -o $@ res/*.xrc

clean:
	rm -rf odalaunch.app
	rm -rf odalaunch
	rm -rf $(OBJECTS)
	rm -rf res/xrc_resource.h
