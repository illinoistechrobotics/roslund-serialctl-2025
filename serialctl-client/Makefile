CC=gcc
CFLAGS=-c -Wall -Werror -g
#LDFLAGS=-static

ifeq ($(OS), Windows_NT)
	PLATFORM_SRC = serio_win32.c
	EXECUTABLE = serialctl.exe
	LDFLAGS=-mconsole
else
	PLATFORM_SRC = serio_posix.c
	EXECUTABLE = serialctl
	LDFLAGS =
endif

SOURCES=base64.c crc16.c $(PLATFORM_SRC) main.c joystick.c
OBJECTS=$(SOURCES:.c=.o)
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)
#SDL_LDFLAGS += -static
all: $(SOURCES) $(EXECUTABLE)
	    
$(EXECUTABLE): $(OBJECTS) 
	    $(CC) $(OBJECTS) -o $@ $(SDL_LDFLAGS) $(LDFLAGS) 
.c.o:
	    $(CC) $< -o $@ $(SDL_CFLAGS) $(CFLAGS) 
clean:
	    rm *.o $(EXECUTABLE)
#Building on windows
#	Install MSYS2 from https://www.msys2.org
#	Open the "MSYS2 UCRT64" shortcut
#	pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 make
#	Go navigate to the file directory you want to build/make
#	make or make windows in the UCRT64 shell
# Compiling for Windows
# 	Adapted so it will compile natively by using serio win or posix (windows or linux)

# To run you need the COM port and joystick number in terminal
# Example: serialctl.exe COM5 0 
# To run you need to pair the robot over bluetooth.
#	Open device manager and expand ports (COM and LPT)
#	Bluetooth SPP devices create two entries, an outgoing/ingoing one
#	Connect to the outgoing from your laptop. If you choose wrong you will get an error.
# To choose the right controller
#	It is usually 0 if you only have one controller plugged in. If you are not sure run joystick_init
#	and it prints the count and name of whichever one it opened so a wrong guess is obvious immediately.
# Example output:
# serial port opened: \\.\COM5
# 1 joysticks were found.
# Opened Joystick 0
# Name: Xbos Controller
# Number of Axes: 6
# ...
# Waiting for safe stick position
# Sticks safe!
# LX: 128, LY: 128, RX:128, RY: 128, CRC: ..., Resp: ...

windows: $(EXECUTABLE)
	@echo "Creating release directory..."
	mkdir -p release
	@echo "Copying executable to release folder..."
	cp "$(EXECUTABLE)" release/
	@echo "Copying required DLLs to release folder..."
	@echo "Ignore errors from Adoptium, those files are unnecessary anyway"
	@for dll in $$(ldd ./$(EXECUTABLE) | grep -o -i '/[^ ]*\.dll' | grep -v -i '/c/Windows/'); do \
		echo "$$dll"; \
		cp "$$dll" release/; \
	done
	@echo "Built serialctl & DLLs in ./release/"
