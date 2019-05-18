#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdalign.h>

#define MIN_ALLOC_SIZE 128
#define BUCKET_SIZE 4

struct entry{
	uint32_t value;
	uint32_t key;
	bool used;
};
typedef struct entry entry;

struct bucket{
	entry entries[BUCKET_SIZE];
	struct bucket *next;
};
typedef struct bucket bucket;


struct hash_table{
	bucket *table;
	size_t size;
	size_t elements;
};
typedef struct hash_table hash_table;

int hash_table_init(hash_table *h);

void hash_table_free(hash_table *h);

void hash_table_insert_new(hash_table *h, const uint32_t key, const uint32_t value);

void hash_table_insert(hash_table *h, const uint32_t key, const uint32_t value);

void hash_table_delete(hash_table *h, uint32_t key);

int hash_table_search(const hash_table *h, const uint32_t key, uint32_t * const value);

static inline hash_table hashtable_copy(hash_table *h)
{
	printf("hashtable_copy not implemented %p\n", (void*)h);
	return (hash_table){0};
}

