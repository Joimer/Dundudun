#ifndef POOL_H
#define POOL_H

#include <stdio.h>

#define MemberSize(type, member) (sizeof(((type*) 0)->member))
#define CreatePoolOf(type, length) CreatePool(length, sizeof(type));

typedef enum { LEVEL_TEXT_POOL, LEVEL_ATTACK_POOL } PoolId;

typedef struct {
	int length;
	int activeItems;
	size_t itemSize;
	bool* active;
	void* data;
} ObjectPool;

typedef void (*PoolItemCallback)(ObjectPool* pool, int index, void* args);

ObjectPool CreatePool(const int length, const size_t itemSize);
void* AddToPool(ObjectPool* pool, void* item);
void RemoveFromPool(ObjectPool* pool, int index);
void* PoolIndexAddress(ObjectPool* pool, int index);
void IteratePool(ObjectPool* pool, PoolItemCallback callback, void* args);

#endif
