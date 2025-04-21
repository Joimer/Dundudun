#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "object-pool.h"
#include "lib.h"

ObjectPool CreatePool(const int length, const size_t itemSize) {
	bool* active = malloc(sizeof(bool) * length);
	void* data = malloc(itemSize * length);
	if (active == NULL || data == NULL) {
		LogDebug("Failed to malloc ObjectPool!!");
	}
	return (ObjectPool){
		.length = length,
		.itemSize = itemSize,
		.active = active,
		.data = data
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
