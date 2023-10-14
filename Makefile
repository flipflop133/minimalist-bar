CC = cc
DEBUG_FLAGS = -g -Wall -Wextra
RELEASE_FLAGS = -O3

SRC_DIR = modules
SRC_FILES = \
    main.c \
    display.c \
    i3ipc.c \
    $(SRC_DIR)/date.c

OUT = i3status
LIBRARIES = -lpulse -lbluetooth -I/usr/include -lcjson -I/usr/include/freetype2/ -lX11 -lXft

default: release

debug: $(SRC_FILES)
	$(CC) $(DEBUG_FLAGS) $(LIBRARIES) $(SRC_FILES) -o $(OUT)_debug

release: $(SRC_FILES)
	$(CC) $(RELEASE_FLAGS) $(LIBRARIES) $(SRC_FILES) -o $(OUT)

clean:
	rm -f $(OUT) $(OUT)_debug

.PHONY: debug release clean
