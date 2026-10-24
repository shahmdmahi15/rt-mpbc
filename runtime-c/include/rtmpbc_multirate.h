/**
 * @file rtmpbc_multirate.h
 * @brief Discrete Variational Multi-Rate Boundary Commutator.
 *
 * Couples high-frequency subsystems (e.g. 100 kHz electronics) to
 * low-frequency subsystems (e.g. 1 kHz mechanics / 10 Hz thermal)
 * while preserving exact discrete energy/momentum balance across the boundary.
 */

#ifndef RTMPBC_MULTIRATE_H
#define RTMPBC_MULTIRATE_H

#include "rtmpbc_midpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t sub_ratio;             /* Substep ratio N = dt_macro / dt_micro */
    size_t current_substep;       /* 0 to sub_ratio - 1 */

    rtmpbc_real_t dt_micro;       /* Fast loop time step */
    rtmpbc_real_t dt_macro;       /* Slow loop time step (sub_ratio * dt_micro) */

    size_t n_ports;               /* Number of coupled boundary ports */
    rtmpbc_real_t* impulse_accum; /* Integrated effort/momentum buffer: sum(e * dt_micro) */
    rtmpbc_real_t* last_macro_flow;/* Flow received from slow system held for fast steps */

    /* Energy tracking */
    rtmpbc_real_t energy_transferred_micro;
    rtmpbc_real_t energy_transferred_macro;
} rtmpbc_multirate_coupler_t;

/**
 * @brief Initialize a multi-rate boundary coupler from arena memory.
 */
rtmpbc_status_t rtmpbc_multirate_init(
    rtmpbc_multirate_coupler_t* coupler,
    size_t n_ports,
    size_t sub_ratio,
    rtmpbc_real_t dt_micro,
    rtmpbc_arena_t* arena
);

/**
 * @brief Fast subsystem deposits its instantaneous boundary effort and reads slow flow.
 *
 * Called once per micro-step.
 *
 * @param coupler Coupler context.
 * @param e_micro Effort vector from fast subsystem (length n_ports).
 * @param f_micro_out Output flow vector delivered to fast subsystem (length n_ports).
 */
rtmpbc_status_t rtmpbc_multirate_micro_step(
    rtmpbc_multirate_coupler_t* coupler,
    const rtmpbc_real_t* e_micro,
    rtmpbc_real_t* f_micro_out
);

/**
 * @brief Slow subsystem reads the accumulated macro-effort and delivers its reaction flow.
 *
 * Called once every sub_ratio micro-steps.
 *
 * @param coupler Coupler context.
 * @param e_macro_out Mean effort vector delivered to slow subsystem (length n_ports).
 * @param f_macro Flow vector from slow subsystem (length n_ports).
 */
rtmpbc_status_t rtmpbc_multirate_macro_step(
    rtmpbc_multirate_coupler_t* coupler,
    rtmpbc_real_t* e_macro_out,
    const rtmpbc_real_t* f_macro
);

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_MULTIRATE_H */
