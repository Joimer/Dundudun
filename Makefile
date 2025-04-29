CC=cc
CFLAGS=-xo2 -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c99 -Wall -Wno-missing-braces -Wno-unused-value -Wno-pointer-sign -D_DEFAULT_SOURCE
SRC=src
BIN=bin
SRCS=$(wildcard $(SRC)/*.c)

all:
	$(CC) $(SRCS) $(CFLAGS) -o $(BIN)/game

clean:
	rm -rf $(BIN)/game
