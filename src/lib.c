#include "lib.h"

int clamp(int num, int min, int max) {
	if (num < min) {
		return min;
	}
	if (num > max) {
		return max;
	}
	return num;
}

float clampf(float num, float min, float max) {
	if (num < min) {
		return min;
	}
	if (num > max) {
		return max;
	}
	return num;
}

// initializes mt[N] with a seed
static inline void InitGenRand(MTRand* rand, unsigned long seed) {
	rand->mt[0] = seed & 0xffffffffUL;
	for (rand->index = 1; rand->index < MTRAND_VECTOR_LENGTH; rand->index++) {
		rand->mt[rand->index] = ((
			1812433253UL * (rand->mt[rand->index - 1] ^ (rand->mt[rand->index - 1] >> 30)) + rand->index
		)) & 0xffffffffUL;
	}
}

// Create a seeded Mersenne Twister PRNG.
MTRand SeedMTRand(unsigned long seed) {
	MTRand rand;
	InitGenRand(&rand, seed);
	return rand;
}

unsigned long GetRandomMTValue(MTRand* rand) {
	unsigned long y;
	static unsigned long mag01[2] = { 0x0UL, MTRAND_MATRIX_A };

	// Generate N words at one time.
	if (rand->index >= MTRAND_VECTOR_LENGTH) {
		int kk;

		for (kk = 0; kk < MTRAND_VECTOR_LENGTH - MTRAND_VECTOR_M; kk++) {
			y = (rand->mt[kk] & MTRAND_UPPER_MASK) | (rand->mt[kk + 1] & MTRAND_LOWER_MASK);
			rand->mt[kk] = rand->mt[kk + MTRAND_VECTOR_M] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		for (; kk < MTRAND_VECTOR_LENGTH - 1; kk++) {
			y = (rand->mt[kk] & MTRAND_UPPER_MASK) | (rand->mt[kk + 1] & MTRAND_LOWER_MASK);
			rand->mt[kk] = rand->mt[kk + (MTRAND_VECTOR_M - MTRAND_VECTOR_LENGTH)] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		y = (rand->mt[MTRAND_VECTOR_LENGTH - 1] & MTRAND_UPPER_MASK) | (rand->mt[0] & MTRAND_LOWER_MASK);
		rand->mt[MTRAND_VECTOR_LENGTH - 1] = rand->mt[MTRAND_VECTOR_M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];
		rand->index = 0;
	}

	y = rand->mt[rand->index++];
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}
