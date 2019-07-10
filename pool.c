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

float evaluate_fitness(pool_s *pool, float (*test)(const brain_s*))
{
	float max_fitness = 0;
	//uint32_t best_indx = 0;
	float avj_fitness = 0;
	#pragma omp parallel for
	for (size_t i = 0; i < pool->size; i++){
		//printf("brain %zu has fitness %f\n", i, test(pool->brains + i));
		float fitness = test(pool->brains + i);
		if(!(fitness >= 0 && fitness < 1000000)){
			printf("abnormal fitness: %f\n", fitness);
			assert(0);
		}
		pool->brains[i].fitness = fitness;

		if (fitness >= max_fitness){
			max_fitness = fitness;
			//best_indx = i;
		}
		avj_fitness += fitness;
	}
	avj_fitness /= pool->size;
	printf("max fitness: %f, avj: %f\n", max_fitness, avj_fitness);
	return avj_fitness;

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
	// imponi 0 elementi ogni specie
	for (uint16_t i = 0; i < n_species(pool); i++){
		pool->species(i, 0) = 0;
	}
	for (uint16_t i = 0; i < pool->size; i++){
		bool needs_realloc = true;
		uint16_t j;
		for (j = 0; j < n_species(pool); j++){
			float dist = brain_dist(&pool->brains[i], &pool->brains[pool->species(j, 1)]);
			//printf("distanza tra %d e specie %d = %f\n", i, j, dist);
			if (pool->species(j, 0) == 0 || dist <= dist_threshold){
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


void print_species(const pool_s *pool)
{
	printf("species: ci sono %d specie\n", n_species(pool));
	for (int i = 0; i < n_species(pool); i++){
		printf("specie %d: ", i);

		for (int j = 0; j < pool->size + 1; j++){
			printf("%d ", pool->species(i, j));
		}

		printf("\n");
	}
}

static void remove_k_least_fit (const brain_s *brains, uint16_t *specie, uint32_t k)
{
	for (uint32_t c = 0; c < k; c++){ // per ogni elemento da rimuovere
		assert(specie[0]); // la specie ha almeno un elemento (ne sta per rimuovere uno)
		uint32_t least_fit = 1;
		for (uint32_t i = 1; i <= specie[0]; i++){ // per ogni elemento della specie
			assert(i <= specie[0]);
			assert(least_fit <= specie[0]);
			assert(specie[least_fit] >= 0 && specie[least_fit] < 10000);
			if (brains[specie[i]].fitness < brains[specie[least_fit]].fitness){
				least_fit = i;
			}
		}
		assert(brains[specie[least_fit]].fitness <= brains[specie[specie[0]]].fitness);
			//printf("penso che il least fit sia l'elemento %d della specie, questo ha fitness %f mentre l'ultimo elemento della specie (%d) ha fitness %f\n", least_fit, brains[specie[least_fit]].fitness, specie[0], brains[specie[specie[0]]].fitness);

		// scambio lultimo elemento della specie con il less fit
		uint16_t t = specie[specie[0]];
		specie[specie[0]] = specie[least_fit];
		specie[least_fit] = t;
		// -1 al numero di elementi dell specie (cosi il least fit che sta alla fine viene escluso)
		specie[0]--;
	}
}

void reproduction(pool_s *pool) // TODO seg faulta
{
	//printf("inizio riproduzione\n");
	brain_s *new_brains = calloc(sizeof(brain_s), pool->size);
	float *avg = calloc(sizeof(float), n_species(pool));
	float sum_avg = 0;
	//print_species(pool);
	uint16_t j;
	for (j = 0; j < n_species(pool); j++){
		if (pool->species(j, 0) == 0) break; // la specie j è vuota
		remove_k_least_fit(pool->brains, &pool->species(j,0), pool->species(j, 0) * remove_bottom_perc);
		for (uint16_t i = 0; i < pool->species(j, 0); i++){
			avg[j] += pool->brains[pool->species(j, i + 1)].fitness;
		}
		assert(pool->species(j, 0)); // la specie j ha almeno  1 elemento
		avg[j] /= pool->species(j, 0);
		sum_avg += avg[j];
		//printf("la specie %d ha %d elementi e avj %f\n", j, pool->species(j, 0), avg[j]);
	}
	uint16_t non_empty_species = j;
	printf("ci sono %d specie\n", non_empty_species);

	uint16_t i = 0;
	uint16_t deficit = pool->size; //number brains to add
	//printf("devo aggiugere %d braini\n", deficit);
	for (uint16_t j = 0; j < non_empty_species; j++){
		//printf("    la specie %d ha avj fitness %f\n", j, avg[j]);
		assert(sum_avg); // TODO non dovrebbe essere un assert, dovrebbe handlare il caso
		uint16_t offspring = pool->size / sum_avg * avg[j];
		//printf("    per la specie %d ne aggiungo %d\n", j, offspring);
		if (pool->species(j, 0) == 0) printf("la specie %d è vuota\n", j);
		for (uint16_t o = 0; o < offspring; o++){
			//printf("         inizio a generare l'offspring %d\n", o);
			int father = 1 + pcg32_random_r(&rng) % pool->species(j, 0);
			//printf("		trovato il papa\n");
			if (o == 1){
				//printf("random brain dalla specie %d:\n", j);
				//print_brain(&pool->brains[pool->species(j, father)]);
			}
			int mother = 1 + pcg32_random_r(&rng) % pool->species(j, 0);
			//printf("		trovata la mamma\n");
			brain_s son = brain_crossover(&pool->brains[pool->species(j, father)], &pool->brains[pool->species(j, mother)]);
			//printf("		fatto il crossover\n");
			brain_mutate(&son);
			//printf("		fatta la mutazione\n");
			assert(i < pool->size);
			new_brains[i++] = son;
			deficit--;
			//printf("         fine generazione offspring\n");


		}
		//printf("    specie %d fatta\n", j);
	}

	//printf("finiti per specie\n");

	// assert(deficit <= n_species(pool)); // perche??
	for (uint16_t o = 0; o < deficit; o++) {
		uint16_t father = pcg32_random_r(&rng) % pool->size;
		uint16_t mother = pcg32_random_r(&rng) % pool->size;
		brain_s son = brain_crossover(&pool->brains[father], &pool->brains[mother]);
		brain_mutate(&son);
		new_brains[i++] = son;
	}

	//printf("finiti i deficit\n");



	//assert(i == pool->size);

	for (size_t i = 0; i < pool->size; i++) {
		brain_free(&pool->brains[i]);
	}
	free(pool->brains);
	free(avg);
	pool->brains = new_brains;
	//printf("fine riproduzione\n");
}

void print_pool(const pool_s *pool)
{
	print_species(pool);
}
#undef species


float evolve(pool_s *pool, float (*test)(const brain_s*))
{
	// printf("%s\n", "-----------inizio evoluzione-------------");  // debug
	// printf("%s\n", "---------calcolo fitness"); // debug
	float avj = evaluate_fitness(pool, test);
	// printf("%s\n", "---------specio"); // debug
	speciation(pool);
	// printf("%s\n", "---------riproduzione"); // debug
	reproduction(pool);
	// printf("%s\n", "--------------fime evoluzione---------------"); // debug
	return avj;
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
