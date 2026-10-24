/**
 * @file rtmpbc_observer.h
 * @brief Passivity-Based Port-Hamiltonian State Observer (PH-PBO).
 *
 * Provides a real-time "Virtual Sensor Array" that reconstructs hidden internal
 * physical states (stress, torque, thermal distribution) from surface sensor measurements.
 *
 * Guarantees unconditional asymptotic Lyapunov stability without computing
 * Riccati equations or dynamic covariance matrices.
 */

#ifndef RTMPBC_OBSERVER_H
#define RTMPBC_OBSERVER_H

#include "rtmpbc_midpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    rtmpbc_phs_linear_t* phs;
    rtmpbc_real_t dt;
    rtmpbc_mat_t Lambda;              /* m x m Observer gain matrix (Lambda > 0) */
    rtmpbc_mat_t R_obs;               /* n x n Augmented dissipation matrix: R + G * Lambda * G^T */

    rtmpbc_midpoint_solver_t solver;  /* Internal deterministic midpoint solver for observer state */
    rtmpbc_vec_t x_hat;               /* Estimated physical state vector (n) */
    rtmpbc_vec_t y_hat;               /* Predicted measurement output vector (m) */
    rtmpbc_vec_t error_y;             /* Innovation/residual: y_meas - y_hat (m) */

    /* Scratch buffers */
    rtmpbc_vec_t u_effective;         /* Effective input vector fed to solver (m) */
} rtmpbc_observer_t;

/**
 * @brief Initialize the passivity-based observer from arena memory.
 *
 * @param obs Observer structure.
 * @param phs Pointer to physical model.
 * @param dt Observer update time step.
 * @param lambda_gains Diagonal gains for the m measurement ports (array of length m).
 * @param arena Static allocation arena.
 */
rtmpbc_status_t rtmpbc_observer_init(
    rtmpbc_observer_t* obs,
    rtmpbc_phs_linear_t* phs,
    rtmpbc_real_t dt,
    const rtmpbc_real_t* lambda_gains,
    rtmpbc_arena_t* arena
);

/**
 * @brief Execute one estimation update tick.
 *
 * Ingests external input u (e.g. motor PWM voltage) and surface measurements y_meas (e.g. motor phase current),
 * and updates the internal state estimate x_hat (e.g. internal shaft torque and rotor speed).
 *
 * @param obs Observer structure.
 * @param u_input Known system inputs (length m). Can be NULL if m=0.
 * @param y_meas Surface sensor measurements (length m).
 * @param out_x_hat Optional destination pointer to receive state estimate (length n). Can be NULL.
 */
rtmpbc_status_t rtmpbc_observer_update(
    rtmpbc_observer_t* obs,
    const rtmpbc_vec_t* u_input,
    const rtmpbc_vec_t* y_meas,
    rtmpbc_vec_t* out_x_hat
);

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_OBSERVER_H */
