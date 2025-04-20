#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

inline bool IsPointInRectangle(Vector2 point, Rectangle rect) {
	return (
		point.x >= rect.x
		&& point.x <= rect.x + rect.width
		&& point.y >= rect.y
		&& point.y <= rect.y + rect.height
	);
}

inline Vector2 ClosestRectCorner(Rectangle rect, Vector2 point) {
	float closestX = point.x < rect.x ? rect.x : rect.x + rect.width;
	float closestY = point.y < rect.y ? rect.y : rect.y + rect.height;
	return (Vector2){ closestX, closestY };
}

inline bool IsPointInCircle(Vector2 point, Circle circle) {
	float xDistance = fabs(point.x - circle.center.x);
	float yDistance = fabs(point.y - circle.center.y);
	if (xDistance > circle.radius || yDistance > circle.radius) {
		return false;
	}
	if (xDistance + yDistance <= circle.radius) {
		return true;
	}
	return (xDistance * xDistance + yDistance * yDistance <= circle.radius * circle.radius);
}

bool DoesRectCollideCircle(Rectangle rect, Circle circle) {
	if (IsPointInRectangle(circle.center, rect)) {
		return true;
	}
	Vector2 closestCorner = ClosestRectCorner(rect, circle.center);

	return IsPointInCircle(closestCorner, circle);
}

bool DoesRectCollideRect(Rectangle rect, Rectangle rect2) {
	if (rect.x > rect2.x + rect2.width) {
		return false;
	}
	if (rect2.x > rect.x + rect.width) {
		return false;
	}
	if (rect.y + rect.height < rect2.y) {
		return false;
	}
	if (rect2.y + rect2.height < rect.y) {
		return false;
	}

	return true;
}

inline bool IsBitSet(int val, int bit) {
	return val & (1 << (bit - 1));
}

static inline void GetXPointDir(char* dirs, float originX, float targetX) {
	// Target X above origin X means it's to the east.
	if (targetX > originX) {
		*dirs = (*dirs) ^ 1;
	}
	// Otherwise, west.
	if (targetX < originX) {
		*dirs = (*dirs) ^ (1 << 1);
	}
}

static inline void GetYPointDir(char* dirs, float originY, float targetY) {
	// Target Y less than origin Y means target is to origin's north.
	if (targetY < originY) {
		*dirs = (*dirs) ^ (1 << 3);
	}
	// Target is to the south (Y bigger than origin Y).
	if (targetY > originY) {
		*dirs = (*dirs) ^ (1 << 2);
	}
}

inline Direction GetPointDir(Vector2 origin, Vector2 target) {
	char dirs = 0;
	GetXPointDir(&dirs, origin.x, target.x);
	GetYPointDir(&dirs, origin.y, target.y);

	return (Direction) dirs;
}

inline Direction GetPointDirThreshold(Vector2 origin, Vector2 target, float xThreshold, float yThreshold) {
	float xDiff = fabs(origin.x - target.x);
	float yDiff = fabs(origin.y - target.y);
	if (xDiff < xThreshold && yDiff < yThreshold) {
		return NO_DIRECTION;
	}
	char dirs = 0;

	// Check X axis direction.
	if (xDiff > xThreshold) {
		GetXPointDir(&dirs, origin.x, target.x);
	}

	// Check Y axis direction.
	if (yDiff > yThreshold) {
		GetYPointDir(&dirs, origin.y, target.y);
	}

	return (Direction) dirs;
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

ObjectPool CreatePool(const int length, const size_t itemSize) {
	return (ObjectPool){
		.length = length,
		.itemSize = itemSize,
		.active = malloc(sizeof(bool) * length),
		.data = malloc(itemSize * length)
	};
}

void* AddToPool(ObjectPool* pool, void* item) {
	for (int i = 0; i < pool->length; i++) {
		if (!pool->active[i]) {
			pool->active[i] = true;
			pool->activeItems++;
			void* index = PoolIndexAddress(pool, i);
			return memcpy(index, item, pool->itemSize);
		}
	}
	return NULL;
}

void* PoolIndexAddress(ObjectPool* pool, int index) {
	return (void*)((size_t)(pool->data) + (index * pool->itemSize));
}

void RemoveFromPool(ObjectPool* pool, int index) {
	if (!pool->active[index]) {
		return;
	}
	pool->active[index] = false;
	pool->activeItems--;
}

void IteratePool(ObjectPool* pool, PoolItemCallback callback, void* args) {
	if (pool == NULL || callback == NULL || pool->activeItems == 0 || pool->length == 0) {
		return;
	}
	int i = 0, j = 0;
	int maxActive = pool->activeItems;
	while (j < maxActive && i < pool->length) {
		if (pool->active[i]) {
			callback(pool, i, args);
			j++;
		}
		i++;
	}
}

char* IntToString(int val) {
	if (val == 0) {
		char* text = malloc(2);
		text[0] = '0';
		text[1] = '\0';
		return text;
	}
	int needed = snprintf(0, 0, "%d", val);
	char* text = malloc(needed);
	sprintf(text, "%d", val);

	// Caller needs to remember to free.
	return text;
}
