/**
 * @file rtmpbc_nonlinear.h
 * @brief González Discrete Gradient Solver for arbitrary nonlinear Hamiltonians.
 *
 * Guarantees exact energy conservation and thermodynamic consistency for
 * non-quadratic energy functions (magnetic saturation, nonlinear tissue elasticity,
 * aerodynamic drag, nonlinear fluid orifice flow).
 */

#ifndef RTMPBC_NONLINEAR_H
#define RTMPBC_NONLINEAR_H

#include "rtmpbc_types.h"
#include "rtmpbc_fixed_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief User-defined nonlinear Hamiltonian callback.
 *
 * @param x State vector of length n.
 * @param grad_out Output gradient vector ∇H(x) of length n.
 * @param user_data User context pointer.
 * @return Total energy scalar H(x).
 */
typedef rtmpbc_real_t (*rtmpbc_hamiltonian_fn)(
    const rtmpbc_real_t* x,
    rtmpbc_real_t* grad_out,
    void* user_data
);

typedef struct {
    size_t n_states;
    size_t n_inputs;
    rtmpbc_real_t dt;
    size_t max_iters;
    rtmpbc_real_t tolerance;

    rtmpbc_hamiltonian_fn hamiltonian_eval;
    void* user_data;

    rtmpbc_mat_t J; /* n x n Skew-symmetric */
    rtmpbc_mat_t R; /* n x n Dissipative */
    rtmpbc_mat_t G; /* n x m Input */

    /* Scratch memory allocated from arena */
    rtmpbc_vec_t x;
    rtmpbc_vec_t x_next;
    rtmpbc_vec_t x_mid;
    rtmpbc_vec_t delta_x;
    rtmpbc_vec_t grad_mid;
    rtmpbc_vec_t disc_grad;
    rtmpbc_vec_t residual;

    /* Telemetry */
    rtmpbc_real_t last_energy;
    rtmpbc_real_t next_energy;
    rtmpbc_real_t power_dissipated;
    rtmpbc_real_t energy_error;
} rtmpbc_nonlinear_solver_t;

/**
 * @brief Initialize the nonlinear González discrete gradient solver.
 */
rtmpbc_status_t rtmpbc_nonlinear_init(
    rtmpbc_nonlinear_solver_t* solver,
    size_t n_states,
    size_t n_inputs,
    rtmpbc_real_t dt,
    rtmpbc_hamiltonian_fn fn,
    void* user_data,
    rtmpbc_arena_t* arena
);

/**
 * @brief Compute the González Discrete Gradient ∇̄H(x_k, x_{k+1}).
 */
void rtmpbc_gonzalez_discrete_gradient(
    rtmpbc_nonlinear_solver_t* solver,
    const rtmpbc_real_t* x_k,
    const rtmpbc_real_t* x_k1,
    rtmpbc_real_t* disc_grad_out
);

/**
 * @brief Execute one hard real-time nonlinear step.
 */
rtmpbc_status_t rtmpbc_nonlinear_step(
    rtmpbc_nonlinear_solver_t* solver,
    const rtmpbc_vec_t* u_input,
    rtmpbc_vec_t* y_output
);

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_NONLINEAR_H */
