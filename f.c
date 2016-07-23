#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <soundpipe.h>
#include <sporth.h>
#include "scheme-private.h"
#include "polysporth.h"

#define car(p) ((p)->_object._cons._car)
#define cdr(p) ((p)->_object._cons._cdr)

static pointer ps_eval(scheme *sc, pointer args);
static pointer ps_parse(scheme *sc, pointer args);
static pointer ps_turnon(scheme *sc, pointer args);
static pointer ps_turnoff(scheme *sc, pointer args);
static pointer ps_noteblock_begin(scheme *sc, pointer args);
static pointer ps_note(scheme *sc, pointer args);
static pointer ps_noteblock_end(scheme *sc, pointer args);
static pointer ps_metanote(scheme *sc, pointer args);
static pointer ps_set_release(scheme *sc, pointer args);
static pointer ps_tset(scheme *sc, pointer args);
static pointer ps_tget(scheme *sc, pointer args);
static pointer ps_lexer(scheme *sc, pointer args);
static pointer ps_init_sporthlet(scheme *sc, pointer args);
static pointer ps_show_pipes(scheme *sc, pointer args);
static pointer ps_write_code(scheme *sc, pointer args);

void ps_scm_load(polysporth *ps, char *filename)
{
    /* load scheme */
    scheme *sc = &ps->sc;
    if(!scheme_init(sc)) {
        fprintf(stderr,"Polysporth: could not initialize!\n");
        return;
    }
    sc->code = sc->NIL;
    scheme_set_input_port_file(sc, stdin);
    scheme_set_output_port_file(sc, stdout);

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-eval"), 
        mk_foreign_func(sc, ps_eval));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-parse"), 
        mk_foreign_func(sc, ps_parse));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-turnon"), 
        mk_foreign_func(sc, ps_turnon));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-init-sporthlet"), 
        mk_foreign_func(sc, ps_init_sporthlet));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-lexer"), 
        mk_foreign_func(sc, ps_lexer));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-write-code"), 
        mk_foreign_func(sc, ps_write_code));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-show-pipes"), 
        mk_foreign_func(sc, ps_show_pipes));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-noteblock-begin"), 
        mk_foreign_func(sc, ps_noteblock_begin));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-noteblock-end"), 
        mk_foreign_func(sc, ps_noteblock_end));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-set-release"), 
        mk_foreign_func(sc, ps_set_release));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-note"), 
        mk_foreign_func(sc, ps_note));
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "ps-metanote"), 
        mk_foreign_func(sc, ps_metanote));

    //s7_define_function(ps->s7, "ps-metanote", ps_metanote, 2, 0, false, "TODO");
    
    sc->ext_data = (void *)ps;

    FILE *fp =fopen(filename,"r");
    scheme_load_file(sc, fp);
    fclose(fp);
    //s7_load(ps->s7, filename);
}

static pointer ps_eval(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    sporthlet *spl;
    plumber_data *pd = &ps->pd;
    int id = ivalue(car(args));
    args = cdr(args);
    const char *str = string_value(car(args));

    spl = &ps->spl[id];
    pd->tmp = &spl->pipes;
    plumbing_parse_string(pd, &spl->pipes, (char *)str);
    plumbing_compute(pd, &spl->pipes, PLUMBER_INIT);
    spl->state = PS_OFF;
    return sc->NIL;
}

/* Parse: just parses, doesn't call plumbing_compute. Can be called multiple times.
 * Designed to build up patches programatically. */
static pointer ps_parse(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    sporthlet *spl;
    plumber_data *pd = &ps->pd;

    int id = (int)ivalue(car(args));
    args = cdr(args);
    const char *str = string_value(car(args));
    spl = &ps->spl[id];
    pd->tmp = &spl->pipes;
    plumbing_parse_string(pd, &spl->pipes, (char *)str);
    spl->state = PS_OFF;
    return sc->NIL;
}

static pointer ps_turnon(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int id = (int) ivalue(car(args));
    args = cdr(args);
    int dur = (int) ivalue(car(args));
    ps_turnon_sporthlet(ps, id, dur);
    return sc->NIL;
}

static pointer ps_turnoff(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int id = (int) ivalue(car(args));
    ps_turnoff_sporthlet(ps, id);
    return sc->NIL;
}

