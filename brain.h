#pragma once
#include <stdio.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>
#include "vector.h"
#include "hash_table.h"
#include "hash_table_64.h"

#define MAX_INPUT_NEURONS 20
#define MAX_OUTPUT_NEURONS 20

struct link{

	uint32_t src; //innov_number
	uint32_t dst; //innov_number
	uint32_t src_id;

	uint32_t dst_id;
	uint32_t innov_number;
	float weight;
	bool disabled;

};
typedef struct link link_s;

DEFINE_VECTOR_TYPE(link_s)

extern _Atomic uint32_t brain_innov_number;

extern _Atomic uint32_t brain_neuron_number;



struct brain{
	uint32_t input_size, output_size; // dovrebbe essere const?

	//std::map<uint32_t, uint32_t> dict;
	float fitness;
	hash_table dict;
	vector_link_s links;
};
typedef struct brain brain_s;


enum brain_disorders {BRAIN_OK,
	 	      INVALID_INPUT_SIZE,
	      	      INVALID_OUTPUT_SIZE,
		      INVALID_LINKS_NUMBER,
		      SRC_TOO_BIG,
		      DST_TOO_BIG,
		      WEIGHT_TOO_BIG,
		      INNOV_NUMBER_TOO_BIG,
		      DST_ID_OUT_OF_BOUND,
		      SRC_ID_OUT_OF_BOUND
	      };


void brain_warmup();
void brain_free(brain_s *b);
void brain_init (brain_s *b, uint32_t in_count, uint32_t out_count);
void brain_propagate (const brain_s *b, float *input, float *output);
void brain_propagate_vis (const brain_s *b, float *input, float *output, float *acc);
void brain_propagate_vis_old (const brain_s *b, float *input, float *output, float *acc);
void brain_mutate ();
void brain_split_link (brain_s *b, uint32_t l);
bool brain_add_link_full (brain_s *b, uint32_t src, uint32_t dst, float weight, bool disabled);
void print_brain (const brain_s *b);
enum brain_disorders check_brain (const brain_s *b);

static inline bool brain_add_link (brain_s *b, uint32_t src, uint32_t dst, float weight)
{
	return brain_add_link_full(b, src, dst, weight, false);
}

static inline void print_link(const link_s *l)
{

	printf("link - inn: %d, src: %d, dst: %d, src_id: %d, dst_id: %d, w: %f, %s\n",
	l->innov_number, l->src, l->dst, l->src_id, l->dst_id, l->weight, l->disabled ? "disabled" : "enabled");

}

static inline void print_links(const brain_s *b)
{
	for (size_t i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		print_link(&l);
	}
}
