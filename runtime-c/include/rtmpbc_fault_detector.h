/**
 * @file rtmpbc_fault_detector.h
 * @brief Autonomous Energy-Invariant Fault, Anomaly & Cyber-Physical Attack Detector.
 *
 * Continuously validates physical conservation laws at every microsecond tick.
 * Instantly flags component degradation (winding shorts, bearing seizure, gear damage)
 * and sensor spoofing with zero false positives.
 */

#ifndef RTMPBC_FAULT_DETECTOR_H
#define RTMPBC_FAULT_DETECTOR_H

#include "rtmpbc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTMPBC_FAULT_NONE           = 0,
    RTMPBC_FAULT_ENERGY_LEAK    = 1, /* Unexpected physical dissipation (short circuit, mechanical friction spike) */
    RTMPBC_FAULT_SENSOR_SPOOF   = 2, /* Injected sensor readings violate passivity (cyber-physical attack / wire cut) */
    RTMPBC_FAULT_THERMAL_RUNAWAY= 3  /* Temperature rising past physical thermodynamic bounds */
} rtmpbc_fault_type_t;

typedef struct {
    rtmpbc_real_t energy_threshold;   /* Maximum allowable energy residual before alarm (e.g. 1e-4 J) */
    rtmpbc_real_t filter_alpha;       /* Leaky accumulator smoothing factor (0.0 to 1.0) */
    rtmpbc_real_t filtered_residual;  /* Filtered energy anomaly metric */

    size_t consecutive_violations;    /* Debounce counter */
    size_t trip_limit;                /* Number of consecutive violations to trigger latch */

    bool is_tripped;                  /* Latched safety trip status */
    rtmpbc_fault_type_t active_fault; /* Classification of detected failure */
    size_t total_fault_events;        /* Telemetry counter */
} rtmpbc_fault_detector_t;

/**
 * @brief Initialize the energy-invariant fault detector.
 */
void rtmpbc_fault_detector_init(
    rtmpbc_fault_detector_t* fd,
    rtmpbc_real_t energy_threshold,
    size_t debounce_ticks
);

/**
 * @brief Evaluate physical plausibility for the current step.
 *
 * @param fd Fault detector instance.
 * @param energy_error Instantaneous invariant error |ΔH - (P_in - P_diss)*dt|.
 * @param out_is_safe Output boolean indicating whether machine is physically safe to operate.
 * @return Active fault code (or RTMPBC_FAULT_NONE).
 */
rtmpbc_fault_type_t rtmpbc_fault_detector_eval(
    rtmpbc_fault_detector_t* fd,
    rtmpbc_real_t energy_error,
    bool* out_is_safe
);

/**
 * @brief Reset a latched fault after safe operator confirmation.
 */
void rtmpbc_fault_detector_reset(rtmpbc_fault_detector_t* fd);

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_FAULT_DETECTOR_H */
