/**
 * @file muscle_actuator.c
 * @brief Compiled zero-branching unrolled execution kernel.
 */

#include "muscle_actuator.h"
#include <string.h>

void muscle_actuator_init(muscle_actuator_sim_t* sim) {
    if (!sim) return;
    memset(sim->x, 0, MUSCLE_ACTUATOR_N_STATES * sizeof(rtmpbc_real_t));
    memset(sim->u, 0, MUSCLE_ACTUATOR_INPUTS_DIM * sizeof(rtmpbc_real_t));
    memset(sim->y, 0, MUSCLE_ACTUATOR_INPUTS_DIM * sizeof(rtmpbc_real_t));
    sim->total_energy = 0.0;
}

rtmpbc_real_t muscle_actuator_energy(const muscle_actuator_sim_t* sim) {
    rtmpbc_real_t h = 0.0;
    h += 0.5 * 4975.124378109453 * sim->x[0] * sim->x[0];
    h += 0.5 * 6.666666666667 * sim->x[1] * sim->x[1];
    return h;
}

void muscle_actuator_step(muscle_actuator_sim_t* sim) {
    if (!sim) return;
    rtmpbc_real_t b[256];
    const rtmpbc_real_t* x = sim->x;
    const rtmpbc_real_t* u = sim->u;
    (void)u;
    (void)x;

    /* Flat unrolled instruction tape (Zero Branches, SIMD-friendly) */
    b[0] = 0.000000000000;
    b[0] += 0.998963515755 * x[0];
    b[0] += 0.000500000000 * u[0];
    b[1] = 0.000000000000;
    b[1] += 0.941666666667 * x[1];

    /* Direct state update */
    sim->x[0] = b[0];
    sim->x[1] = b[1];

    sim->total_energy = muscle_actuator_energy(sim);
}
