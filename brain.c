#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "brain.h"

_Atomic uint32_t brain_innov_number = 0;
_Atomic uint32_t brain_neuron_number = MAX_INPUT_NEURONS + MAX_OUTPUT_NEURONS;

static hash_table64 links_hash;

void brain_warmup()
{
	hash_table64_init(&links_hash); // TODO rendi parte di pool
}

/*
INFORMAZIONI UTILI PER IL FUTURO
1	INNOV NUMBER è numero unico per ogni mutazione genetica di aggiunta link, ogni
	classe di link (links con stesso src e dst) hanno stesso innov_number.
	viene usato per allineare i geni (link) simili durante il crossover e per
	calcoare la distanza tra due braini (speciation).
2	ogni NEURONE inteso come particolare neurone di partenza o arrivo da un
	link con un certo innov_number ha un nome unico che pero è salvato solo
	nei link a lui collegati (è il membro src o dst). inoltre se questo numero
	indica anche il tipo di neurone (input output interno) (vedi MAX_INPUT_NEURONS, ..)
3	il DIZIONARIO nel cervello serve a tradurre i nomi dei neuroni negli
	indici della lista di accumulatori (che non esiste ma viene creata
	ogni volta da brain_propagate).

*/

void print_brain (const brain_s *b)
{
	printf("BRAIN:\n");
	printf("input size: %d, output size: %d\n", b->input_size, b->output_size);
	printf("fitness: %f\n", b->fitness);
	printf("hash entries: ");
	for(size_t i = 0; i < b->dict.elements; i++){
		bucket *u = b->dict.table + i;
		while(u){
			for(size_t j = 0; j < BUCKET_SIZE; j++){
				if(u->entries[j].used){
					printf("(%d, %d) ", u->entries[j].key, u->entries[j].value);
				}
			}
			u = u->next;
		}
	}
	printf("\nlinks:\n");
	print_links(b);

}

#define in_range(x, a, b) (x >= a && x <= b)

enum brain_disorders check_brain (const brain_s *b) // seg faulta
{
	if (b->input_size > 100) return INVALID_INPUT_SIZE;
	if (b->input_size > 100) return INVALID_OUTPUT_SIZE;
	//assert(b->fitness >= 0);
	// controllo che il dict tenga i neuroni tutti vicini
	//uint32_t value_should_be = 0;
	uint32_t entries_number = b->dict.elements;
	/*  qui seg faulta TODO
	printf("inizializato check\n");
	printf("dict size: %d\n", b->dict.elements);
	for(size_t i = 0; i < b->dict.elements; i++){
		bucket *u = b->dict.table + i;
		while(u){
			for(size_t j = 0; j < BUCKET_SIZE; j++){
				if(u->entries[j].used){
					entries_number++;
					// toppato perche non è detto che sono in ordine//ok *= (u->entries[j].value == value_should_be++);
				}
			}
			u = u->next;
		}
	}
	printf("checkato dizionario\n");
	*/

	if (b->links.size > 1000) return INVALID_LINKS_NUMBER;
	for (size_t i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		if (l.src > 1000000) return SRC_TOO_BIG;
		if (l.dst > 1000000) return DST_TOO_BIG;
		if (l.dst_id > entries_number) return DST_ID_OUT_OF_BOUND;
		if (l.src_id > entries_number) return SRC_ID_OUT_OF_BOUND;
		if (l.innov_number > 1000000) return INNOV_NUMBER_TOO_BIG;
		if (!in_range(l.weight, -100, 100)) return WEIGHT_TOO_BIG;
	}
	return BRAIN_OK;
}
#undef in_range

