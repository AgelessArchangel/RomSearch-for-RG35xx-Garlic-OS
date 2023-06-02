
TARGET = romSearch

CC = $(CROSS_COMPILE)gcc
CFLAGS   = -Os -D_GNU_SOURCE -marm -mtune=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a
LDFLAGS	 = -ldl -lSDL -lSDL_image -lSDL_ttf -lpthread -lm
all:
	@mkdir -p build/res
	$(CC) ./src/main.c ./src/cJSON.c ./src/SDL_rotozoom.c -o ./build/$(TARGET) $(CFLAGS) $(LDFLAGS)
	@cp ./res/* ./build/res

linux:
	$(MAKE) CC=gcc CROSS_COMPILE="" CFLAGS="-D_GNU_SOURCE" --no-print-directory	
	
.PHONY: clean

clean:
	rm -fr ./build
