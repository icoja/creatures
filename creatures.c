#include <unistd.h>
#include <math.h>
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



	cpFloat timeStep = 1.0/30.0;
	//printf("test begin\n");
	for(cpFloat time = 0; time < 4; time += timeStep){

		mike_brain_inputs(&mike, input);
		//printf("    input:");
		for (int i = 0; i < 12; i++) //printf(" %f", input[i]);
		brain_propagate(b, input, output);
		//printf("\n    output:");
		for (int i = 0; i < 4; i++) //printf(" %f", output[i]);
		mike_muscle_input(&mike, output);

		//printf("\n    current head height: %f\n", cpBodyGetPosition(mike.head).y);

		cpSpaceStep(space, timeStep);
	}

	//printf("test end\n");

	float h = cpBodyGetPosition(mike.head).y;

	mike_free(&mike);

	cpShapeFree(ground);

	//printf("fine test\n");

	return h;
}

int short_demo()
{
	brain_warmup();
	brain_mutations_warmup();

	mike_s mike;
	mike_init(&mike);

	sfContextSettings settings = {0};
	settings.antialiasingLevel = 8;
	int xres = 900, yres = 600;
	sfRenderWindow *window = sfRenderWindow_create((sfVideoMode){xres, yres, 32}, "creatures", sfResize | sfClose, &settings);
	sfView *view = sfView_createFromRect((sfFloatRect){0, (float)yres, (float)xres, (float)-yres});
	sfRenderWindow_setView(window, view);


	cpVect gravity = cpv(0, -100);

	// Create an empty space.
	cpSpace *space = cpSpaceNew();
	cpSpaceSetGravity(space, gravity);

	// Add a static line segment shape for the ground.
	// We'll make it slightly tilted so the ball will roll off.
	// We attach it to a static body to tell Chipmunk it shouldn't be movable.
	cpShape *ground = cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(-99, 0), cpv(xres+99, 0), 0);
	cpShapeSetFriction(ground, 1);
	cpSpaceAddShape(space, ground);



	mike_spawn(&mike, space, 300, 100);


	cpFloat timeStep = 1.0/60.0;
	for(cpFloat time = 0; time < 20; time += timeStep){
		sfRenderWindow_clear(window, sfBlack);
		cpSpaceStep(space, timeStep);
		draw_mike(window, &mike);
		sfRenderWindow_display(window);

	}


	int pool_size = 100;

	pool_s pool;
	pool_init(&pool, pool_size);
	for (int i = 0; i < pool_size; i++){
		brain_s b;
		brain_init(&b, 2, 1);
		b.fitness = 0;
		pool.brains[i] = b;

	}

	for (int i = 0; i < 4; i++){
		evolve(&pool, test);
	}



	print_pool(&pool);


	cpShapeFree(ground);
	return 0;
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
	int pool_size = 80;
	pool_init(&pool, pool_size);
	for (int i = 0; i < pool_size; i++){
		brain_s b;
		brain_init(&b, 12, 4);
		b.fitness = 1;
		pool.brains[i] = b;

	}

	int i = 0;
	int gen_numb = 10000000000;
	while (sfRenderWindow_isOpen(window))
	{
		sfEvent event;
		while (sfRenderWindow_pollEvent(window, &event)){
			if (event.type == sfEvtClosed)
			sfRenderWindow_close(window);
		}

		sfRenderWindow_clear(window, sfBlack);

		if (i < gen_numb){
			evolve(&pool, test);
			printf("gen numb: %d, fitenss: %f\n", i, test(&pool.brains[0]));

			if (i%50 == 0){

				cpVect gravity = cpv(0, -100);
				cpSpace *space = cpSpaceNew();
				cpSpaceSetGravity(space, gravity);
				cpShape *ground = cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(-99, 0), cpv(xres+99, 0), 0);
				cpShapeSetFriction(ground, 1);
				cpSpaceAddShape(space, ground);
				mike_s mike;
				mike_init(&mike);
				mike_spawn(&mike, space, 400, 75);
				brain_s b = pool.brains[0];
				cpFloat timeStep = 1.0/60.0;
				float input[12];
				float output[4];
				assert(b.dict.elements < 1000);
				float vis[1000]; // max number of neurons

				for(cpFloat time = 0; time < 2; time += timeStep){
					sfRenderWindow_clear(window, sfBlack);
					mike_brain_inputs(&mike, input);
					brain_propagate_vis(&b, input, output, vis);
					mike_muscle_input(&mike, output);
					cpSpaceStep(space, timeStep);
					draw_mike(window, &mike);
					brain_display(&b, window, 1, vis);
					sfRenderWindow_display(window);
				}
				mike_free(&mike);
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
