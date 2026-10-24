/**
 * @file rtmpbc_observer.c
 * @brief Implementation of Passivity-Based Port-Hamiltonian State Observer (PH-PBO).
 */

#include "rtmpbc_observer.h"
#include <string.h>

rtmpbc_status_t rtmpbc_observer_init(
    rtmpbc_observer_t* obs,
    rtmpbc_phs_linear_t* phs,
    rtmpbc_real_t dt,
    const rtmpbc_real_t* lambda_gains,
    rtmpbc_arena_t* arena
) {
    if (!obs || !phs || !lambda_gains || !arena || dt <= 0.0) {
        return RTMPBC_ERR_NULL_PTR;
    }

    size_t n = phs->n_states;
    size_t m = phs->n_inputs;

    obs->phs = phs;
    obs->dt = dt;

    if (rtmpbc_arena_alloc_mat(arena, m, m, &obs->Lambda) != RTMPBC_OK ||
        rtmpbc_arena_alloc_mat(arena, n, n, &obs->R_obs) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n, &obs->x_hat) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, m, &obs->y_hat) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, m, &obs->error_y) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, m, &obs->u_effective) != RTMPBC_OK) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    /* Initialize Lambda matrix (diagonal) */
    memset(obs->Lambda.data, 0, m * m * sizeof(rtmpbc_real_t));
    for (size_t p = 0; p < m; ++p) {
        obs->Lambda.data[p * m + p] = lambda_gains[p];
    }

    /* Compute R_obs = R + G * Lambda * G^T */
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            rtmpbc_real_t g_lambda_gt = 0.0;
            for (size_t p = 0; p < m; ++p) {
                /* G[i, p] * Lambda[p, p] * G[j, p] */
                g_lambda_gt += phs->G.data[i * m + p] * lambda_gains[p] * phs->G.data[j * m + p];
            }
            obs->R_obs.data[i * n + j] = phs->R.data[i * n + j] + g_lambda_gt;
        }
    }

    /* Allocate shadow PHS on arena with augmented R_obs */
    rtmpbc_phs_linear_t* shadow_phs = (rtmpbc_phs_linear_t*)rtmpbc_arena_alloc(
        arena, sizeof(rtmpbc_phs_linear_t), sizeof(void*)
    );
    if (!shadow_phs) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    shadow_phs->n_states = n;
    shadow_phs->n_inputs = m;
    shadow_phs->J = phs->J;
    shadow_phs->R = obs->R_obs;
    shadow_phs->Q = phs->Q;
    shadow_phs->G = phs->G;
    shadow_phs->M = phs->M;

    /* Initialize internal midpoint solver for observer state */
    rtmpbc_status_t status = rtmpbc_midpoint_init(&obs->solver, shadow_phs, dt, 3, arena);
    if (status != RTMPBC_OK) {
        return status;
    }

    memset(obs->x_hat.data, 0, n * sizeof(rtmpbc_real_t));
    memset(obs->y_hat.data, 0, m * sizeof(rtmpbc_real_t));

    return RTMPBC_OK;
}

rtmpbc_status_t rtmpbc_observer_update(
    rtmpbc_observer_t* obs,
    const rtmpbc_vec_t* u_input,
    const rtmpbc_vec_t* y_meas,
    rtmpbc_vec_t* out_x_hat
) {
    if (!obs || !y_meas) {
        return RTMPBC_ERR_NULL_PTR;
    }

    size_t n = obs->phs->n_states;
    size_t m = obs->phs->n_inputs;

    /* Compute effective input: u_effective = u + Lambda * y_meas */
    for (size_t p = 0; p < m; ++p) {
        rtmpbc_real_t u_val = (u_input && u_input->data) ? u_input->data[p] : 0.0;
        obs->u_effective.data[p] = u_val + obs->Lambda.data[p * m + p] * y_meas->data[p];
    }

    /* Step the augmented Port-Hamiltonian system */
    rtmpbc_status_t status = rtmpbc_midpoint_step(&obs->solver, &obs->u_effective, &obs->y_hat);
    if (status != RTMPBC_OK) {
        return status;
    }

    /* Extract updated state estimate */
    memcpy(obs->x_hat.data, obs->solver.x.data, n * sizeof(rtmpbc_real_t));

    /* Calculate innovation: error_y = y_meas - y_hat */
    for (size_t p = 0; p < m; ++p) {
        obs->error_y.data[p] = y_meas->data[p] - obs->y_hat.data[p];
    }

    if (out_x_hat && out_x_hat->data) {
        memcpy(out_x_hat->data, obs->x_hat.data, n * sizeof(rtmpbc_real_t));
    }

    return RTMPBC_OK;
}
