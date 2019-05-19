#include "mike.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define MIKE_COLLISION_GROUP 16

//mike specific shape is hard coded
static const float upper_leg_length = 40;
static const float upper_leg_thickness = 3;
static const float lower_leg_length = 40;
static const float lower_leg_thickness = 3;
static const float head_radius = 10;
static const float upper_leg_density = 0.1;
static const float lower_leg_density = 0.1;
static const float head_density = 0.1;
static const float friction = 0.8;
static const float bounciness = 0.1;
static const float upper_joint_stiffness = 18000;
static const float lower_joint_stiffness = 18000;
static const float upper_joint_damping = 60000;
static const float lower_joint_damping = 60000;



void mike_init(mike_s *m)
{
	m->space = NULL;
	double head_mass = head_radius * head_radius * CP_PI * head_density;
	m->head = cpBodyNew( head_mass,
		cpMomentForCircle( head_mass, head_radius, 0, (cpVect){0,0}));
	m->head_s = cpCircleShapeNew(m->head, head_radius, (cpVect){0,0});

	double upper_leg_mass = upper_leg_length * upper_leg_thickness * upper_leg_density;
	m->l = cpBodyNew(upper_leg_mass,
		cpMomentForBox(upper_leg_mass, upper_leg_thickness, upper_leg_length));
	m->r = cpBodyNew(upper_leg_mass,
		cpMomentForBox(upper_leg_mass, upper_leg_thickness, upper_leg_length));
	m->l_s = cpBoxShapeNew(m->l, upper_leg_thickness, upper_leg_length, 0);
	m->r_s = cpBoxShapeNew(m->r, upper_leg_thickness, upper_leg_length, 0);

	double lower_leg_mass = lower_leg_length * lower_leg_thickness * lower_leg_density;
	m->ll = cpBodyNew(lower_leg_mass,
		cpMomentForBox(lower_leg_mass, lower_leg_thickness, lower_leg_length));
	m->rr = cpBodyNew(lower_leg_mass,
		cpMomentForBox(lower_leg_mass, lower_leg_thickness, lower_leg_length));
	m->ll_s = cpBoxShapeNew(m->ll, lower_leg_thickness, lower_leg_length, 0);
	m->rr_s = cpBoxShapeNew(m->rr, lower_leg_thickness, lower_leg_length, 0);

	cpShapeFilter filter = cpShapeFilterNew(MIKE_COLLISION_GROUP, CP_ALL_CATEGORIES, CP_ALL_CATEGORIES);

	cpShapeSetFilter(m->l_s, filter);
	cpShapeSetFilter(m->ll_s, filter);
	cpShapeSetFilter(m->r_s, filter);
	cpShapeSetFilter(m->rr_s, filter);
	cpShapeSetFilter(m->head_s, filter);

	cpShapeSetFriction(m->head_s, friction);
	cpShapeSetFriction(m->l_s, friction);
	cpShapeSetFriction(m->ll_s, friction);
	cpShapeSetFriction(m->r_s, friction);
	cpShapeSetFriction(m->rr_s, friction);

	cpBodySetPosition(m->head, (cpVect){0, 0});
	cpBodySetPosition(m->l, (cpVect){-(head_radius + upper_leg_thickness / 2), -upper_leg_length * 0.5});
	cpBodySetPosition(m->r, (cpVect){(head_radius + upper_leg_thickness / 2), -upper_leg_length * 0.5});
	cpBodySetPosition(m->ll, (cpVect){-(head_radius + upper_leg_thickness / 2), -upper_leg_length - lower_leg_length * 0.5});
	cpBodySetPosition(m->rr, (cpVect){(head_radius + upper_leg_thickness / 2), -upper_leg_length - lower_leg_length * 0.5});

	m->l_c = cpPivotJointNew(m->l, m->head, (cpVect){-(head_radius + upper_leg_thickness / 2), 0});
	m->ll_c = cpPivotJointNew(m->ll, m->l, (cpVect){-(head_radius + upper_leg_thickness / 2), -upper_leg_length});
	m->r_c = cpPivotJointNew(m->r, m->head, (cpVect){(head_radius + upper_leg_thickness / 2), 0});
	m->rr_c = cpPivotJointNew(m->rr, m->r, (cpVect){(head_radius + upper_leg_thickness / 2), -upper_leg_length});


	m->l_m = cpDampedRotarySpringNew(m->l, m->head, 0, upper_joint_stiffness, upper_joint_damping);
	m->ll_m = cpDampedRotarySpringNew(m->ll, m->l, 0, lower_joint_stiffness, lower_joint_damping);
	m->r_m = cpDampedRotarySpringNew(m->r, m->head, 0, upper_joint_stiffness, upper_joint_damping);
	m->rr_m = cpDampedRotarySpringNew(m->rr, m->r, 0, lower_joint_stiffness, lower_joint_damping);
}

