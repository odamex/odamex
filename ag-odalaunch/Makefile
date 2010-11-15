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

CC = g++
LD = g++

AGCONFIG = agar-config

CFLAGS = $(shell $(AGCONFIG) --cflags) -g -Wall
LFLAGS = $(shell $(AGCONFIG) --libs) -g -lSDL_image

ifneq ($(strip $(shell $(CC) -v 2>&1 |grep "mingw")),)
	LFLAGS += -static -lwsock32 -lwinmm -lpng -lz
endif

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

ag-odalaunch: $(OBJECTS)
	$(LD) $(OBJECTS) -o ag-odalaunch $(LFLAGS)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -Ires -c $< -o $@

clean:
	rm -rf ag-odalaunch.app
	rm -rf ag-odalaunch
	rm -rf $(OBJECTS)
