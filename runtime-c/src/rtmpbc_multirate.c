/**
 * @file rtmpbc_multirate.c
 * @brief Implementation of Discrete Variational Multi-Rate Boundary Commutator.
 */

#include "rtmpbc_multirate.h"
#include <string.h>

rtmpbc_status_t rtmpbc_multirate_init(
    rtmpbc_multirate_coupler_t* coupler,
    size_t n_ports,
    size_t sub_ratio,
    rtmpbc_real_t dt_micro,
    rtmpbc_arena_t* arena
) {
    if (!coupler || !arena || n_ports == 0 || sub_ratio == 0 || dt_micro <= 0.0) {
        return RTMPBC_ERR_NULL_PTR;
    }

    coupler->n_ports = n_ports;
    coupler->sub_ratio = sub_ratio;
    coupler->current_substep = 0;
    coupler->dt_micro = dt_micro;
    coupler->dt_macro = dt_micro * (rtmpbc_real_t)sub_ratio;

    coupler->impulse_accum = rtmpbc_arena_alloc_real(arena, n_ports);
    coupler->last_macro_flow = rtmpbc_arena_alloc_real(arena, n_ports);

    if (!coupler->impulse_accum || !coupler->last_macro_flow) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    memset(coupler->impulse_accum, 0, n_ports * sizeof(rtmpbc_real_t));
    memset(coupler->last_macro_flow, 0, n_ports * sizeof(rtmpbc_real_t));

    coupler->energy_transferred_micro = 0.0;
    coupler->energy_transferred_macro = 0.0;

    return RTMPBC_OK;
}

rtmpbc_status_t rtmpbc_multirate_micro_step(
    rtmpbc_multirate_coupler_t* coupler,
    const rtmpbc_real_t* e_micro,
    rtmpbc_real_t* f_micro_out
) {
    if (!coupler || !e_micro || !f_micro_out) {
        return RTMPBC_ERR_NULL_PTR;
    }

    /* Accumulate boundary impulse: p += e_micro * dt_micro */
    for (size_t i = 0; i < coupler->n_ports; ++i) {
        rtmpbc_real_t de = e_micro[i] * coupler->dt_micro;
        coupler->impulse_accum[i] += de;

        /* Provide slow system's flow to fast system */
        f_micro_out[i] = coupler->last_macro_flow[i];

        /* Energy tracking */
        coupler->energy_transferred_micro += e_micro[i] * coupler->last_macro_flow[i] * coupler->dt_micro;
    }

    coupler->current_substep++;
    return RTMPBC_OK;
}

rtmpbc_status_t rtmpbc_multirate_macro_step(
    rtmpbc_multirate_coupler_t* coupler,
    rtmpbc_real_t* e_macro_out,
    const rtmpbc_real_t* f_macro
) {
    if (!coupler || !e_macro_out || !f_macro) {
        return RTMPBC_ERR_NULL_PTR;
    }

    /* Deliver mean effort: e_macro = p_accum / dt_macro */
    for (size_t i = 0; i < coupler->n_ports; ++i) {
        e_macro_out[i] = coupler->impulse_accum[i] / coupler->dt_macro;
        coupler->last_macro_flow[i] = f_macro[i];

        /* Energy tracking */
        coupler->energy_transferred_macro += e_macro_out[i] * f_macro[i] * coupler->dt_macro;

        /* Reset accumulator for next macro cycle */
        coupler->impulse_accum[i] = 0.0;
    }

    coupler->current_substep = 0;
    return RTMPBC_OK;
}
