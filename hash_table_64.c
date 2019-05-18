#include "hash_table_64.h"

static inline bool is_over_loaded64(hash_table64 *h, size_t add)//reahashing heuristics
{
	return (h->elements + add) * 4 > 3 * h->size * BUCKET_SIZE / 2;
}

int hash_table64_init(hash_table64 *h)
{
	if(!(h->table = calloc(sizeof(bucket64), MIN_ALLOC_SIZE))){
		return 0;
	}
	h->size = MIN_ALLOC_SIZE;
	h->elements = 0;
	return 1;
}

static inline void delete_list_64(bucket64 *b)
{
	while (b){
		bucket64 *a = b->next;
		free(b);
		b = a;
	}
}

void hash_table64_free(hash_table64 *h)
{
	for (size_t i = 0; i < h->size; i++){
		delete_list_64(h->table[i].next);
	}
	free(h->table);
}

static int hash_table64_realloc(hash_table64 *h)
{
	printf("reallocating hash_map...\n");
	hash_table64 j;
	if(!(j.table = calloc(sizeof(bucket64), h->size * 2))){
		printf("Failed to reallocate hash_map. Aborting.");
		abort();
		return 0;
	}
	j.size = h->size * 2;
	j.elements = 0;

	//re-add every element to the new hashmap;
	for (size_t i = 0; i < h->size; i++){ //iterate through the keys
		bucket64 *b = &h->table[i];
		while(b){
			for (size_t i = 0; i < BUCKET_SIZE; i++){ //iterate through the linked blocks
				if(b->entries[i].used){
					hash_table64_insert_new(&j, b->entries[i].key, b->entries[i].value);
				}
			}
			b = b->next;
		}
	}
	hash_table64_free(h);
	*h = j;
	return 1;
}

void hash_table64_insert_new(hash_table64 *h, const key64 key, const uint32_t value)
{
	//this breaks the data structure if key is already present
	if(is_over_loaded64(h, 1)){
		hash_table64_realloc(h);
	}
	bucket64 *b = &h->table[hash(key) % h->size];
	while(true){
		for (size_t i = 0; i < BUCKET_SIZE; i++){
			if(!b->entries[i].used){
				b->entries[i].used = true;
				b->entries[i].value = value;
				b->entries[i].key = key;
				h->elements++;
				return;
			}
		}
		if (b->next)
			b = b->next;
		else{
			b = b->next = calloc(sizeof(bucket64), 1);//might fail
			assert(b);
		}
	}
}

void hash_table64_insert(hash_table64 *h, const key64 key, const uint32_t value)
{
	bucket64 *b = &h->table[hash(key) % h->size];
	//iterate through the linked blocks corresponding to key. (key % h->size)
	while(true){
		//look for key. do not insert new
		for (size_t i = 0; i < BUCKET_SIZE; i++){
			if(b->entries[i].used && b->entries[i].key.i == key.i){
				b->entries[i].value = value;
				return;
			}
		}
		if (b->next)
			b = b->next;
		else{
			hash_table64_insert_new(h, key, value);//this also increases h->elements
			return;
		}
	}
}

void hash_table64_delete(hash_table64 *h, const key64 key){
	bucket64 *b = &h->table[hash(key) % h->size];
	while(b){
		for (size_t i = 0; i < BUCKET_SIZE; i++){
			if(b->entries[i].used && b->entries[i].key.i == key.i){
				b->entries[i].used = false;
				return;
			}
		}
		b = b->next;
	}
}

int hash_table64_search(const hash_table64 *h, const key64 key, uint32_t * const value)
{
	bucket64 *b = &h->table[hash(key) % h->size];
	while(true){
		for (size_t i = 0; i < BUCKET_SIZE; i++){
			if(b->entries[i].used && b->entries[i].key.i == key.i){
				if (value){
					*value = b->entries[i].value;
				}
				return 1;
			}
		}
		if (b->next)
			b = b->next;
		else{
			return 0;
		}
	}
}

