CXX = g++
LD  = g++

PROGRAM = ag-odalaunch

SOURCES = \
	src/agol_about.cpp \
	src/agol_dirsel.cpp \
	src/agol_main.cpp \
	src/agol_manual.cpp \
	src/agol_settings.cpp \
	src/agol_solo.cpp \
	src/event_handler.cpp \
	src/game_command.cpp \
	src/gui_config.cpp \
	src/main.cpp \
	src/net_io.cpp \
	src/net_packet.cpp

HEADERS = src/*.h

AGCONFIG = agar-config

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

CXXFLAGS = $(shell $(AGCONFIG) --cflags)
LFLAGS = $(shell $(AGCONFIG) --libs) -lSDL_image

# mingw extras
ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "mingw")),)
	LFLAGS += -static -lwsock32 -lwinmm -lpng -lz
endif

# Debug Profile (default)
debug: CXXFLAGS += -g -Wall
debug: LFLAGS += -g
debug: $(PROGRAM)

# Release Profile
release: CXXFLAGS += -O3
release: $(PROGRAM)

all: $(debug)

$(PROGRAM).app: $(PROGRAM)
	rm -rf $@
	mkdir $@
	mkdir $@/Contents
	mkdir $@/Contents/Resources
	mkdir $@/Contents/MacOS
	cp res/Info.plist $@/Contents
	cp res/odalaunch.icns $@/Contents/Resources
	echo -n "APPL????" > $@/Contents/PkgInfo
	cp $< $@/Contents/MacOS

$(PROGRAM): $(OBJECTS)
	$(LD) $(OBJECTS) -o $@ $(LFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -Ires -c $< -o $@

clean:
	rm -rf $(PROGRAM).app
	rm -rf $(PROGRAM)
	rm -rf $(OBJECTS)
