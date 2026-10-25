/**
 * @file rtmpbc_switched_dirac.c
 * @brief Implementation of Switched Dirac Morphism Engine.
 */

#include "rtmpbc_switched_dirac.h"
#include <string.h>

rtmpbc_status_t rtmpbc_switched_dirac_init(
    rtmpbc_switched_dirac_t* sys,
    size_t n_states,
    size_t n_inputs,
    size_t n_switches,
    rtmpbc_real_t dt,
    rtmpbc_arena_t* arena
) {
    if (!sys || !arena || n_states == 0 || dt <= 0.0) {
        return RTMPBC_ERR_NULL_PTR;
    }

    if (n_switches > 8) {
        /* Bounded to 256 modes to guarantee hard real-time memory constraints */
        return RTMPBC_ERR_INVALID_DIM;
    }

    size_t num_modes = 1u << n_switches;
    sys->n_states = n_states;
    sys->n_inputs = n_inputs;
    sys->n_switches = n_switches;
    sys->active_mode_mask = 0;

    sys->J_modes = (rtmpbc_mat_t*)rtmpbc_arena_alloc(arena, num_modes * sizeof(rtmpbc_mat_t), sizeof(void*));
    sys->R_modes = (rtmpbc_mat_t*)rtmpbc_arena_alloc(arena, num_modes * sizeof(rtmpbc_mat_t), sizeof(void*));

    if (!sys->J_modes || !sys->R_modes) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    for (size_t m = 0; m < num_modes; ++m) {
        if (rtmpbc_arena_alloc_mat(arena, n_states, n_states, &sys->J_modes[m]) != RTMPBC_OK ||
            rtmpbc_arena_alloc_mat(arena, n_states, n_states, &sys->R_modes[m]) != RTMPBC_OK) {
            return RTMPBC_ERR_OUT_OF_MEMORY;
        }
        memset(sys->J_modes[m].data, 0, n_states * n_states * sizeof(rtmpbc_real_t));
        memset(sys->R_modes[m].data, 0, n_states * n_states * sizeof(rtmpbc_real_t));
    }

    if (rtmpbc_arena_alloc_mat(arena, n_states, n_states, &sys->Q) != RTMPBC_OK ||
        rtmpbc_arena_alloc_mat(arena, n_states, n_inputs, &sys->G) != RTMPBC_OK) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    /* Allocate dynamic shadow PHS for midpoint solver */
    rtmpbc_phs_linear_t* phs = (rtmpbc_phs_linear_t*)rtmpbc_arena_alloc(
        arena, sizeof(rtmpbc_phs_linear_t), sizeof(void*)
    );
    if (!phs) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    phs->n_states = n_states;
    phs->n_inputs = n_inputs;
    phs->J = sys->J_modes[0];
    phs->R = sys->R_modes[0];
    phs->Q = sys->Q;
    phs->G = sys->G;
    phs->M.data = NULL;
    phs->M.rows = 0;
    phs->M.cols = 0;

    return rtmpbc_midpoint_init(&sys->solver, phs, dt, 3, arena);
}
