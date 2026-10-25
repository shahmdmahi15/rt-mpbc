/**
 * @file rtmpbc_nonlinear.c
 * @brief Implementation of González Discrete Gradient Solver for arbitrary nonlinear Hamiltonians.
 */

#include "rtmpbc_nonlinear.h"
#include "rtmpbc_simd.h"
#include <math.h>
#include <string.h>

rtmpbc_status_t rtmpbc_nonlinear_init(
    rtmpbc_nonlinear_solver_t* solver,
    size_t n_states,
    size_t n_inputs,
    rtmpbc_real_t dt,
    rtmpbc_hamiltonian_fn fn,
    void* user_data,
    rtmpbc_arena_t* arena
) {
    if (!solver || !fn || !arena || dt <= 0.0 || n_states == 0) {
        return RTMPBC_ERR_NULL_PTR;
    }

    solver->n_states = n_states;
    solver->n_inputs = n_inputs;
    solver->dt = dt;
    solver->max_iters = 4; /* Deterministic bounded iterations */
    solver->tolerance = RTMPBC_EPSILON;
    solver->hamiltonian_eval = fn;
    solver->user_data = user_data;

    if (rtmpbc_arena_alloc_mat(arena, n_states, n_states, &solver->J) != RTMPBC_OK ||
        rtmpbc_arena_alloc_mat(arena, n_states, n_states, &solver->R) != RTMPBC_OK ||
        rtmpbc_arena_alloc_mat(arena, n_states, n_inputs, &solver->G) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n_states, &solver->x) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n_states, &solver->x_next) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n_states, &solver->x_mid) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n_states, &solver->delta_x) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n_states, &solver->grad_mid) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n_states, &solver->disc_grad) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n_states, &solver->residual) != RTMPBC_OK) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    memset(solver->x.data, 0, n_states * sizeof(rtmpbc_real_t));
    memset(solver->x_next.data, 0, n_states * sizeof(rtmpbc_real_t));

    solver->last_energy = 0.0;
    solver->next_energy = 0.0;
    solver->power_dissipated = 0.0;
    solver->energy_error = 0.0;

    return RTMPBC_OK;
}

void rtmpbc_gonzalez_discrete_gradient(
    rtmpbc_nonlinear_solver_t* solver,
    const rtmpbc_real_t* x_k,
    const rtmpbc_real_t* x_k1,
    rtmpbc_real_t* disc_grad_out
) {
    size_t n = solver->n_states;

    for (size_t i = 0; i < n; ++i) {
        solver->x_mid.data[i] = 0.5 * (x_k[i] + x_k1[i]);
        solver->delta_x.data[i] = x_k1[i] - x_k[i];
    }

    /* Evaluate H(x_mid) and ∇H(x_mid) */
    solver->hamiltonian_eval(solver->x_mid.data, solver->grad_mid.data, solver->user_data);

    /* Evaluate scalar energies H(x_k) and H(x_k1) */
    rtmpbc_real_t H_k = solver->hamiltonian_eval(x_k, NULL, solver->user_data);
    rtmpbc_real_t H_k1 = solver->hamiltonian_eval(x_k1, NULL, solver->user_data);

    rtmpbc_real_t delta_norm_sq = rtmpbc_simd_dot(solver->delta_x.data, solver->delta_x.data, n);
    rtmpbc_real_t grad_dot_delta = rtmpbc_simd_dot(solver->grad_mid.data, solver->delta_x.data, n);

    if (delta_norm_sq < 1e-14) {
        /* As delta_x -> 0, discrete gradient converges smoothly to standard gradient ∇H(x_mid) */
        for (size_t i = 0; i < n; ++i) {
            disc_grad_out[i] = solver->grad_mid.data[i];
        }
    } else {
        /* Exact González Discrete Gradient formula */
        rtmpbc_real_t scalar_factor = (H_k1 - H_k - grad_dot_delta) / delta_norm_sq;
        for (size_t i = 0; i < n; ++i) {
            disc_grad_out[i] = solver->grad_mid.data[i] + scalar_factor * solver->delta_x.data[i];
        }
    }
}

rtmpbc_status_t rtmpbc_nonlinear_step(
    rtmpbc_nonlinear_solver_t* solver,
    const rtmpbc_vec_t* u_input,
    rtmpbc_vec_t* y_output
) {
    if (!solver) {
        return RTMPBC_ERR_NULL_PTR;
    }

    size_t n = solver->n_states;
    size_t m = solver->n_inputs;
    rtmpbc_real_t dt = solver->dt;

    solver->last_energy = solver->hamiltonian_eval(solver->x.data, NULL, solver->user_data);

    /* Predictor: x_next = x_k */
    for (size_t i = 0; i < n; ++i) {
        solver->x_next.data[i] = solver->x.data[i];
    }

    /* Fixed-iteration Picard / Newton contraction loop for hard real-time determinism */
    for (size_t iter = 0; iter < solver->max_iters; ++iter) {
        rtmpbc_gonzalez_discrete_gradient(solver, solver->x.data, solver->x_next.data, solver->disc_grad.data);

        for (size_t i = 0; i < n; ++i) {
            rtmpbc_real_t f_phs_i = 0.0;
            for (size_t j = 0; j < n; ++j) {
                rtmpbc_real_t j_minus_r = solver->J.data[i * n + j] - solver->R.data[i * n + j];
                f_phs_i += j_minus_r * solver->disc_grad.data[j];
            }

            if (u_input && u_input->data && m > 0) {
                for (size_t p = 0; p < m; ++p) {
                    f_phs_i += solver->G.data[i * m + p] * u_input->data[p];
                }
            }

            /* Corrector step */
            solver->x_next.data[i] = solver->x.data[i] + dt * f_phs_i;
        }
    }

    /* Final discrete gradient evaluation */
    rtmpbc_gonzalez_discrete_gradient(solver, solver->x.data, solver->x_next.data, solver->disc_grad.data);

    /* Compute output y = G^T * disc_grad */
    rtmpbc_real_t power_in = 0.0;
    if (y_output && y_output->data && m > 0) {
        for (size_t p = 0; p < m; ++p) {
            rtmpbc_real_t y_p = 0.0;
            for (size_t i = 0; i < n; ++i) {
                y_p += solver->G.data[i * m + p] * solver->disc_grad.data[i];
            }
            y_output->data[p] = y_p;
            if (u_input && u_input->data) {
                power_in += u_input->data[p] * y_p;
            }
        }
    }

    /* Compute dissipated power P_diss = disc_grad^T * R * disc_grad */
    solver->power_dissipated = 0.0;
    for (size_t i = 0; i < n; ++i) {
        rtmpbc_real_t r_grad_i = 0.0;
        for (size_t j = 0; j < n; ++j) {
            r_grad_i += solver->R.data[i * n + j] * solver->disc_grad.data[j];
        }
        solver->power_dissipated += solver->disc_grad.data[i] * r_grad_i;
    }

    /* Commit state */
    memcpy(solver->x.data, solver->x_next.data, n * sizeof(rtmpbc_real_t));
    solver->next_energy = solver->hamiltonian_eval(solver->x.data, NULL, solver->user_data);

    /* Invariant verification */
    rtmpbc_real_t delta_H = solver->next_energy - solver->last_energy;
    rtmpbc_real_t expected_delta_H = (power_in - solver->power_dissipated) * dt;
    solver->energy_error = fabs(delta_H - expected_delta_H);

    return RTMPBC_OK;
}
