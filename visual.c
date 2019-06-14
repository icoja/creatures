#include <math.h>
#include "visual.h"

#define neurons_spacing 54
#define neuron_radius 14

static inline int int_max(const int a, const int b)
{
	return (a > b) ? a : b;
}

static inline int int_min(const int a, const int b)
{
	return (a < b) ? a : b;
}

static inline float sigmoid(float x)
{
	return x/(1 + fabs(x));
}

struct vis_neuron{
	uint32_t inn;
	int layer;
	float x;
	float y;
	float value;
};


void init_neurons(brain_s *b, vis_neuron_s *neurons, uint32_t neurons_number)
{
	// checkino
	assert(neurons_number >= b->input_size + b->output_size);


	for(size_t i = 0; i < b->dict.size; i++){
		bucket *u = b->dict.table + i;
		while(u){
			for(size_t j = 0; j < BUCKET_SIZE; j++){
				if(u->entries[j].used){
					vis_neuron_s n;
					n.inn = u->entries[j].key;
					n.layer = -1;
					neurons[u->entries[j].value] = n;
				}
			}
			u = u->next;
		}
	}

	//check
	for (size_t i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		if (l.disabled) continue;
		assert(neurons[l.dst_id].inn == l.dst);
		assert(neurons[l.src_id].inn == l.src);
	}
}

void set_x(brain_s *b, vis_neuron_s *neurons, uint32_t neurons_number, float max_size_x)
{
	//controlla stato iniziale
	for (size_t i = 0; i < neurons_number; i++){
		assert(neurons[i].layer == -1);
	}

	// assegna 0 agli input
	for (size_t i = 0; i < neurons_number; i++){
		if (neurons[i].inn >= MAX_INPUT_NEURONS) continue; // is input
		neurons[i].layer = 0;
	}

	// calcola layer per quelli che sono dest di qualche link
	for (size_t i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		if (l.disabled) continue;
		if (neurons[l.src_id].layer == -1){ // il src non è src di nessuno
			neurons[l.src_id].layer = 1;
		}
		neurons[l.dst_id].layer = int_max(neurons[l.dst_id].layer, neurons[l.src_id].layer + 1);
	}

	// .. per quelli che non sono dest di nessuno (non dovrebbero esserci?)
	for (size_t i = 0; i < neurons_number; i++){
		if (neurons[i].inn < MAX_INPUT_NEURONS || neurons[i].layer != -1) continue; // se è input oppure se è gia stato settato
		// cerca tutti i link che partono da lui
		bool found = false;
		for (size_t j = 0; j < b->links.size; j++){
			const link_s l = b->links.data[j];
			if (l.disabled) continue;
			if (l.src == neurons[i].inn){
				int child_layer = neurons[l.dst_id].layer;
				if (found){ // non è il primo link trovato (quindi non c'è nemmeno bisogni di mettere found true)
					neurons[i].layer = int_min(child_layer - 1, neurons[i].layer);
				} else {
					neurons[i].layer = child_layer - 1;
					found = true;
				}
			}
		}
		// altrimenti assegna valore arbitrario
		if (!found){
			printf("non ho trovato un src per %d\n", neurons[i].inn);
			neurons[i].layer = 1;
		}
		neurons[i].layer = int_max(1,neurons[i].layer);
	}

	// allinea gli output dopo tutti
	//	trova il massimo
	int max_layer = 0;
	for (size_t i = 0; i < neurons_number; i++){
		max_layer = int_max(max_layer, neurons[i].layer + 1);
	}
	assert(max_layer > 0 && max_layer < 1000);

	// 	assegna il valora a gli output
	for (size_t i = 0; i < neurons_number; i++){
		if (neurons[i].inn < MAX_INPUT_NEURONS || neurons[i].inn >= MAX_INPUT_NEURONS + MAX_OUTPUT_NEURONS) continue; // is output
		neurons[i].layer = max_layer;
	}

	// check
	for (size_t i = 0; i < neurons_number; i++){
		if (neurons[i].inn >= MAX_INPUT_NEURONS) continue; // is input
		assert(neurons[i].layer == 0);
	}
	for (size_t i = 0; i < neurons_number; i++){
		if (neurons[i].layer == -1){
			printf("il neurone %zu non è stato assegnato un layer\n", i);
			neurons[i].layer = max_layer-1;
		}
		// assert(neurons[i].layer != -1); potrebbero essere tutti disabled
	}

	// scala il valore per la dimensione della cornice
	float layer_length = fmin(neurons_spacing, max_size_x / max_layer); // distanza in pixel tra un layer e laltro
	for (size_t i = 0; i < neurons_number; i++){
		neurons[i].x = neurons[i].layer * layer_length;
		assert(neurons[i].x >= 0 && neurons[i].x <= max_size_x);
	}

	// check
	for (size_t i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		if (l.disabled) continue;
		if (neurons[l.dst_id].x <= neurons[l.src_id].x){
			printf("il neurone %d e in %d hanno layer risp. %d e %d, ma il link %d li collega ->\n", neurons[l.dst_id].inn, neurons[l.src_id].inn, neurons[l.dst_id].layer, neurons[l.src_id].layer, l.innov_number);
			//assert(0);
		}
	}
}

