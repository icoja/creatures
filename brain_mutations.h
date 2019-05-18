#pragma once
#include "brain.h"

void brain_mutate (brain_s *b);
void brain_mutations_warmup();
brain_s brain_crossover(const brain_s *mother, const brain_s *father);
