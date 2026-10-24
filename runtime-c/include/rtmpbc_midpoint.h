/**
 * @file rtmpbc_midpoint.h
 * @brief Energy-preserving Symplectic Implicit Midpoint solver for Port-Hamiltonian systems.
 *
 * Guarantees zero numerical energy drift and thermodynamic consistency.
 * Uses bounded-iteration Newton-Raphson for deterministic WCET in hard real-time systems.
 * Zero dynamic memory allocations.
 */

#ifndef RTMPBC_MIDPOINT_H
#define RTMPBC_MIDPOINT_H

#include "rtmpbc_types.h"
#include "rtmpbc_fixed_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Definition of a linear or linearized Port-Hamiltonian system.
 *
 * System form:
 *   dx/dt = (J - R) * Q * x + G * u
 *   y     = G^T * Q * x + M * u
 */
typedef struct {
    size_t n_states;  /* Dimension n of state vector x */
    size_t n_inputs;  /* Dimension m of input vector u */

    rtmpbc_mat_t J;   /* n x n Skew-symmetric interconnection matrix (J = -J^T) */
    rtmpbc_mat_t R;   /* n x n Dissipation matrix (R = R^T >= 0) */
    rtmpbc_mat_t Q;   /* n x n Symmetric positive-definite energy matrix (Q = Q^T > 0) */
    rtmpbc_mat_t G;   /* n x m Input coupling matrix */
    rtmpbc_mat_t M;   /* m x m Feedthrough matrix (often 0) */
} rtmpbc_phs_linear_t;

/**
 * @brief Runtime state and scratch memory for the Implicit Midpoint solver.
 */
typedef struct {
    rtmpbc_phs_linear_t* phs;
    rtmpbc_real_t dt;                 /* Time-step size [seconds] */
    size_t max_newton_iters;          /* Fixed maximum iterations for real-time determinism (default 3) */
    rtmpbc_real_t tolerance;          /* Convergence tolerance */

    /* Scratch memory allocated from arena */
    rtmpbc_vec_t x;                   /* Current state vector (n) */
    rtmpbc_vec_t x_next;              /* Candidate next state vector (n) */
    rtmpbc_vec_t x_mid;               /* Midpoint state (x + x_next)/2 */
    rtmpbc_vec_t grad_H;              /* Energy gradient Q * x_mid */
    rtmpbc_vec_t residual;            /* Solver residual F(x_next) */
    rtmpbc_mat_t A_mat;               /* LHS iteration matrix (n x n) */
    rtmpbc_mat_t LU_scratch;          /* In-place LU decomposition scratch (n x n) */
    int32_t*     pivot_scratch;       /* Pivot indices (n) */
    rtmpbc_vec_t b_vec;               /* RHS vector (n) */

    /* Telemetry and physical validation */
    rtmpbc_real_t last_energy;        /* H(x) at start of step */
    rtmpbc_real_t next_energy;        /* H(x_next) at end of step */
    rtmpbc_real_t power_dissipated;   /* Energy lost to resistance R during step */
    rtmpbc_real_t power_input;        /* Energy supplied by port u during step */
    rtmpbc_real_t energy_error;       /* Invariant residual: |ΔH - (P_in - P_diss)*dt| */
} rtmpbc_midpoint_solver_t;

/**
 * @brief Initialize the implicit midpoint solver from static arena memory.
 */
rtmpbc_status_t rtmpbc_midpoint_init(
    rtmpbc_midpoint_solver_t* solver,
    rtmpbc_phs_linear_t* phs,
    rtmpbc_real_t dt,
    size_t max_newton_iters,
    rtmpbc_arena_t* arena
);

/**
 * @brief Execute one hard real-time symplectic step.
 *
 * @param solver Pointer to initialized solver.
 * @param u_input Current input vector u(t + dt/2), dimension m. Can be NULL if m=0.
 * @param y_output Optional destination for conjugate output vector y(t + dt/2). Can be NULL.
 * @return RTMPBC_OK or error status.
 */
rtmpbc_status_t rtmpbc_midpoint_step(
    rtmpbc_midpoint_solver_t* solver,
    const rtmpbc_vec_t* u_input,
    rtmpbc_vec_t* y_output
);

/**
 * @brief Calculate the instantaneous Hamiltonian H(x) = 1/2 * x^T * Q * x.
 */
rtmpbc_real_t rtmpbc_midpoint_hamiltonian(const rtmpbc_phs_linear_t* phs, const rtmpbc_vec_t* x);

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_MIDPOINT_H */
