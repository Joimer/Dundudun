CC=cc
CFLAGS=-xo2 -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c99 -Wall -Wno-missing-braces -Wno-unused-value -Wno-pointer-sign -D_DEFAULT_SOURCE
SRC=src
BIN=bin
SRCS=$(wildcard $(SRC)/*.c)

all:
	$(CC) $(SRCS) $(CFLAGS) -o $(BIN)/game

win:
	x86_64-w64-mingw32-gcc $(SRCS) -g -I../raylib/src -L../raylib/src -xo2 -lraylib -lm -lpthread -std=c99 -lopengl32 -lgdi32 -lwinmm -mwindows -o $(BIN)/game.exe

clean:
	rm -f $(BIN)/game $(BIN)/game.exe
