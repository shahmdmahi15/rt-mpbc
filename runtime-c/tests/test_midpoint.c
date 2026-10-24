#include "rtmpbc.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define ARENA_BUF_SIZE 16384

int main(void) {
    printf("[TEST] Running test_midpoint (Harmonic LC / Mass-Spring Oscillator)...\n");

    RTMPBC_ALIGN64 static uint8_t raw_buffer[ARENA_BUF_SIZE];
    rtmpbc_arena_t arena;
    rtmpbc_arena_init(&arena, raw_buffer, ARENA_BUF_SIZE);

    /* 2-State System:
     * State x = [q, p]^T (Displacement q [m], Momentum p [kg*m/s])
     * Mass m = 2.0 kg, Spring k = 50.0 N/m
     * H(q, p) = 1/2 * k * q^2 + 1/2 * (1/m) * p^2
     *
     * Q = diag(k, 1/m) = diag(50.0, 0.5)
     * J = [[ 0,  1],
     *      [-1,  0]]
     * R = [[ 0,  0],
     *      [ 0,  0]] (Lossless, pure Hamiltonian)
     */
    rtmpbc_phs_linear_t phs;
    phs.n_states = 2;
    phs.n_inputs = 0;

    rtmpbc_arena_alloc_mat(&arena, 2, 2, &phs.J);
    rtmpbc_arena_alloc_mat(&arena, 2, 2, &phs.R);
    rtmpbc_arena_alloc_mat(&arena, 2, 2, &phs.Q);
    phs.G.data = NULL; phs.G.rows = 2; phs.G.cols = 0;
    phs.M.data = NULL; phs.M.rows = 0; phs.M.cols = 0;

    /* J matrix (skew-symmetric) */
    phs.J.data[0 * 2 + 0] =  0.0; phs.J.data[0 * 2 + 1] =  1.0;
    phs.J.data[1 * 2 + 0] = -1.0; phs.J.data[1 * 2 + 1] =  0.0;

    /* R matrix (zero dissipation) */
    phs.R.data[0 * 2 + 0] = 0.0; phs.R.data[0 * 2 + 1] = 0.0;
    phs.R.data[1 * 2 + 0] = 0.0; phs.R.data[1 * 2 + 1] = 0.0;

    /* Q matrix: k = 50.0, 1/m = 0.5 */
    phs.Q.data[0 * 2 + 0] = 50.0; phs.Q.data[0 * 2 + 1] = 0.0;
    phs.Q.data[1 * 2 + 0] =  0.0; phs.Q.data[1 * 2 + 1] = 0.5;

    /* Time step dt = 0.005s (200 Hz), run 100,000 steps (500 seconds of physical time) */
    rtmpbc_real_t dt = 0.005;
    size_t total_steps = 100000;

    rtmpbc_midpoint_solver_t solver;
    rtmpbc_status_t status = rtmpbc_midpoint_init(&solver, &phs, dt, 3, &arena);
    assert(status == RTMPBC_OK);

    /* Initial condition: q(0) = 1.0 m, p(0) = 0.0 kg*m/s */
    solver.x.data[0] = 1.0;
    solver.x.data[1] = 0.0;

    rtmpbc_real_t H_initial = rtmpbc_midpoint_hamiltonian(&phs, &solver.x);
    printf("  Initial Energy H(0) = %.10f J\n", H_initial);
    assert(fabs(H_initial - 25.0) < 1e-10); /* 1/2 * 50 * 1^2 = 25.0 J */

    rtmpbc_real_t max_energy_drift = 0.0;

    for (size_t step = 0; step < total_steps; ++step) {
        status = rtmpbc_midpoint_step(&solver, NULL, NULL);
        assert(status == RTMPBC_OK);

        rtmpbc_real_t H_current = solver.next_energy;
        rtmpbc_real_t drift = fabs(H_current - H_initial);
        if (drift > max_energy_drift) {
            max_energy_drift = drift;
        }

        /* Verify step-by-step invariant */
        assert(solver.energy_error < 1e-10);
    }

    rtmpbc_real_t H_final = solver.next_energy;
    printf("  Final Energy after %zu steps H(%zu) = %.10f J\n", total_steps, total_steps, H_final);
    printf("  Max Energy Drift across all 100,000 steps = %.2e J\n", max_energy_drift);

    /* The symplectic implicit midpoint method strictly conserves energy within machine precision accumulation */
    assert(max_energy_drift < 1e-8);

    printf("[TEST PASS] test_midpoint strictly conserved energy over %zu steps!\n", total_steps);
    return 0;
}
