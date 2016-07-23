#include <stdlib.h>
#include <soundpipe.h>
#include <sporth.h>
#include "scheme-private.h"
#include "polysporth.h"

static int sporth_ps(plumber_data *pd, sporth_stack *stack, void **ud)
{
    polysporth *ps;

    char *in_tbl;
    char *out_tbl;
    char *filename;
    int ninstances;
    SPFLOAT clock;
    SPFLOAT tick;
    switch(pd->mode) {
        case PLUMBER_CREATE:

#ifdef DEBUG_MODE
            fprintf(stderr, "polysporth: Creating\n");
#endif

            ps = malloc(sizeof(polysporth));
            *ud = ps;
            if(sporth_check_args(stack, "fffsss") != SPORTH_OK) {
                fprintf(stderr, "polysporth: not enough/wrong arguments\n");
                return PLUMBER_NOTOK;
            }

            filename = sporth_stack_pop_string(stack);
            out_tbl = sporth_stack_pop_string(stack);
            in_tbl = sporth_stack_pop_string(stack);
            ninstances = (int) sporth_stack_pop_float(stack);
            clock = sporth_stack_pop_float(stack);
            tick = sporth_stack_pop_float(stack);
            if(ps_init(pd, stack, ps, ninstances, in_tbl, out_tbl, filename) == PLUMBER_NOTOK) {
                fprintf(stderr, "Initialization of polysporth failed\n");
                return PLUMBER_NOTOK;
            }

            break;

        case PLUMBER_INIT:
            filename = sporth_stack_pop_string(stack);
            out_tbl = sporth_stack_pop_string(stack);
            in_tbl = sporth_stack_pop_string(stack);
            ninstances = (int) sporth_stack_pop_float(stack);
            clock = sporth_stack_pop_float(stack);
            tick = sporth_stack_pop_float(stack);
            
#ifdef DEBUG_MODE
            fprintf(stderr, "polysporth: Initialising\n");
#endif
            break;

        case PLUMBER_COMPUTE:
            ninstances = (int) sporth_stack_pop_float(stack);
            clock = sporth_stack_pop_float(stack);
            tick = sporth_stack_pop_float(stack);
            ps = *ud;
            ps_compute(ps, tick, clock);

            break;
        case PLUMBER_DESTROY:
            ps = *ud;
            ps_clean(ps);
            free(ps);
            break;
        default:
            fprintf(stderr, "polysporth: Unknown mode!\n");
            break;
    }
    return PLUMBER_OK;
}

static int sporth_nil(plumber_data *pd, sporth_stack *stack, void **ud)
{
    return PLUMBER_OK;
}

plumber_dyn_func sporth_return_ugen()
{
    return sporth_ps;
}