void brain_init (brain_s *b, uint32_t in_count, uint32_t out_count)
{
	b->input_size = in_count;
	b->output_size = out_count;
	b->fitness = 0;
	hash_table_init(&b->dict);
	vector_link_s_construct(&b->links);

	assert(in_count < MAX_INPUT_NEURONS);
	assert(out_count < MAX_OUTPUT_NEURONS);

	for (uint32_t i = 0; i < in_count; i++){
		hash_table_insert_new(&b->dict, i, i);
	}
	for (uint32_t i = 0; i < out_count; i++){
		hash_table_insert_new(&b->dict, MAX_INPUT_NEURONS + i, b->input_size + i);
	}
}

void brain_free(brain_s *b)
{
	hash_table_free(&b->dict);
	vector_link_s_free(&b->links);
}

static inline float sigmoid(float x)
{
	return x/(1 + fabs(x));
}

void brain_propagate (const brain_s *b, float *input, float *output)
{
	float *neurons = calloc(sizeof(float), b->dict.elements);
	bool *cached = calloc(sizeof(bool), b->dict.elements);

	assert(neurons);
	assert(cached);

	// carica l'input nei neuroni di input
	for (size_t i = 0; i < b->input_size; i++){
		//assert(b->links.data[i].src_id < b->input_size);
		neurons[i] = input[i];
	}

	// propaga (assumendo l'ordinamento dei links)
	uint32_t count = 0; // a che serve??
	// per ogni link
	for (size_t  i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		// ignora se diablato
		if (l.disabled) continue;
		// se il source non è mai stato usato, quindi non gli è stata ancora applicata la sigmoide
		if (!cached[l.src_id]){
			cached[l.src_id] = true;
			neurons[l.src_id] = sigmoid(neurons[l.src_id]);
		}

		neurons[l.dst_id] += l.weight * neurons[l.src_id];
		count++; // a che serve ??

	}
	// gli output non essendo source di nessun neurone sicuramente non sono stati sigmoidati + carica i valori finali nell'array "output":
	for (size_t i = 0; i < b->output_size; i++){
		// per convenzione i neuroni di output sono quelli subito dopo i neuroni di input: quindi da input_size a input_size + output_size
		float res = sigmoid(neurons[b->input_size + i]);
		assert(!isnan(res));
		output[i] = res; // diverso dalla versione c++ che non sigmoida l'output
	}

	free(neurons);
	free(cached);
}


void brain_propagate_vis (const brain_s *b, float *input, float *output, float *acc)
{
	// questa funzione deve essere sempre uguale a brain_propagate tranne per le righe evidenziate
	float *neurons = calloc(sizeof(float), b->dict.elements);
	bool *cached = calloc(sizeof(bool), b->dict.elements);

	assert(neurons);
	assert(cached);

	// carica l'input nei neuroni di input
	for (size_t i = 0; i < b->input_size; i++){
		//assert(b->links.data[i].src_id < b->input_size);
		neurons[i] = input[i];
	}

	// propaga (assumendo l'ordinamento dei links)
	uint32_t count = 0; // a che serve??
	// per ogni link
	for (size_t  i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		// ignora se diablato
		if (l.disabled) continue;
		// se il source non è mai stato usato, quindi non gli è stata ancora applicata la sigmoide
		if (!cached[l.src_id]){
			cached[l.src_id] = true;
			neurons[l.src_id] = sigmoid(neurons[l.src_id]);
		}

		neurons[l.dst_id] += l.weight * neurons[l.src_id];
		count++; // a che serve ??

	}
	// gli output non essendo source di nessun neurone sicuramente non sono stati sigmoidati + carica i valori finali nell'array "output":
	for (size_t i = 0; i < b->output_size; i++){
		// per convenzione i neuroni di output sono quelli subito dopo i neuroni di input: quindi da input_size a input_size + output_size
		float res = sigmoid(neurons[b->input_size + i]);
		assert(!isnan(res));
		output[i] = res; // diverso dalla versione c++ che non sigmoida l'output
	}
	////////////////////////// unica differenza con brain_propagate qui ////////////
	for (size_t i = 0; i < b->dict.elements; i++){
		acc[i] = neurons[i];
	}
	////////////////////////////////////////////////////////////////////////
	
	free(neurons);
	free(cached);
}

