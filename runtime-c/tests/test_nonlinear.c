#include "rtmpbc.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define ARENA_BUF_SIZE 32768

/* Nonlinear Duffing Oscillator:
 * H(q, p) = 1/2 * p^2 + 1/2 * k * q^2 + 1/4 * beta * q^4
 * ∇H = [k*q + beta*q^3, p]^T
 */
typedef struct {
    rtmpbc_real_t k;
    rtmpbc_real_t beta;
} duffing_params_t;

static rtmpbc_real_t duffing_hamiltonian(
    const rtmpbc_real_t* x,
    rtmpbc_real_t* grad_out,
    void* user_data
) {
    duffing_params_t* p = (duffing_params_t*)user_data;
    rtmpbc_real_t q = x[0];
    rtmpbc_real_t p_mom = x[1];

    if (grad_out) {
        grad_out[0] = p->k * q + p->beta * q * q * q; /* Nonlinear force */
        grad_out[1] = p_mom;                          /* Velocity */
    }

    return 0.5 * p_mom * p_mom + 0.5 * p->k * q * q + 0.25 * p->beta * q * q * q * q;
}

int main(void) {
    printf("[TEST] Running test_nonlinear (González Discrete Gradient on Duffing Oscillator)...\n");

    RTMPBC_ALIGN64 static uint8_t raw_buffer[ARENA_BUF_SIZE];
    rtmpbc_arena_t arena;
    rtmpbc_arena_init(&arena, raw_buffer, ARENA_BUF_SIZE);

    duffing_params_t params = { .k = 10.0, .beta = 100.0 }; /* Strong nonlinearity beta = 100 */

    rtmpbc_nonlinear_solver_t solver;
    rtmpbc_real_t dt = 0.002;
    rtmpbc_status_t status = rtmpbc_nonlinear_init(&solver, 2, 0, dt, duffing_hamiltonian, &params, &arena);
    assert(status == RTMPBC_OK);

    /* Skew-symmetric J matrix: [[0, 1], [-1, 0]] */
    solver.J.data[0 * 2 + 0] =  0.0; solver.J.data[0 * 2 + 1] =  1.0;
    solver.J.data[1 * 2 + 0] = -1.0; solver.J.data[1 * 2 + 1] =  0.0;

    /* Conservative (zero dissipation) */
    solver.R.data[0 * 2 + 0] = 0.0; solver.R.data[0 * 2 + 1] = 0.0;
    solver.R.data[1 * 2 + 0] = 0.0; solver.R.data[1 * 2 + 1] = 0.0;

    /* Large displacement initial condition: q=1.5 m, p=0 */
    solver.x.data[0] = 1.5;
    solver.x.data[1] = 0.0;

    rtmpbc_real_t H_initial = duffing_hamiltonian(solver.x.data, NULL, &params);
    printf("  Initial Energy H(0) = %.10f J\n", H_initial);

    size_t total_steps = 50000;
    rtmpbc_real_t max_energy_drift = 0.0;

    for (size_t step = 0; step < total_steps; ++step) {
        status = rtmpbc_nonlinear_step(&solver, NULL, NULL);
        assert(status == RTMPBC_OK);

        rtmpbc_real_t H_curr = solver.next_energy;
        rtmpbc_real_t drift = fabs(H_curr - H_initial);
        if (drift > max_energy_drift) {
            max_energy_drift = drift;
        }

        if (solver.energy_error > 1e-4) {
            printf("  Step %zu: energy_error = %.6e, drift = %.6e\n", step, solver.energy_error, drift);
        }
    }

    printf("  Final Energy after %zu steps H = %.10f J\n", total_steps, solver.next_energy);
    printf("  Max Energy Drift on Strong Nonlinearity = %.2e J\n", max_energy_drift);

    /* González discrete gradient maintains bounded energy within 0.001 J across 50,000 violent cycles */
    assert(max_energy_drift < 1e-3);

    printf("[TEST PASS] test_nonlinear González Discrete Gradient strictly conserved non-quadratic energy!\n");
    return 0;
}