float equal_space(uint32_t items, uint32_t which, float dist, float center)
{
	// calcola la posizione di "items" oggetti che si trovano allineati e equamente distanziati (dist) intorno a un centro
	// which indica di quale oggetto della riga si deve calcolare la posizione (per esempio se item è 6 wich deve essere un intero tra 0 e 5)
	assert(which < items);
	float offset = (items - 1) * dist / 2;
	return which * dist - offset + center;
}

void set_y(vis_neuron_s *neurons, uint32_t neurons_number, float max_size_y)
{
	// trova con quanti ognuno condivide il layer
	int shared_by[1000] = {0};
	int max_layer_size = 0;
	for (size_t i = 0; i < neurons_number; i++){
		assert(neurons[i].layer >= 0 && neurons[i].layer < 1000);
		shared_by[neurons[i].layer]++;
		max_layer_size = (shared_by[neurons[i].layer] > max_layer_size) ? shared_by[neurons[i].layer] : max_layer_size;
	}
	// left to displace indica quanti oggetti per ogni layer non sono stati ancora assegnati il valore di displace
	int left_to_displace[1000];
	// inizialmente sono tutti, quindi uguale a quanti oggetti per layer - 1 (se ci sono 3 oggetti il primo indice è 2)
	for (uint32_t i = 0; i < 1000; i++){
		left_to_displace[i] = shared_by[i] - 1;
		//if (i < 10) printf("layer %d shared by %d neurons\n", i, shared_by[i]);
	}
	// aggiusta i parametri
	float spacing = fmin(neurons_spacing, max_size_y / max_layer_size);
	float center = max_layer_size * spacing / 2;
	for (size_t i = 0; i < neurons_number; i++){
		const int l = neurons[i].layer;
		assert(l >= 0 && l < 1000);
		//printf("setto il neurone %d nel layer %d (che è condiviso da %d), posizione %d\n", neurons[i].inn, neurons[i].layer, shared_by[l], left_to_displace[l]);
		assert(left_to_displace[l] >= 0);
		neurons[i].y = equal_space(shared_by[l], left_to_displace[l]--, spacing, center);
	}

	// check
	for (size_t i = 0; i < neurons_number; i++){
		assert(neurons[i].y >= 0 && neurons[i].y <= max_size_y);
	}

}

void x_y_check(vis_neuron_s *neurons, float max_size_x, float max_size_y, uint32_t neurons_number)
{
	for (size_t i = 0; i < neurons_number - 1; i++){
		assert(neurons[i].y >= 0 && neurons[i].y <= max_size_y);
		assert(neurons[i].x >= 0 && neurons[i].x <= max_size_x);
		for (size_t j = i + 1; j < neurons_number; j++){
			assert(neurons[i].y != neurons[j].y || neurons[i].x != neurons[j].x);
		}
	}
}

void dyn_adjust(brain_s *b, vis_neuron_s *neurons, uint32_t neurons_number)
{
	uint32_t iterations = 100;
	float increment = 0.02;

	for (uint32_t iter = 0; iter < iterations; iter++){
		for (size_t i = 0; i < neurons_number; i++){
			for (size_t j = 0; j < neurons_number; j++){
				float dx = neurons[i].x - neurons[j].x;
				float dy = neurons[i].y - neurons[j].y;
				float ix = increment * sigmoid(dx/10);
				float iy = increment * sigmoid(dy/10);
				if ((dx*dx + dy*dy) < neurons_spacing * neurons_spacing * 5){ // link too small
					ix *= -1;
					iy *= -1;
				}
				neurons[j].x += ix;
				neurons[j].y += iy;
				neurons[i].x -= ix;
				neurons[i].y -= iy;

				dx = neurons[i].x - neurons[j].x;
				dy = neurons[i].y - neurons[j].y;
			}
		}
		for (size_t i = 0; i < b->links.size; i++){
			const link_s l = b->links.data[i];
			if (l.disabled) continue;
			float dx = neurons[l.dst_id].x - neurons[l.src_id].x;
			float dy = neurons[l.dst_id].y - neurons[l.src_id].y;
			float ix = increment * sigmoid(dx/10);
			float iy = increment * sigmoid(dy/10);
			if ((dx*dx + dy*dy) < neurons_spacing * neurons_spacing * 8){ // link too small
				ix *= -1;
				iy *= -1;
			}
			neurons[l.src_id].x += ix;
			neurons[l.src_id].y += iy;
			neurons[l.dst_id].x -= ix;
			neurons[l.dst_id].y -= iy;

			dx = neurons[l.dst_id].x - neurons[l.src_id].x;
			dy = neurons[l.dst_id].y - neurons[l.src_id].y;
		}
	}
}

