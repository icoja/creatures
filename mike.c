#include "mike.h"
#include <math.h>
#include <chipmunk/chipmunk_private.h>
#include <stdio.h>


static const double mike_upper_leg_length = 40;
static const double mike_upper_leg_thickness = 3;
static const double mike_lower_leg_length = 40;
static const double mike_lower_leg_thickness = 3;
static const double mike_head_radius = 10;
static const double mike_upper_leg_density = 0.1;
static const double mike_lower_leg_density = 0.1;
static const double mike_head_density = 0.1;
static const double mike_friction = 0.8;
//static const double mike_bounciness = 0.1;
static const double mike_upper_joint_stiffness = 18000;
static const double mike_lower_joint_stiffness = 18000;
static const double mike_upper_joint_damping = 60000;
static const double mike_lower_joint_damping = 60000;


void mike_init(mike_s *mike, cpSpace* space, float x, float y)
{
	//TODO deleting mike is a mess
	//go through each instance of cp*New()
	//and make sure that everything is freed in the mike_destroy(mike*) function
	mike->space = space;
	double head_mass = mike_head_radius * mike_head_radius * CP_PI * mike_head_density;
	mike->head = cpBodyNew(head_mass,
		cpMomentForCircle(head_mass, mike_head_radius, 0, (cpVect){0,0}));
	mike->head_s = cpCircleShapeNew(mike->head, mike_head_radius, (cpVect){0,0});

	double upper_leg_mass = mike_upper_leg_length * mike_upper_leg_thickness * mike_upper_leg_density;
	mike->l = cpBodyNew(upper_leg_mass,
		cpMomentForBox(upper_leg_mass, mike_upper_leg_thickness, mike_upper_leg_length));
	mike->r = cpBodyNew(upper_leg_mass,
		cpMomentForBox(upper_leg_mass, mike_upper_leg_thickness, mike_upper_leg_length));
	mike->r_s = cpBoxShapeNew(mike->r, mike_upper_leg_thickness, mike_upper_leg_length, 1);
        mike->l_s = cpBoxShapeNew(mike->l, mike_upper_leg_thickness, mike_upper_leg_length, 1);

	double lower_leg_mass = mike_lower_leg_length * mike_lower_leg_thickness * mike_lower_leg_density;
	mike->ll = cpBodyNew(lower_leg_mass,
		cpMomentForBox(lower_leg_mass, mike_lower_leg_thickness, mike_lower_leg_length));
	mike->rr = cpBodyNew(lower_leg_mass,
		cpMomentForBox(lower_leg_mass, mike_lower_leg_thickness, mike_lower_leg_length));
	mike->ll_s = cpBoxShapeNew(mike->ll, mike_lower_leg_thickness, mike_lower_leg_length, 1); // ultimo arg è il "radius" (Adding a small radius will bevel the corners and can significantly reduce problems where the box gets stuck on seams in your geometry)
	mike->rr_s = cpBoxShapeNew(mike->rr, mike_lower_leg_thickness, mike_lower_leg_length, 1);
	
	//cpShapeFilter only_ground = cpShapeFilterNew(1, 0, 0);
	
        //mike->l_s->filter =  // (uint8_t)0b00000000, (uint8_t)0b11111111); // cpGroup group, cpBitmask categories, cpBitmask mask. docs: cpShape.h
	
	//LEET number cause the filter group aint no bitmask
        mike->l_s->filter.group = 1337;
	mike->ll_s->filter.group = 1337;
	mike->r_s->filter.group = 1337;
	mike->rr_s->filter.group = 1337;
	mike->head_s->filter.group = 1337;

	cpShapeSetFriction(mike->head_s, mike_friction);
	cpShapeSetFriction(mike->l_s, mike_friction);
	cpShapeSetFriction(mike->ll_s, mike_friction);
	cpShapeSetFriction(mike->r_s, mike_friction);
	cpShapeSetFriction(mike->rr_s, mike_friction);

	mike->head->p = (cpVect){0,0};
	mike->l->p = (cpVect){-(mike_head_radius + mike_upper_leg_thickness / 2), -mike_upper_leg_length * 0.5};
	mike->r->p = (cpVect){(mike_head_radius + mike_upper_leg_thickness / 2), -mike_upper_leg_length * 0.5};
	mike->ll->p = (cpVect){-(mike_head_radius + mike_upper_leg_thickness / 2), -mike_upper_leg_length - mike_lower_leg_length * 0.5};
	mike->rr->p = (cpVect){(mike_head_radius + mike_upper_leg_thickness / 2), -mike_upper_leg_length - mike_lower_leg_length * 0.5};

	mike->l_c = cpPivotJointNew(mike->l, mike->head, (cpVect){-(mike_head_radius + mike_upper_leg_thickness / 2), 0});
	mike->ll_c = cpPivotJointNew(mike->ll, mike->l, (cpVect){-(mike_head_radius + mike_upper_leg_thickness / 2), -mike_upper_leg_length});
	mike->r_c = cpPivotJointNew(mike->r, mike->head, (cpVect){(mike_head_radius + mike_upper_leg_thickness / 2), 0});
	mike->rr_c = cpPivotJointNew(mike->rr, mike->r, (cpVect){(mike_head_radius + mike_upper_leg_thickness / 2), -mike_upper_leg_length});


	mike->l_m = cpDampedRotarySpringNew(mike->l, mike->head, 0, mike_upper_joint_stiffness, mike_upper_joint_damping);
	mike->ll_m = cpDampedRotarySpringNew(mike->ll, mike->l, 0, mike_lower_joint_stiffness, mike_lower_joint_damping);
	mike->r_m = cpDampedRotarySpringNew(mike->r, mike->head, 0, mike_upper_joint_stiffness, mike_upper_joint_damping);
	mike->rr_m = cpDampedRotarySpringNew(mike->rr, mike->r, 0, mike_lower_joint_stiffness, mike_lower_joint_damping);


	cpSpaceAddConstraint(space, mike->l_c);
	cpSpaceAddConstraint(space, mike->ll_c);
	cpSpaceAddConstraint(space, mike->r_c);
	cpSpaceAddConstraint(space, mike->rr_c);

	cpSpaceAddConstraint(space, mike->l_m);
	cpSpaceAddConstraint(space, mike->ll_m);
	cpSpaceAddConstraint(space, mike->r_m);
	cpSpaceAddConstraint(space, mike->rr_m);

	cpSpaceAddBody(space, mike->l);
	cpSpaceAddShape(space, mike->l_s);
	cpSpaceAddBody(space, mike->ll);
	cpSpaceAddShape(space, mike->ll_s);
	cpSpaceAddBody(space, mike->r);
	cpSpaceAddShape(space, mike->r_s);
	cpSpaceAddBody(space, mike->rr);
	cpSpaceAddShape(space, mike->rr_s);
	cpSpaceAddBody(space, mike->head);
	cpSpaceAddShape(space, mike->head_s);

	mike_reset(mike, x, y);
}

int main(){
        printf("ddd\n");
}
