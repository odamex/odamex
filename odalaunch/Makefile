SOURCES = src/dlg_config.cpp \
 src/lst_custom.cpp \
 src/misc.cpp \
 src/net_packet.cpp \
 src/dlg_main.cpp \
 src/dlg_servers.cpp \
 src/main.cpp \
 src/net_io.cpp \
 src/query_thread.cpp

HEADERS = src/*.h

CC = g++
LD = g++

WXCONFIG = wx-config
WXRC = wxrc
CFLAGS = $(shell $(WXCONFIG) --cflags) -g
LFLAGS = $(shell $(WXCONFIG) --libs) -g

RESOURCES = res/xrc_resource.xrc res/icons.xrc

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

odalaunch.app: odalaunch
	rm -rf odalaunch.app 
	mkdir odalaunch.app
	mkdir odalaunch.app/Contents
	mkdir odalaunch.app/Contents/MacOS
	mkdir odalaunch.app/Contents/MacOS/Resources
	echo -n "APPL????" > odalaunch.app/Contents/PkgInfo
	cp odalaunch odalaunch.app/Contents/MacOS

odalaunch: $(OBJECTS)
	$(LD) $(OBJECTS) -o odalaunch $(LFLAGS)

%.o: %.cpp res/xrc_resource.h $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS) -I.

res/xrc_resource.h: $(RESOURCES)
	$(WXRC) -c -o $@ res/*.xrc

clean:
	rm -rf odalaunch.app
	rm -rf odalaunch
	rm -rf $(OBJECTS)
	rm -rf res/xrc_resource.h
