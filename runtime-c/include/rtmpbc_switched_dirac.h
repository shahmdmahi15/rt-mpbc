/**
 * @file rtmpbc_switched_dirac.h
 * @brief Dynamic Switched Dirac Morphism Engine for ideal power electronics & friction.
 *
 * Models ideal switches (MOSFETs, IGBTs, diodes, clutches, stick-slip friction)
 * in O(1) time (1 CPU clock cycle) without introducing artificial numerical stiffness.
 */

#ifndef RTMPBC_SWITCHED_DIRAC_H
#define RTMPBC_SWITCHED_DIRAC_H

#include "rtmpbc_midpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t n_states;
    size_t n_inputs;
    size_t n_switches;

    /* Pre-factored Dirac topology modes */
    rtmpbc_mat_t* J_modes; /* Array of 2^n_switches skew-symmetric matrices */
    rtmpbc_mat_t* R_modes; /* Array of 2^n_switches dissipation matrices */
    rtmpbc_mat_t Q;        /* Common energy storage matrix */
    rtmpbc_mat_t G;        /* Common input matrix */

    uint32_t active_mode_mask; /* Bitmask representing current switch states */

    rtmpbc_midpoint_solver_t solver;
} rtmpbc_switched_dirac_t;

/**
 * @brief Initialize a switched Dirac system with pre-compiled topological modes.
 */
rtmpbc_status_t rtmpbc_switched_dirac_init(
    rtmpbc_switched_dirac_t* sys,
    size_t n_states,
    size_t n_inputs,
    size_t n_switches,
    rtmpbc_real_t dt,
    rtmpbc_arena_t* arena
);

/**
 * @brief Set switch boolean states in O(1) time without matrix inversion.
 *
 * @param sys Switched Dirac system.
 * @param switch_idx Index of switch (0 to n_switches - 1).
 * @param state True for ON (conducting, e=0), False for OFF (blocking, f=0).
 */
static inline void rtmpbc_switched_dirac_set(rtmpbc_switched_dirac_t* sys, size_t switch_idx, bool state) {
    if (state) {
        sys->active_mode_mask |= (1u << switch_idx);
    } else {
        sys->active_mode_mask &= ~(1u << switch_idx);
    }
    /* Instantaneous pointer swap: O(1) mode morphism */
    sys->solver.phs->J = sys->J_modes[sys->active_mode_mask];
    sys->solver.phs->R = sys->R_modes[sys->active_mode_mask];
}

/**
 * @brief Execute one real-time switched Dirac step.
 */
static inline rtmpbc_status_t rtmpbc_switched_dirac_step(
    rtmpbc_switched_dirac_t* sys,
    const rtmpbc_vec_t* u,
    rtmpbc_vec_t* y
) {
    return rtmpbc_midpoint_step(&sys->solver, u, y);
}

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_SWITCHED_DIRAC_H */
