#include <time.h>
#include "brain_mutations.h"
#include "random_pcg.h"

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

void brain_mutations_warmup()
{
	time((int64_t*)&rng.state);
	time((int64_t*)&rng.inc);
}

void brain_mutate (brain_s *b)
{
	assert(check_brain(b));

	// non cambiare l'ordine di esecuzione delle mutazioni (anche se comunque non è troppo grave)
	float remove_neuron_prob = 0; // n * 4o
	float split_link_prob = 0.001;
	float remove_link_prob = 0.0; // mmh meglio non usare
	float add_link_prob = 0.01;
	float weight_mut_prob = 0.1;

	float weight_mut_range = 1;

	// init
	float which_mutation_rand = (float)pcg32_random_r(&rng) / UINT32_MAX; // rng definita a inizio file
	float interval_start = 0, interval_end = 0;
	// remove neuron mutation
	interval_end += remove_neuron_prob;
	if (which_mutation_rand > interval_start && which_mutation_rand < interval_end){
	}
	interval_start += remove_neuron_prob;

	// add neuron (split link) mutation
	interval_end += split_link_prob;
	if (which_mutation_rand > interval_start && which_mutation_rand < interval_end && b->links.size != 0){

		int link_index_rand = pcg32_random_r(&rng) % b->links.size;
		brain_split_link(b, b->links.data[link_index_rand].innov_number);
	}
	interval_start += split_link_prob;

	// remove link mutation
	interval_end += remove_link_prob;
	if (which_mutation_rand > interval_start && which_mutation_rand < interval_end && b->links.size != 0){

		int link_index_rand = pcg32_random_r(&rng) % b->links.size;
		b->links.data[link_index_rand].disabled = true;
	}
	interval_start += remove_link_prob;

	// add link mutation
	assert(check_brain(b));
	interval_end += add_link_prob;
	if (which_mutation_rand > interval_start && which_mutation_rand < interval_end){
		uint32_t src_index = pcg32_random_r(&rng) % (b->dict.elements - b->output_size); // circa uniformemente distribuita tra 0 e #neuroni possibili (ovvviamente il src non puo indicare un neurone output)
		uint32_t dst_index = pcg32_random_r(&rng) % (b->dict.elements - b->input_size); // circa uguale a su

		int src_key = -1, dst_key = -1;
		for(size_t i = 0; i < b->dict.size; i++){
			bucket *u = b->dict.table + i;
			while(u){
				for(size_t j = 0; j < BUCKET_SIZE; j++){
					if(u->entries[j].used){
						//printf("entry = %d, valid = %d, dst = %d\n", u->entries[j].key, u->entries[j].key > MAX_INPUT_NEURONS, dst_index);
						if ((u->entries[j].key < MAX_INPUT_NEURONS || u->entries[j].key >= MAX_INPUT_NEURONS + MAX_OUTPUT_NEURONS) && src_index-- == 0)
						src_key = u->entries[j].key;
						if (u->entries[j].key >= MAX_INPUT_NEURONS && dst_index-- == 0)
						dst_key = u->entries[j].key;
					}
				}
				u = u->next;
			}
		}

		assert(src_key + 1);
		assert(dst_key + 1);
		//printf("adding random link btween: %d and %d\n", src_key, dst_key);
		brain_add_link(b, src_key, dst_key, 9.99); // weight a 0 per minimizzare il disturbo

		assert(check_brain(b));
		//brain_add_link(b, src_index, dst_index, 0);
	}
	interval_start += add_link_prob;


	// weight mutation
	interval_end += weight_mut_prob;
	if (which_mutation_rand > interval_start && which_mutation_rand < interval_end && b->links.size != 0){

		int link_index_rand = pcg32_random_r(&rng) % b->links.size;
		float rand_increment = ((float) pcg32_random_r(&rng) / UINT32_MAX - 0.5) * weight_mut_range;
		b->links.data[link_index_rand].weight += rand_increment;
	}
	interval_start += weight_mut_prob;

	assert(check_brain(b));
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


brain_s brain_crossover(const brain_s *mother, const brain_s *father)
{
	assert(check_brain(mother));
	assert(check_brain(father));

	const brain_s *fittest = (mother->fitness > father->fitness) ? mother : father;
	const brain_s *lessfit = (mother->fitness <= father->fitness) ? mother : father;

	vector_link_s fittest_links = vector_link_s_copy(&(fittest->links)); // con o senza * ??
	vector_link_s lessfit_links = vector_link_s_copy(&(lessfit->links));

	quick_sort_links(&fittest_links);
	quick_sort_links(&lessfit_links);

	// crea new braino
	brain_s new_brain;
	brain_init(&new_brain, fittest->input_size, fittest->output_size);

	size_t j = 0; // indice del less fit
	for (size_t i = 0; i < fittest_links.size; i++){ // i indice del fittest
		link_s current_fittest = fittest_links.data[i];
		link_s current_lessfit = lessfit_links.data[j];

		// (1) i link che ha il lessfit e il fittest no vengono ignorati
		while (current_lessfit.innov_number < current_fittest.innov_number && j < lessfit_links.size){
			j++;
			current_lessfit = lessfit_links.data[j];
		}

		// (2) quelli che tutti e due hanno vengono scelti a caso
		if (current_lessfit.innov_number == current_fittest.innov_number){
			// basta  scegliere solo il weight perche stesso inn implica stesso src e dst
			//printf("fitt: %f, less: %f\n", current_fittest.weight, current_lessfit.weight);
			float new_weight = (pcg32_random_r(&rng) < UINT32_MAX / 2) ? current_fittest.weight : current_lessfit.weight;
			// probabilita che il link sara disabled, espressa su tutti gli interni e non 0-1. 0 se nessuno dei parent ha quel link disabled, intMAX se tutti e due, intMAX/2 se uno solo
			// TODO check that unsigned int is the proper typing (later this value is compared with an uint32_t)
			unsigned int disabled_prob = (current_lessfit.disabled * UINT32_MAX / 2 +
				current_fittest.disabled * UINT32_MAX / 2);
				bool shoud_be_disabled = pcg32_random_r(&rng) < disabled_prob;
				brain_add_link_full(&new_brain, current_fittest.src, current_fittest.dst, new_weight, shoud_be_disabled); // perche & ?? (anche nella linea del else) // un po inefficente forse visto che il link gia esiste fatto


				// (3) quelli che ha solo il fittest vengono preservati
			} else{
				brain_add_link(&new_brain, current_fittest.src, current_fittest.dst, current_fittest.weight); // un po inefficente forse visto che il link gia esiste fatto
			}
		}

		//print_links(&new_brain);

		// free vectors
		vector_link_s_free(&fittest_links);
		vector_link_s_free(&lessfit_links);


		if (!check_brain(&new_brain)){
			print_brain(&new_brain);
			assert(0);
		}

		return new_brain;
	}
