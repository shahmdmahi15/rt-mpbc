/**
 * @file robotic_sensor.c
 * @brief Compiled zero-branching unrolled execution kernel.
 */

#include "robotic_sensor.h"
#include <string.h>

void robotic_sensor_init(robotic_sensor_sim_t* sim) {
    if (!sim) return;
    memset(sim->x, 0, ROBOTIC_SENSOR_N_STATES * sizeof(rtmpbc_real_t));
    memset(sim->u, 0, ROBOTIC_SENSOR_INPUTS_DIM * sizeof(rtmpbc_real_t));
    memset(sim->y, 0, ROBOTIC_SENSOR_INPUTS_DIM * sizeof(rtmpbc_real_t));
    sim->total_energy = 0.0;
}

rtmpbc_real_t robotic_sensor_energy(const robotic_sensor_sim_t* sim) {
    rtmpbc_real_t h = 0.0;
    h += 0.5 * 2222.222222222222 * sim->x[0] * sim->x[0];
    h += 0.5 * 20000.000000000000 * sim->x[1] * sim->x[1];
    h += 0.5 * 1.250000000000 * sim->x[2] * sim->x[2];
    return h;
}

void robotic_sensor_step(robotic_sensor_sim_t* sim) {
    if (!sim) return;
    rtmpbc_real_t b[256];
    const rtmpbc_real_t* x = sim->x;
    const rtmpbc_real_t* u = sim->u;
    (void)u;
    (void)x;

    /* Flat unrolled instruction tape (Zero Branches, SIMD-friendly) */
    b[0] = 0.000000000000;
    b[0] += 0.993333333333 * x[0];
    b[0] += 0.000050000000 * u[0];
    b[1] = 0.000000000000;
    b[1] += -32.333333333333 * x[1];
    b[1] += 0.000031250000 * x[2];
    b[2] = 0.000000000000;
    b[2] += 0.999999531250 * x[2];
    b[2] += -0.500000000000 * x[1];

    /* Direct state update */
    sim->x[0] = b[0];
    sim->x[1] = b[1];
    sim->x[2] = b[2];

    sim->total_energy = robotic_sensor_energy(sim);
}