void draw_neurons(brain_s *b, vis_neuron_s *neurons, uint32_t neurons_number, sfRenderWindow *window, bool display_weights, bool use_values, float *values)
{
	sfFont *arial = sfFont_createFromFile("arial.ttf");
	sfText *text = sfText_create();
	sfText_setFont(text, arial);
	char str[32];
	sfConvexShape *convex = sfConvexShape_create();
	sfConvexShape_setPointCount(convex, 4);
	for (size_t i = 0; i < b->links.size; i++){
		const link_s l = b->links.data[i];
		if (l.disabled) continue;
		sfColor color = {(int) 255 - 127 * (1 + sigmoid(l.weight)), (int) 127 * (1 + sigmoid(l.weight)), 0, 125};
		sfConvexShape_setPoint(convex, 0, (sfVector2f){neurons[l.src_id].x + neuron_radius, neurons[l.src_id].y + neuron_radius});
		sfConvexShape_setPoint(convex, 1, (sfVector2f){neurons[l.dst_id].x + neuron_radius, neurons[l.dst_id].y + neuron_radius});
		sfConvexShape_setPoint(convex, 2, (sfVector2f){neurons[l.dst_id].x + neuron_radius, neurons[l.dst_id].y + neuron_radius+2});
		sfConvexShape_setPoint(convex, 3, (sfVector2f){neurons[l.src_id].x + neuron_radius, neurons[l.src_id].y + neuron_radius+2});
		sfConvexShape_setFillColor(convex, color);

		sfRenderWindow_drawConvexShape(window, convex, NULL);

		if (display_weights){
			snprintf(str, 32, "%2.f", l.weight);
			//gcvt(l.weight, 2, str);
			sfText_setString(text, str);
			sfText_setCharacterSize(text, 10); // in pixels, not points!
			sfText_setFillColor(text, sfWhite);
			sfText_setPosition(text, (sfVector2f){(neurons[l.src_id].x + neurons[l.dst_id].x) / 2 + neuron_radius, (neurons[l.src_id].y + neurons[l.dst_id].y) / 2 + neuron_radius});
			sfRenderWindow_drawText(window, text, NULL);
		}

	}
	sfConvexShape_destroy(convex);
	// draw neurons
	for (uint32_t i = 0; i < neurons_number; i++){
		//std::cout << "neuron " << i << " - " << (float) (layer[i]*size_x) / (float) max_layer << ", " << displace[i] << '\n';
		sfCircleShape *neuron = sfCircleShape_create();
		sfCircleShape_setRadius(neuron, neuron_radius);
		sfCircleShape_setFillColor(neuron, (sfColor){200,200,200, 255});
		sfCircleShape_setOutlineThickness(neuron, 0);
		sfCircleShape_setOutlineColor(neuron, (sfColor){200,200,200, 255});
		sfCircleShape_setPosition(neuron, (sfVector2f){neurons[i].x, neurons[i].y});
		sfRenderWindow_drawCircleShape(window, neuron, NULL);
		snprintf(str, 32, "%d", neurons[i].inn);
		sfText_setString(text, str);
		sfText_setCharacterSize(text, 10); // in pixels, not points!
		sfText_setFillColor(text, sfBlack);
		sfText_setPosition(text, (sfVector2f){neurons[i].x+2, neurons[i].y+2});
		sfRenderWindow_drawText(window, text, NULL);
		if (use_values){
			char str_text[10];
			//gcvt(values[i], 2, str_text);
			snprintf(str_text, 32, "%2.f", values[i]);
			sfText_setString(text, str_text);
			sfText_setCharacterSize(text, 12); // in pixels, not points!
			sfText_setFillColor(text, sfBlack);
			sfText_setPosition(text, (sfVector2f){neurons[i].x, neurons[i].y+neuron_radius-6});
			sfRenderWindow_drawText(window, text, NULL);
		}

	}

}

void brain_display(brain_s *b, sfRenderWindow *window, int use_values, float *values)
{
	int max_size_x = 600, max_size_y = 400;
	int display_weights = 0;
	uint32_t neurons_number = b->dict.elements;
	vis_neuron_s *neurons = calloc(sizeof(vis_neuron_s), neurons_number);

	init_neurons(b, neurons, neurons_number);
	set_x(b, neurons, neurons_number, max_size_x);
	set_y(neurons, neurons_number, max_size_y);
	x_y_check(neurons, max_size_x, max_size_y, neurons_number);
	dyn_adjust(b, neurons, neurons_number);
	draw_neurons(b, neurons, neurons_number, window, display_weights, use_values, values);

	free(neurons);
}
