#include <chipmunk/chipmunk.h>

struct mike{
        cpBody *head, *l, *r, *ll, *rr;
	cpShape *head_s, *l_s, *r_s, *ll_s, *rr_s;
	cpConstraint	*l_c, *ll_c, *r_c, *rr_c,
			*l_m, *ll_m, *r_m, *rr_m;
	cpSpace *space;
};
typedef struct mike mike_s;


void mike_free(mike_s *mike);
void mike_init(mike_s *mike, cpSpace* space, float x, float y);
void mike_brain_input(mike_s *mike, double target, float *brain_inputs);
void mike_muscle_input(mike_s *mike, const float *arr);
void mike_despawn(mike_s *mike);
void mike_reset(mike_s *mike, float x, float y);
