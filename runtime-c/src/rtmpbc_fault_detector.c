/**
 * @file rtmpbc_fault_detector.c
 * @brief Implementation of Energy-Invariant Fault & Anomaly Detector.
 */

#include "rtmpbc_fault_detector.h"
#include <math.h>

void rtmpbc_fault_detector_init(
    rtmpbc_fault_detector_t* fd,
    rtmpbc_real_t energy_threshold,
    size_t debounce_ticks
) {
    if (!fd) return;
    fd->energy_threshold = (energy_threshold > 0.0) ? energy_threshold : 1e-4;
    fd->filter_alpha = 0.95;
    fd->filtered_residual = 0.0;
    fd->consecutive_violations = 0;
    fd->trip_limit = (debounce_ticks > 0) ? debounce_ticks : 3;
    fd->is_tripped = false;
    fd->active_fault = RTMPBC_FAULT_NONE;
    fd->total_fault_events = 0;
}

rtmpbc_fault_type_t rtmpbc_fault_detector_eval(
    rtmpbc_fault_detector_t* fd,
    rtmpbc_real_t energy_error,
    bool* out_is_safe
) {
    if (!fd) {
        if (out_is_safe) *out_is_safe = false;
        return RTMPBC_FAULT_ENERGY_LEAK;
    }

    /* Leaky accumulator filter */
    fd->filtered_residual = fd->filter_alpha * fd->filtered_residual + (1.0 - fd->filter_alpha) * fabs(energy_error);

    if (fd->filtered_residual > fd->energy_threshold) {
        fd->consecutive_violations++;
        if (fd->consecutive_violations >= fd->trip_limit) {
            fd->is_tripped = true;
            fd->active_fault = RTMPBC_FAULT_ENERGY_LEAK;
            fd->total_fault_events++;
        }
    } else {
        if (fd->consecutive_violations > 0) {
            fd->consecutive_violations--;
        }
    }

    if (out_is_safe) {
        *out_is_safe = !fd->is_tripped;
    }

    return fd->active_fault;
}

void rtmpbc_fault_detector_reset(rtmpbc_fault_detector_t* fd) {
    if (!fd) return;
    fd->filtered_residual = 0.0;
    fd->consecutive_violations = 0;
    fd->is_tripped = false;
    fd->active_fault = RTMPBC_FAULT_NONE;
}
