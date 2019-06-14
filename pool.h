#pragma once
#include "brain.h"
#include "brain_mutations.h"
#include "vector.h"

#define dist_threshold 5
#define remove_bottom_perc 0.1 // valid interval: (0,1)

typedef uint16_t u16;

DEFINE_VECTOR_TYPE(u16)

struct pool{
	vector_u16 species;
	brain_s *brains;
	uint16_t size;
};

typedef struct pool pool_s;

void print_pool(const pool_s *pool);

void pool_init(pool_s* pool, uint16_t n_brains);
void pool_free(pool_s* pool);

uint16_t n_species(const pool_s *pool); // era una static inline
brain_s best(const pool_s *pool);

void evolve(pool_s *pool, float (*test)(const brain_s*));