void mike_spawn(mike_s *m, cpSpace* space, double x, double y)
{
	if(m->space){
		fprintf(stderr, "mike already spawned in %p, cannot spawn in %p", m->space, space);
		abort();
	}
	m->space = space;
	cpSpaceAddConstraint(space, m->l_c);
	cpSpaceAddConstraint(space, m->ll_c);
	cpSpaceAddConstraint(space, m->r_c);
	cpSpaceAddConstraint(space, m->rr_c);

	cpSpaceAddConstraint(space, m->l_m);
	cpSpaceAddConstraint(space, m->ll_m);
	cpSpaceAddConstraint(space, m->r_m);
	cpSpaceAddConstraint(space, m->rr_m);

	cpSpaceAddBody(space, m->l);
	cpSpaceAddShape(space, m->l_s);
	cpSpaceAddBody(space, m->ll);
	cpSpaceAddShape(space, m->ll_s);
	cpSpaceAddBody(space, m->r);
	cpSpaceAddShape(space, m->r_s);
	cpSpaceAddBody(space, m->rr);
	cpSpaceAddShape(space, m->rr_s);
	cpSpaceAddBody(space, m->head);
	cpSpaceAddShape(space, m->head_s);

	mike_reset(m, x, y);
}

void mike_reset(mike_s *m, double x, double y)
{
	cpBodySetPosition(m->head, (cpVect){x,y});
	cpBodySetPosition(m->l, (cpVect){-(head_radius + upper_leg_thickness / 2) + x, -upper_leg_length * 0.5 + y});
	cpBodySetPosition(m->r, (cpVect){(head_radius + upper_leg_thickness / 2) + x, -upper_leg_length * 0.5 + y});
	cpBodySetPosition(m->ll, (cpVect){-(head_radius + upper_leg_thickness / 2) + x, -upper_leg_length - lower_leg_length * 0.5 + y});
	cpBodySetPosition(m->rr, (cpVect){(head_radius + upper_leg_thickness / 2) + x, -upper_leg_length - lower_leg_length * 0.5 + y});

	cpBodySetVelocity(m->head, (cpVect){0, 0});
	cpBodySetVelocity(m->l, (cpVect){0, 0});
	cpBodySetVelocity(m->ll,  (cpVect){0, 0});
	cpBodySetVelocity(m->r, (cpVect){0, 0});
	cpBodySetVelocity(m->rr,  (cpVect){0, 0});


	cpBodySetAngularVelocity(m->head, 0);
	cpBodySetAngularVelocity(m->l, 0);
	cpBodySetAngularVelocity(m->ll, 0);
	cpBodySetAngularVelocity(m->r, 0);
	cpBodySetAngularVelocity(m->rr, 0);

	cpBodySetAngle(m->head, 0);
	cpBodySetAngle(m->l, 0);
	cpBodySetAngle(m->ll, 0);
	cpBodySetAngle(m->r, 0);
	cpBodySetAngle(m->rr, 0);

	if (m->space){
		cpSpaceReindexShapesForBody(m->space, m->l);
		cpSpaceReindexShapesForBody(m->space, m->ll);
		cpSpaceReindexShapesForBody(m->space, m->r);
		cpSpaceReindexShapesForBody(m->space, m->rr);
		cpSpaceReindexShapesForBody(m->space, m->head);
	}
}
void mike_despawn(mike_s *m)
{
	if(m->space){
		cpSpaceRemoveBody(m->space, m->head);
		cpSpaceRemoveBody(m->space, m->l);
		cpSpaceRemoveBody(m->space, m->ll);
		cpSpaceRemoveBody(m->space, m->r);
		cpSpaceRemoveBody(m->space, m->rr);

		cpSpaceRemoveShape(m->space, m->head_s);
		cpSpaceRemoveShape(m->space, m->l_s);
		cpSpaceRemoveShape(m->space, m->ll_s);
		cpSpaceRemoveShape(m->space, m->r_s);
		cpSpaceRemoveShape(m->space, m->rr_s);

		cpSpaceRemoveConstraint(m->space, m->l_c);
		cpSpaceRemoveConstraint(m->space, m->ll_c);
		cpSpaceRemoveConstraint(m->space, m->r_c);
		cpSpaceRemoveConstraint(m->space, m->rr_c);

		cpSpaceRemoveConstraint(m->space, m->l_m);
		cpSpaceRemoveConstraint(m->space, m->ll_m);
		cpSpaceRemoveConstraint(m->space, m->r_m);
		cpSpaceRemoveConstraint(m->space, m->rr_m);
	}
}

