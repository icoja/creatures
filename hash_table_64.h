#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdalign.h>

#define MIN_ALLOC_SIZE 128
#define BUCKET_SIZE 4

typedef struct{
	uint32_t src;
	uint32_t dst;
}key64_s;

typedef union{
	key64_s s;
	uint64_t i;
}key64;

static inline key64 make_key64(uint32_t s, uint32_t d)
{
	return (key64){.s = {s, d}};
}

struct entry64{
	key64 key;
	uint32_t value;
	bool used;
};
typedef struct entry64 entry64;

struct bucket64{
	entry64 entries[BUCKET_SIZE];
	struct bucket64 *next;
};
typedef struct bucket64 bucket64;


struct hash_table64{
	bucket64 *table;
	size_t size;
	size_t elements;
};
typedef struct hash_table64 hash_table64;

static inline uint64_t hash(key64 k)
{
	uint32_t src = k.s.src;
	uint32_t dst = k.s.dst;
	uint64_t  b = src * dst + (src * 2501953) + (dst * 3502427);
	return b;
}

int hash_table64_init(hash_table64 *h);

void hash_table64_free(hash_table64 *h);

void hash_table64_insert_new(hash_table64 *h, const key64 key, const uint32_t value);

void hash_table64_insert(hash_table64 *h, const key64 key, const uint32_t value);

void hash_table64_delete(hash_table64 *h, const key64 key);

int hash_table64_search(const hash_table64 *h, const key64 key, uint32_t * const value);

static inline hash_table64 hash_table64_copy(hash_table64 *h)
{
	printf("hashtable_copy not implemented %p\n", (void*)h);
	return (hash_table64){0};
}
