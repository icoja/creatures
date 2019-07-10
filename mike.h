#pragma once
#include <chipmunk/chipmunk.h>

/*
mike body constants
these constants pollute the main namespace. they should be isolated somehow (separate header)
as they are also needed in visual.c for mike's visualization
*/
static const float upper_leg_length = 40;
static const float upper_leg_thickness = 3;
static const float lower_leg_length = 40;
static const float lower_leg_thickness = 3;
static const float head_radius = 10;
static const float upper_leg_density = 0.03;
static const float lower_leg_density = 0.03;
static const float head_density = 0.1;
static const float friction = 0.8;
static const float bounciness = 0.1;
static const float upper_joint_stiffness = 100000; // 18000;
static const float lower_joint_stiffness = 100000; // 18000;
static const float upper_joint_damping = 10000; // 60000;
static const float lower_joint_damping = 10000; // 60000;

typedef struct mike{
	cpBody *head, *l, *r, *ll, *rr;
	cpShape *head_s, *l_s, *r_s, *ll_s, *rr_s;
	cpConstraint	*l_c, *ll_c, *r_c, *rr_c,
			*l_m, *ll_m, *r_m, *rr_m;
	cpSpace *space;
} mike_s;

void mike_free(mike_s *m);
//initializes mike
void mike_init(mike_s *m);
//adds mike to a cpSpace. Cannot add the same mike twice.
void mike_spawn(mike_s *m, cpSpace* space, double x, double y);
//removes mike to the space he is currently in
void mike_despawn(mike_s *m);
//puts into output what mike senses. The sizeo of outputs is hard coded. (BAD)
void mike_brain_inputs(mike_s *m, float *output);
//how muscles should move
void mike_muscle_input(mike_s *m, float *input);
//places mike in a still standing position at coordinates x,y
void mike_reset(mike_s *m, double x, double y);
