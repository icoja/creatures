#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "pool.h"
#include "brain_mutations.h"
#include "random_pcg.h"



void pool_init(pool_s *pool, uint16_t n_brains)
{
	vector_u16_construct(&pool->species);
	pool->size = n_brains;
	pool->brains = calloc(sizeof(brain_s), n_brains);
}

void pool_free(pool_s* pool)
{
	free(pool->brains);
	vector_u16_free(&pool->species);
}

static void evaluate_fitness(pool_s *pool, float (*test)(const brain_s*))
{
	for (size_t i = 0; i < pool->size; i++){
		pool->brains[i].fitness = test(pool->brains + i);
	}

}

uint16_t n_species(const pool_s *pool){
	return pool->species.size / (pool->size + 1);
}

static inline void swap(link_s *a, link_s *b){link_s tmp = *a; *a = *b; *b = tmp;}

static size_t partition(vector_link_s *A, size_t p, size_t r)
{
	size_t i = p;
	for (size_t j = p; j < r - 1; ++j){
		if (A->data[j].innov_number <= A->data[r - 1].innov_number){
			swap(&A->data[i], &A->data[j]);
			++i;
		}
	}
	// the last place requires no test:
	swap(&A->data[i], &A->data[r-1]);
	return i;
}

static void quick_sort_links_inner(vector_link_s *A, size_t p, size_t r)
{
	if (r - p > 1){
		size_t q = partition(A,p,r);
		quick_sort_links_inner(A, p, q);
		quick_sort_links_inner(A, q + 1, r);
	}
}

static void quick_sort_links(vector_link_s *A)
{
	quick_sort_links_inner(A, 0, A->size);
}

static float brain_dist(const brain_s *b, const brain_s *c){
	vector_link_s b_links = vector_link_s_copy(&(b->links));
	vector_link_s c_links = vector_link_s_copy(&(c->links));

	quick_sort_links(&b_links);
	quick_sort_links(&c_links);

	size_t j = 0, i = 0; // indice del less fit
	uint32_t n_different = 0;
	float weight_sum = 0;
	uint32_t n_same = 0;

	while (i < b_links.size && j < c_links.size){
		link_s current_b = b_links.data[i];
		link_s current_c = c_links.data[j];

		if (current_c.innov_number < current_b.innov_number) {
			j++;
		} else if (current_c.innov_number > current_b.innov_number) {
			i++;
		} else if (current_c.innov_number == current_b.innov_number) {
			j++;
			i++;
			n_same++;
			weight_sum += fabs(current_b.weight - current_c.weight);
		}

	}
	n_different =  b_links.size + c_links.size - 2 * n_same;

	// free vectors
	vector_link_s_free(&b_links);
	vector_link_s_free(&c_links);

	float N = 1; // parametri segreti
	float m = 1;

	//printf("different %d, weight sum %f, same %d\n", n_different, weight_sum, n_same);

	float diff = (float)n_different / N + weight_sum / fmax(n_same, 1) * m;
	assert(diff >= 0);
	return diff;
}


#define species(i, j) species.data[i * (pool->size + 1) + j]
static void speciation(pool_s *pool)
{
	const float dist_threshold = 0.5;
	for (uint16_t i = 0; i < n_species(pool); i++){
		pool->species(i, 0) = 0;
	}
	for (uint16_t i = 0; i < pool->size; i++){
		bool needs_realloc = true;
		uint16_t j;
		for (j = 0; j < n_species(pool); j++){
			float dist = brain_dist(&pool->brains[i], &pool->brains[pool->species(j, 1)]);
			//printf("distanza tra %d e specie %d = %f\n", i, j, dist);
			if (dist <= dist_threshold || pool->species(j, 0) == 0){
				pool->species(j, ++pool->species(j, 0)) = i;
				needs_realloc = false;
				break;
			}
		}
		if (needs_realloc){
			//printf("realloco\n");
			for (uint16_t i = 0; i < pool->size + 1; i++) {
				vector_u16_push_back(&pool->species, 0);
			}
			pool->species(j, ++pool->species(j, 0)) = i;
		}

	}

}



void reproduction(pool_s *pool)
{
	brain_s *new_brains = calloc(sizeof(brain_s), pool->size);
	float *avg = calloc(sizeof(float), n_species(pool));
	float sum_avg = 0;
	for (uint16_t j = 0; j < n_species(pool); j++){
		if (pool->species(j, 0) == 0) break;
		for (uint16_t i = 0; i < pool->species(j, 0); i++){
			avg[j] += pool->brains[pool->species(j, i + 1)].fitness;
		}
		avg[j] /= pool->species(j, 0);
		sum_avg += avg[j];
	}

	uint16_t i = 0;
	uint16_t deficit = pool->size; //number brains to add
	for (uint16_t j = 0; j < n_species(pool); j++){
		assert(sum_avg); // non dovrebbe essere un assert, dovrebbe handlare il caso
		uint16_t offspring = pool->size / sum_avg * avg[j];
		deficit -= offspring;
		for (uint16_t o = 0; o < offspring; o++){
			int father = pcg32_random_r(&rng) % pool->species(j, 0);
			int mother = pcg32_random_r(&rng) % pool->species(j, 0);
			brain_s son = brain_crossover(&pool->brains[father], &pool->brains[mother]);
			brain_mutate(&son);
			new_brains[i++] = son;

		}
	}

	// assert(deficit <= n_species(pool)); // perche??
	for (uint16_t o = 0; o < deficit; o++) {
		uint16_t father = pcg32_random_r(&rng) % pool->size;
		uint16_t mother = pcg32_random_r(&rng) % pool->size;
		brain_s son = brain_crossover(&pool->brains[father], &pool->brains[mother]);
		brain_mutate(&son);
		new_brains[i++] = son;
	}

	assert(i == pool->size);
	for (size_t i = 0; i < pool->size; i++) {
		brain_free(&pool->brains[i]);
	}
	free(pool->brains);
	free(avg);
	pool->brains = new_brains;
}

#undef species


void evolve(pool_s *pool, float (*test)(const brain_s*))
{
	evaluate_fitness(pool, test);
	speciation(pool);
	reproduction(pool);

}



/*

	//divisione in specie
	per ogni brain b:
		per ogni specie s:
			se dist(b, s) < threshold:
				aggiungi b a s
	//riproduzione?
	pool_nuova p
	per ogni specie s:
		numero_figli = euristica(s) //dipende da fitness medio ed elementi della specie
		per ogni figlio:
			prendo due a caso a, b in s:
				c = muta(crossover(a, b))
				aggiungi c a nuova_pool
	pool = pool_nuova;
}
*/