bool brain_add_link_full (brain_s *b, uint32_t src, uint32_t dst, float weight, bool disabled) // ogni (o qualche) volta che returna 0 il programma aborta
{
	if (check_brain(b) != BRAIN_OK){
		print_brain(b);
		assert(0);
	}
	size_t first_src = b->links.size; // prima volta che si legge da dst
	bool found = false;
	size_t last_dst = 0; // ultima volta che si scrive nel src
	if (dst < b->input_size){
		printf("input non validi: fallito add link\n");
		assert(0);
	}
	if (src > MAX_INPUT_NEURONS && src < MAX_INPUT_NEURONS + b->output_size){
		printf("input non validi: fallito add link\n");
		return 0;
	}

	if (src == dst){
		printf("src e dst uguali: fallito add link\n");
		return 0;
	}

	for (size_t i = 0; i < b->links.size; i++) {
		if (b->links.data[i].disabled)
			continue;
		if (b->links.data[i].dst == src)
			last_dst = i + 1;
		if (!found && b->links.data[i].src == dst){
			first_src = i;
			found = true;
		}
	}

	if(first_src < last_dst){
		printf("	non c'è posto dove inserire link\n");
		return 0;
	}

	for (size_t i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		if (l.src == src && l.dst == dst){
			printf("	stai provando a aggiungere un link che gia esiste\n");
			return 0;
		}
	}
	link_s l;

	l.src = src;

	l.dst = dst;

	l.disabled = disabled;

	if (!hash_table64_search(&links_hash, make_key64(src, dst), &l.innov_number)){
		l.innov_number = brain_innov_number++;
		hash_table64_insert_new(&links_hash, make_key64(src, dst), l.innov_number);
	}

	l.weight = weight;

	if(!hash_table_search(&(b->dict), src, NULL)) printf("searching %d not found\n", src);
	assert(hash_table_search(&(b->dict), src, NULL));

	if (!hash_table_search(&(b->dict), src, &l.src_id)){
		hash_table_insert_new(&(b->dict), src, (uint32_t)b->dict.elements);
	}
	//l.src_id = dict[src];
	int new_in_hash = 0;
	if (!hash_table_search(&(b->dict), dst, &l.dst_id)){
		l.dst_id = (uint32_t)b->dict.elements;
		hash_table_insert_new(&(b->dict), dst, l.dst_id);
	}
	//assert(hash_table_search(&(b->dict), dst, NULL));

	l.disabled = false;
	size_t new_link_index = (first_src + last_dst) / 2;
	//vector_link_s_insert(&b->links, links.begin() + new_link_index, l);

	if(l.dst_id > 1000){
		printf("new in hash? %d\n", new_in_hash);
		printf("dst_id = %d\n", l.dst_id);
		assert(0);
	}

	vector_link_s_insert(&b->links, new_link_index, l);

	if (check_brain(b) != BRAIN_OK){
		printf("brain sbagliato: errore %d\n", check_brain(b));
		print_brain(b);
		assert(0);
	}

	return 1;
}




void brain_split_link (brain_s *b, uint32_t lnk) // ale dice che è buggata forse
{
	for (size_t i = 0; i < b->links.size; i++){
		link_s *ln = &b->links.data[i]; //why was this const?
		if(ln->innov_number == lnk) {
			ln->disabled = true;
			link_s l = *ln;
			//why was this atomic?
			uint32_t middle = brain_neuron_number++; //fetch_add might need the std macro
			assert(b->dict.elements <= UINT32_MAX);
			hash_table_insert(&b->dict, middle, (uint32_t)b->dict.elements);
			brain_add_link(b, l.src, middle, l.weight);
			brain_add_link(b, middle, l.dst, 1);
			return;
		}
	}
	printf("innovation number not found in links vector");
	abort();
}
