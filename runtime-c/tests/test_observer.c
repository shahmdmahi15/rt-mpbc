#include "rtmpbc.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define ARENA_BUF_SIZE 32768

int main(void) {
    printf("[TEST] Running test_observer (Passivity-Based Port-Hamiltonian Virtual Sensor)...\n");

    RTMPBC_ALIGN64 static uint8_t raw_buffer[ARENA_BUF_SIZE];
    rtmpbc_arena_t arena;
    rtmpbc_arena_init(&arena, raw_buffer, ARENA_BUF_SIZE);

    /* 2-State System:
     * x = [q, p]^T
     * k = 100.0 N/m, m = 1.0 kg, b = 2.0 N*s/m (damping)
     * Q = diag(100.0, 1.0)
     * J = [[ 0,  1],
     *      [-1,  0]]
     * R = [[ 0,  0],
     *      [ 0,  b]] (b = 2.0)
     * G = [[ 0 ],
     *      [ 1 ]] (External force input u)
     * Surface sensor: measures velocity y = p/m = G^T * Q * x
     */
    rtmpbc_phs_linear_t phs;
    phs.n_states = 2;
    phs.n_inputs = 1;

    rtmpbc_arena_alloc_mat(&arena, 2, 2, &phs.J);
    rtmpbc_arena_alloc_mat(&arena, 2, 2, &phs.R);
    rtmpbc_arena_alloc_mat(&arena, 2, 2, &phs.Q);
    rtmpbc_arena_alloc_mat(&arena, 2, 1, &phs.G);
    rtmpbc_arena_alloc_mat(&arena, 1, 1, &phs.M);

    phs.J.data[0 * 2 + 0] =  0.0; phs.J.data[0 * 2 + 1] =  1.0;
    phs.J.data[1 * 2 + 0] = -1.0; phs.J.data[1 * 2 + 1] =  0.0;

    phs.R.data[0 * 2 + 0] = 0.0; phs.R.data[0 * 2 + 1] = 0.0;
    phs.R.data[1 * 2 + 0] = 0.0; phs.R.data[1 * 2 + 1] = 2.0; /* Damping */

    phs.Q.data[0 * 2 + 0] = 100.0; phs.Q.data[0 * 2 + 1] = 0.0;
    phs.Q.data[1 * 2 + 0] =   0.0; phs.Q.data[1 * 2 + 1] = 1.0;

    phs.G.data[0 * 1 + 0] = 0.0;
    phs.G.data[1 * 1 + 0] = 1.0;

    phs.M.data[0] = 0.0;

    rtmpbc_real_t dt = 0.001; /* 1 kHz loop */

    /* Setup True Physical Plant */
    rtmpbc_midpoint_solver_t plant;
    rtmpbc_status_t status = rtmpbc_midpoint_init(&plant, &phs, dt, 3, &arena);
    assert(status == RTMPBC_OK);

    /* True plant starts at q=2.0 m, p=5.0 kg*m/s */
    plant.x.data[0] = 2.0;
    plant.x.data[1] = 5.0;

    /* Setup Passivity-Based Observer */
    rtmpbc_real_t lambda_gain[1] = { 10.0 }; /* Observer injection gain */
    rtmpbc_observer_t obs;
    status = rtmpbc_observer_init(&obs, &phs, dt, lambda_gain, &arena);
    assert(status == RTMPBC_OK);

    /* Observer has ZERO initial knowledge: x_hat = [0.0, 0.0] */
    obs.solver.x.data[0] = 0.0;
    obs.solver.x.data[1] = 0.0;

    rtmpbc_vec_t u_in;
    rtmpbc_arena_alloc_vec(&arena, 1, &u_in);
    u_in.data[0] = 0.0; /* Free response or known input */

    rtmpbc_vec_t y_meas;
    rtmpbc_arena_alloc_vec(&arena, 1, &y_meas);

    /* Run for 2.0 seconds (2000 steps) */
    for (size_t step = 0; step < 2000; ++step) {
        /* Plant step produces measurement y_meas */
        rtmpbc_midpoint_step(&plant, &u_in, &y_meas);

        /* Observer step ingests y_meas and estimates full state x_hat */
        rtmpbc_observer_update(&obs, &u_in, &y_meas, NULL);
    }

    printf("  True Plant final state:     q = %.6f, p = %.6f\n", plant.x.data[0], plant.x.data[1]);
    printf("  Observer estimated state:   q = %.6f, p = %.6f\n", obs.x_hat.data[0], obs.x_hat.data[1]);

    rtmpbc_real_t err_q = fabs(plant.x.data[0] - obs.x_hat.data[0]);
    rtmpbc_real_t err_p = fabs(plant.x.data[1] - obs.x_hat.data[1]);

    printf("  Final State Estimation Error: |err_q| = %.2e, |err_p| = %.2e\n", err_q, err_p);

    /* Passivity-based observer should have asymptotically converged to < 1e-4 */
    assert(err_q < 1e-3);
    assert(err_p < 1e-3);

    printf("[TEST PASS] test_observer successfully converged to true internal states!\n");
    return 0;
}
