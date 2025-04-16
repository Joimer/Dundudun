#include <stdlib.h>
#include <raylib.h>
#include "level.h"
#include "lib.h"

long int GenerateGameSeed() {
	// Seeds are 48 bits long.
	// 6 bits per character, all numbers and caps ASCII alphabet
	// From 000000000000000000000000000000000000000000000000
	// To   111111111111111111111111111111111111111111111111
	// Max int is usually 2147483647, max uint 4294967295
	long int seed = 0;
	for (int i = 0; i < SEED_LENGTH; i++) {
		long int part = GetRandomValue(0, 35);
		seed |= part << BITS_PER_SEED_CHAR * i;
	}
	return seed;
}

static char SeedIntToChar(int val) {
	if (val > 35) {
		val = val % 36;
	}
	if (val < 10) {
		return (char)(48 + val);
	}
	return (char)(55 + val);
}

const char* SeedToString(long int seed) {
	char* seedString = malloc(SEED_LENGTH + 1);
	for (int i = 0; i < SEED_LENGTH; i++) {
		// Each 6 bits on the number defines a character.
		// Create a mask where only the bits corresponding to the character position are gotten.
		long int mask = (((1 << BITS_PER_SEED_CHAR) - 1) << (i * BITS_PER_SEED_CHAR));
		// We only need the first bits here, so can use int for this.
		int charBits = (int)(seed & mask);
		seedString[i] = SeedIntToChar(charBits);
	}
	// string termination char.
	seedString[SEED_LENGTH] = '\0';

	// Note: This is used once currently, if used more, you would need to free() the result.
	return seedString;
}

Level generateLevel(int floor, long int seed) {
	floor = clamp(floor, 1, MAX_LEVEL);
	Level level = { };
	return level;
}
