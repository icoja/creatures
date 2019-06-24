#include <unistd.h>
#include <math.h>
#include <unistd.h>
#include "brain.h"
#include "visual.h"
#include "pool.h"
#include "brain_mutations.h"
#include "random_pcg.h"
#include "mike.h"

pcg32_random_t rng= {0, 0};

static inline float small_test(const brain_s *b){
	float input[2] = {1, 1};
	float output;
	float fitness = 4;

	/*
	for (size_t i = 0; i < 4; i++) {
		input[0] = i % 2;
		input[1] = i / 2;
		//printf("input: %f %f\n", input[0], input[1]);
		brain_propagate(b, input, &output);
		fitness -= fabs((1 + output)/2 - !!((int)input[0] ^ (int)input[1]));
		//printf("fitness after test with %f and %f: %f\n", input[0], input[1], fitness);

	}
	*/
	brain_propagate(b, input, &output);
	return 1.5 - fabs(output - 0.5);
}


float test(const brain_s *b){
	//printf("inizio test\n");
	float output[4];
	float input[12];


	mike_s mike;
	mike_init(&mike);
	cpSpace *space = cpSpaceNew();

	cpVect gravity = cpv(0, -100);
	cpSpaceSetGravity(space, gravity);
	cpShape *ground = cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(-99, 0), cpv(900+99, 0), 0);
	cpShapeSetFriction(ground, 1);
	cpSpaceAddShape(space, ground);



	mike_spawn(&mike, space, 400, 75);



	cpFloat timeStep = 1.0/20.0;
	//printf("test begin\n");
	for(cpFloat time = 0; time < 4; time += timeStep){

		mike_brain_inputs(&mike, input);
		brain_propagate(b, input, output);
		mike_muscle_input(&mike, output);

		//printf("\n    current head height: %f\n", cpBodyGetPosition(mike.head).y);
		cpSpaceStep(space, timeStep);
	}

	//printf("test end\n");

	float h = cpBodyGetPosition(mike.head).x;

	mike_free(&mike);

	cpShapeFree(ground);

	//printf("fine test\n");

	return fmax(1,h);
}

int main()
{
	brain_warmup();
	brain_mutations_warmup();

	sfContextSettings settings = {0};
	settings.antialiasingLevel = 8;
	int xres = 900, yres = 600;
	sfRenderWindow *window = sfRenderWindow_create((sfVideoMode){xres, yres, 32}, "creatures", sfResize | sfClose, &settings);
	sfView *view = sfView_createFromRect((sfFloatRect){0, (float)yres, (float)xres, (float)-yres});
	sfRenderWindow_setView(window, view);

	pool_s pool;
	int pool_size = 100;
	pool_init(&pool, pool_size);
	for (int i = 0; i < pool_size; i++){
		brain_s b;
		brain_init(&b, 12, 4);
		b.fitness = 1;
		pool.brains[i] = b;

	}

	int i = 0;
	int gen_numb = 10000000;
	while (sfRenderWindow_isOpen(window))
	{
		sfEvent event;
		while (sfRenderWindow_pollEvent(window, &event)){
			if (event.type == sfEvtClosed)
			sfRenderWindow_close(window);
		}

		sfRenderWindow_clear(window, sfBlack);

		if (i < gen_numb){
			brain_s best;
			evolve(&pool, test);
			printf("gen numb: %d", i);
			int show_brain_every = 20000000;
			if (i%show_brain_every == (show_brain_every - 1)){
				printf("showing brains: ");
				for (int b = 0; b < pool.size; b++){
					printf("%d ", b);
					fflush(stdout);
					sfRenderWindow_clear(window, sfBlack);
					brain_display(pool.brains + b, window, 0, NULL);
					sfRenderWindow_display(window);
					usleep(20000);
				}
				printf("\n");
			}
			int show_every = 100;
			if (i%show_every == (show_every - 1)){
				cpVect gravity = cpv(0, -100);
				cpSpace *space = cpSpaceNew();
				cpSpaceSetGravity(space, gravity);
				cpShape *ground = cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(-99, 0), cpv(xres+99, 0), 0);
				cpShapeSetFriction(ground, 1);
				cpSpaceAddShape(space, ground);
				cpFloat timeStep = 1.0/20.0;
				for (int s = 0; s < n_species(&pool); s++){
					if (!(pool.species.data[s * (pool.size + 1)])) continue; // spcies is empty
					mike_s mike;
					mike_init(&mike);
					mike_spawn(&mike, space, 400, 75);
					brain_s b = pool.brains[pool.species.data[s * (pool.size + 1) + 1]];
					float input[12];
					float output[4];
					float vis[100000]; // max number of neurons

					for(cpFloat time = 0; time < 4; time += timeStep){
						sfRenderWindow_clear(window, sfBlack);
						mike_brain_inputs(&mike, input);
						assert(b.dict.elements < 100000);
						brain_propagate_vis(&b, input, output, vis);
						mike_muscle_input(&mike, output);
						cpSpaceStep(space, timeStep);
						draw_mike(window, &mike);
						brain_display(&b, window, 1, vis);
						sfRenderWindow_display(window);
					}
					printf("test fitness: %f\n", cpBodyGetPosition(mike.head).x);

					mike_free(&mike);
				}
				cpShapeFree(ground);

			}
			// sleep(1);
		} else if (i == gen_numb) {
			for (int j = 0; j < pool_size; j++){
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
