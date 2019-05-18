#pragma once

#include <SFML/Graphics.h>
#include "brain.h"

typedef struct vis_neuron vis_neuron_s;

void brain_display(brain_s *b, sfRenderWindow *window, int use_values, float *n_values);

//void set_layer(brain_s *b, vis_neuron_s *neurons, uint32_t neurons_number, float max_size_x);
