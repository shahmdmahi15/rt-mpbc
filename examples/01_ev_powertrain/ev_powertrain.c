/**
 * @file ev_powertrain.c
 * @brief Compiled zero-branching unrolled execution kernel.
 */

#include "ev_powertrain.h"
#include <string.h>

void ev_powertrain_init(ev_powertrain_sim_t* sim) {
    if (!sim) return;
    memset(sim->x, 0, EV_POWERTRAIN_N_STATES * sizeof(rtmpbc_real_t));
    memset(sim->u, 0, EV_POWERTRAIN_INPUTS_DIM * sizeof(rtmpbc_real_t));
    memset(sim->y, 0, EV_POWERTRAIN_INPUTS_DIM * sizeof(rtmpbc_real_t));
    sim->total_energy = 0.0;
}

rtmpbc_real_t ev_powertrain_energy(const ev_powertrain_sim_t* sim) {
    rtmpbc_real_t h = 0.0;
    h += 0.5 * 0.010000000000 * sim->x[0] * sim->x[0];
    h += 0.5 * 50.000000000000 * sim->x[1] * sim->x[1];
    h += 0.5 * 1000.000000000000 * sim->x[2] * sim->x[2];
    h += 0.5 * 0.000555555556 * sim->x[3] * sim->x[3];
    return h;
}

void ev_powertrain_step(ev_powertrain_sim_t* sim) {
    if (!sim) return;
    rtmpbc_real_t b[256];
    const rtmpbc_real_t* x = sim->x;
    const rtmpbc_real_t* u = sim->u;
    (void)u;
    (void)x;

    /* Flat unrolled instruction tape (Zero Branches, SIMD-friendly) */
    b[0] = 0.000000000000;
    b[0] += 0.999990000000 * x[0];
    b[1] = 0.000000000000;
    b[1] += 1.000000000000 * x[1];
    b[2] = 0.000000000000;
    b[2] += 0.998000000000 * x[2];
    b[2] += 0.000000027778 * x[3];
    b[3] = 0.000000000000;
    b[3] += 0.999999305556 * x[3];
    b[3] += -0.050000000000 * x[2];

    /* Direct state update */
    sim->x[0] = b[0];
    sim->x[1] = b[1];
    sim->x[2] = b[2];
    sim->x[3] = b[3];

    sim->total_energy = ev_powertrain_energy(sim);
}
