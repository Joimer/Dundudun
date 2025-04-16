#ifndef LEVEL_H
#define LEVEL_H

#define SEED_LENGTH 8
#define BITS_PER_SEED_CHAR 6

typedef struct {

} Tile;

typedef struct {

} Level;

Level generateLevel(int floor, unsigned long seed);

#endif
