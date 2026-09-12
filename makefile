CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = build/sandbox.bin
SRC = ./src/sandbox.c
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lcjson

all: $(TARGET)
$(TARGET): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
