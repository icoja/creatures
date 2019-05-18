#include <unistd.h>
#include <math.h>
#include "brain.h"
#include "visual.h"
#include "pool.h"
#include "brain_mutations.h"
#include "random_pcg.h"

pcg32_random_t rng= {0, 0};

static inline float test(const brain_s *b){
	float input[2] = {1, 1};
	float output;
	float fitness = 4;

	brain_propagate(b, input, &output);

	for (size_t i = 0; i < 4; i++) {
		input[0] = i % 2;
		input[1] = i / 2;
		brain_propagate(b, input, &output);
		fitness -= fabs(output - !!((int)input[0] ^ (int)input[1]));
	}
	return fitness;
}

int main()
{
	brain_warmup();
	brain_mutations_warmup();

	sfContextSettings settings = {0};
	settings.antialiasingLevel = 8;
	sfRenderWindow *window = sfRenderWindow_create((sfVideoMode){900, 600, 32}, "creatures", sfResize | sfClose, &settings);



	pool_s pool;
	pool_init(&pool, 100);
	for (int i = 0; i < 100; i++){
		brain_s b;
		brain_init(&b, 2, 1);
		pool.brains[i] = b;
	}

	int i = 0;
	int gen_numb = 100;
	while (sfRenderWindow_isOpen(window))
	{
		sfEvent event;
		while (sfRenderWindow_pollEvent(window, &event))
		{
			if (event.type == sfEvtClosed)
			sfRenderWindow_close(window);
		}

		sfRenderWindow_clear(window, sfBlack);

		if (i < gen_numb){
			evolve(&pool, test);
			printf("gen numb: %d, fitenss: %f\n", i, test(&pool.brains[0]));
			sfRenderWindow_clear(window, sfBlack);
			brain_display(pool.brains + 0, window, 0, NULL);
			sfRenderWindow_display(window);
			// sleep(1);
		} else if (i == gen_numb) {
			for (int j = 0; j < 100; j++){
				printf("fitness: %f ", test(pool.brains + j));
				printf("number of links: %zu \n", (pool.brains[j]).links.size);
			}
			print_links(&pool.brains[0]);
			printf("done\n");
		} else {
			return 0;
		}
		i++;
	}

	pool_free(&pool);

	sfRenderWindow_destroy(window);
	return 0;
}

int gmain()
{


	brain_warmup();
	brain_mutations_warmup();

	/*for (size_t i = 0; i < 100; i++) {
		(pcg32_random_r(&rng) < UINT32_MAX / 2);
	}*/

	sfContextSettings settings = {0};
	settings.antialiasingLevel = 8;
	sfRenderWindow *window = sfRenderWindow_create((sfVideoMode){900, 600, 32}, "creatures", sfResize | sfClose, &settings);

	printf("bellissimi\n");
	brain_s b;
	brain_init(&b, 3, 2);
	b.fitness = 10;
	brain_add_link(&b, 0, 20, 0.88);
	brain_add_link(&b, 1, 20, 0.18);
	//brain_split_link(&b, 0);
	brain_add_link(&b, 2, 21, -1.5);
	//brain_split_link(&b, 1);
	//brain_add_link(&b, 41, 40, -0.1);

	print_links(&b);
	brain_s c;
	brain_init(&c, 3, 2);
	c.fitness = 18;
	brain_add_link(&c, 0, 20, 0.11);
	brain_add_link(&c, 1, 21, -0.9);

	//brain_display(&c, window, 0, NULL);
	printf("\n");
	print_links(&c);

	brain_crossover(&b, &c);

	for (size_t j = 0; j < 2000; j++) {
		brain_s b;
		brain_init(&b, 3, 2);
		for (size_t i = 0; i < 180; i++) {
			brain_mutate(&b);
		}
		//print_links(&b);
		sfRenderWindow_clear(window, sfBlack);
		brain_display(&b, window, 0, NULL);
		sfRenderWindow_display(window);
		sleep(1);
		brain_free(&b);
	}
	sleep(200);
	return 0;
	int i = 0;
	while (sfRenderWindow_isOpen(window))
	{
		sfEvent event;
		while (sfRenderWindow_pollEvent(window, &event))
		{
			if (event.type == sfEvtClosed)
			sfRenderWindow_close(window);
		}

		sfRenderWindow_clear(window, sfBlack);
		//b.propagate(in, out, 1, &window);
		brain_mutate(&b);
		brain_display(&b, window, 0, NULL);
		sfRenderWindow_display(window);
	}



	sfRenderWindow_destroy(window);
	return 0;
}
