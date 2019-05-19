#include <chipmunk/chipmunk.h>

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
void mike_brain_inputs(mike_s *m, double *output);
//how muscles should move
void mike_muscle_input(mike_s *m, double *input);
//places mike in a still standing position at coordinates x,y
void mike_reset(mike_s *m, double x, double y);

/*
void draw_mike(sf::RenderWindow &window, Mike &m, double target)
{
	sf::CircleShape head {(float)m.head_radius};
	head.setOrigin(m.head_radius, m.head_radius);
	head.setPosition(cpBodyGetPosition(m.head).x, cpBodyGetPosition(m.head).y);
	window.draw(head);
	// debug target loc
	head = sf::CircleShape {3};
	head.setPosition(target, 100);
	window.draw(head);
	//
	sf::RectangleShape upper_leg {sf::Vector2f{(float)m.upper_leg_thickness, (float)m.upper_leg_length}};
	sf::RectangleShape lower_leg {sf::Vector2f{(float)m.lower_leg_thickness, (float)m.lower_leg_length}};
	upper_leg.setOrigin(m.upper_leg_thickness/2., m.upper_leg_length/2.);
	lower_leg.setOrigin(m.lower_leg_thickness/2., m.lower_leg_length/2.);
	upper_leg.setPosition(cpBodyGetPosition(m.l).x, cpBodyGetPosition(m.l).y);
	lower_leg.setPosition(cpBodyGetPosition(m.ll).x, cpBodyGetPosition(m.ll).y);
	upper_leg.setRotation(cpBodyGetAngle(m.l) / (2 * M_PI) * 360);
	lower_leg.setRotation(cpBodyGetAngle(m.ll) / (2 * M_PI) * 360);
	window.draw(upper_leg);
	window.draw(lower_leg);
	upper_leg.setPosition(cpBodyGetPosition(m.r).x, cpBodyGetPosition(m.r).y);
	lower_leg.setPosition(cpBodyGetPosition(m.rr).x, cpBodyGetPosition(m.rr).y);
	upper_leg.setRotation(cpBodyGetAngle(m.r) / (2 * M_PI) * 360);
	lower_leg.setRotation(cpBodyGetAngle(m.rr) / (2 * M_PI) * 360);
	window.draw(upper_leg);
	window.draw(lower_leg);
}*/