static pointer ps_init_sporthlet(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int id = (int)ivalue(car(args));

    /* TODO: error checking */
    sporthlet *spl = &ps->spl[id];
    plumbing_compute(&ps->pd, &spl->pipes, PLUMBER_INIT);
    spl->state = PS_OFF;
    return sc->NIL;
}


static pointer ps_lexer(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int id = (int)ivalue(car(args));
    args = cdr(args);
    const char *str = string_value(car(args));
    uint32_t size = strlen(str);
    plumbing *old_tmp;
    /* TODO: error checking */
    sporthlet *spl= &ps->spl[id];
    int prev_mode = ps->pd.mode;
    ps->pd.mode = PLUMBER_CREATE;
    old_tmp = ps->pd.tmp; 
    ps->pd.tmp = &spl->pipes; 
    plumber_lexer(&ps->pd, &spl->pipes, (char *)str, size);
    ps->pd.tmp = old_tmp; 
    ps->pd.mode = prev_mode;
    return sc->NIL;
}

static pointer ps_show_pipes(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int id = (int) ivalue(car(args));

    /* TODO: error checking */
    sporthlet *spl = &ps->spl[id];
    plumbing_show_pipes(&ps->pd, &spl->pipes);
    return sc->NIL;
}

static pointer ps_write_code(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int id = (int) ivalue(car(args));
    /* TODO: error checking */
    sporthlet *spl = &ps->spl[id];
    plumbing_write_code(&ps->pd, &spl->pipes, stdout);
    return sc->NIL;
}

static pointer ps_noteblock_begin(scheme *sc, pointer args)
{
#ifdef DEBUG_POLYSPORTH
    fprintf(stderr, "--- NOTEBLOCK BEGIN ---\n");
#endif
    polysporth *ps = sc->ext_data;
    dvector_init(&ps->tmp);
    if(ps->noteblock == PS_OFF) {
        ps->noteblock = PS_ON;
    } else {
        fprintf(stderr,
            "Warning: noteblock already on. Not doing anything.\n");
    }
    return NULL;
}

static pointer ps_noteblock_end(scheme *sc, pointer args)
{
#ifdef DEBUG_POLYSPORTH
    fprintf(stderr, "--- NOTEBLOCK END ---\n");
#endif
    polysporth *ps = sc->ext_data;
    if(ps->noteblock == PS_ON) {
        dvector_time_to_delta(&ps->tmp);
        //ps->events = dvector_merge(&ps->events, &ps->tmp);
        ps->noteblock = PS_OFF;
    }
    return NULL;
}

static pointer ps_note(scheme *sc, pointer args)
{
#ifdef DEBUG_POLYSPORTH
    fprintf(stderr, "ps-note: adding value\n");
#endif
    polysporth *ps = sc->ext_data;
    int grp_start = 
        (int)ivalue(car(args));
    args = cdr(args);
    int grp_end = 
        (int)ivalue(car(args));
    args = cdr(args);
    int start = 
        (int)ivalue(car(args));
    args = cdr(args);
    int dur = 
        (int)ivalue(car(args));
    args = cdr(args);
    /* add release value to duration */
    dur += ps->reltime;
    int len = 0;


    pointer tmp = args;
    while(args != sc->NIL) {
        len++;
#ifdef DEBUG_POLYSPORTH
        printf("value is %g\n", rvalue(car(args)));
#endif
        args = cdr(args);
    }

    SPFLOAT *argtbl = NULL;

    if(len > 0) {
        int i;
        argtbl = malloc(sizeof(SPFLOAT) * len);
        args = tmp;
        for(i = 0; i < len; i++) {
            argtbl[i] = rvalue(car(args));
            args = cdr(args);
#ifdef DEBUG_POLYSPORTH
            fprintf(stderr, "--- %g\n", argtbl[i]);
#endif
        }
    }

    dvector_append(&ps->tmp, grp_start, grp_end, start, dur, argtbl, len);
    return NULL;
}

static pointer ps_metanote(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int start = (int)ivalue(car(args));
    args = cdr(args);
    pointer func = car(args);
    dvector_append_metanote(&ps->tmp, start, func);
    return sc->NIL;
}

static pointer ps_set_release(scheme *sc, pointer args)
{
    polysporth *ps = sc->ext_data;
    int release = ivalue(car(args));
    ps->reltime = release;
    return sc->NIL;
}