void mike_free(mike_s *m)
{
	mike_despawn(m);
	cpBodyFree(m->head);
	cpBodyFree(m->l);
	cpBodyFree(m->ll);
	cpBodyFree(m->r);
	cpBodyFree(m->rr);

	cpShapeFree(m->head_s);
	cpShapeFree(m->l_s);
	cpShapeFree(m->ll_s);
	cpShapeFree(m->r_s);
	cpShapeFree(m->rr_s);

	cpConstraintFree(m->l_c);
	cpConstraintFree(m->ll_c);
	cpConstraintFree(m->r_c);
	cpConstraintFree(m->rr_c);

	cpConstraintFree(m->l_m);
	cpConstraintFree(m->ll_m);
	cpConstraintFree(m->r_m);
	cpConstraintFree(m->rr_m);
}

void mike_brain_inputs(mike_s *m, double *output)
{
	output[0] =	1;
	output[1] =	cpBodyGetPosition(m->head).y / 70. - 0.3; //why 0.3?
	output[2] =	(cpBodyGetAngle(m->head) - cpBodyGetAngle(m->l)) / (2 * CP_PI);
	output[3] =	(cpBodyGetAngle(m->l) - cpBodyGetAngle(m->ll)) / (2 * CP_PI);
	output[4] =	(cpBodyGetAngle(m->head) - cpBodyGetAngle(m->r)) / (2 * CP_PI);
	output[5] =	(cpBodyGetAngle(m->r) - cpBodyGetAngle(m->rr)) / (2 * CP_PI);
	output[6] =	cpBodyGetAngle(m->head) / (2 * CP_PI);
	output[7] =	(float)(cpShapeGetBB(m->ll_s).b < 6.);
	output[8] =	(float)(cpShapeGetBB(m->rr_s).b < 6.);
	output[9] =	cpBodyGetVelocity(m->head).x/10;
	output[10] =	cpBodyGetVelocity(m->head).y/10;
}

void mike_muscle_input(mike_s *m, double *arr)
{
	cpDampedRotarySpringSetRestAngle(m->l_m, (arr[0] + 1) * CP_PI);
	cpDampedRotarySpringSetRestAngle(m->ll_m, (arr[1] + 1) * CP_PI);
	cpDampedRotarySpringSetRestAngle(m->r_m, (arr[2] + 1) * CP_PI);
	cpDampedRotarySpringSetRestAngle(m->rr_m, (arr[3] + 1) * CP_PI);
}
