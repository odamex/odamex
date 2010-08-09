SOURCES = src/main.cpp \
	src/agol_main.cpp \
	src/agol_settings.cpp \
	src/agol_dirsel.cpp \
	src/agol_solo.cpp \
	src/event_handler.cpp \
	src/net_io.cpp \
	src/net_packet.cpp \
	src/game_command.cpp \
	src/gui_config.cpp

HEADERS = src/*.h

CC = g++
LD = g++

AGCONFIG = agar-config

CFLAGS = $(shell $(AGCONFIG) --cflags) -g
LFLAGS = $(shell $(AGCONFIG) --libs) -lSDL_image -g

# Add the universal build flags if this is wx_mac
#ifneq (,$(findstring wx_mac, $(strip $(shell $(AGCONFIG) --basename 2>&1))))
#	CFLAGS += -mmacosx-version-min=10.4 -arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk
#	LFLAGS += -mmacosx-version-min=10.4 -arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk
#endif

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

#ag-odalaunch.app: ag-odalaunch
#	rm -rf ag-odalaunch.app 
#	mkdir ag-odalaunch.app
#	mkdir ag-odalaunch.app/Contents
#	mkdir ag-odalaunch.app/Contents/Resources
#	mkdir ag-odalaunch.app/Contents/MacOS
#	cp res/Info.plist ag-odalaunch.app/Contents
#	cp res/ag-odalaunch.icns ag-odalaunch.app/Contents/Resources
#	echo -n "APPL????" > ag-odalaunch.app/Contents/PkgInfo
#	cp ag-odalaunch ag-odalaunch.app/Contents/MacOS

ag-odalaunch: $(OBJECTS)
	$(LD) $(OBJECTS) -o ag-odalaunch $(LFLAGS)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -Ires -c $< -o $@

clean:
	rm -rf ag-odalaunch.app
	rm -rf ag-odalaunch
	rm -rf $(OBJECTS)
