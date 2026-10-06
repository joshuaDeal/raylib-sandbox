CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = build/sandbox.bin
SRC = src/sandbox.c src/buildtool.c src/character.c src/pickup.c src/savesystem.c src/soundsystem.c src/tespecial.c src/uisystem.c
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lcjson

all: $(TARGET)
$(TARGET): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
