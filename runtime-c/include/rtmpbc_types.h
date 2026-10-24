/**
 * @file rtmpbc_types.h
 * @brief Fundamental types, physical domain identifiers, and status codes for RT-MPBC.
 *
 * Designed for zero-dependency, bare-metal C99/C11 execution.
 * Zero dynamic memory allocations (malloc-free).
 */

#ifndef RTMPBC_TYPES_H
#define RTMPBC_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Floating-point precision selector (default: double, can be overridden with -DRTMPBC_USE_FLOAT) */
#ifdef RTMPBC_USE_FLOAT
typedef float rtmpbc_real_t;
#define RTMPBC_EPSILON 1e-6f
#else
typedef double rtmpbc_real_t;
#define RTMPBC_EPSILON 1e-12
#endif

/* Cache-line alignment macro (64-byte alignment for modern ARM/x86 cache lines) */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#include <stdalign.h>
#define RTMPBC_ALIGN64 alignas(64)
#elif defined(__GNUC__) || defined(__clang__)
#define RTMPBC_ALIGN64 __attribute__((aligned(64)))
#elif defined(_MSC_VER)
#define RTMPBC_ALIGN64 __declspec(align(64))
#else
#define RTMPBC_ALIGN64
#endif

/* Status and error codes */
typedef enum {
    RTMPBC_OK                   =  0,
    RTMPBC_ERR_NULL_PTR         = -1,
    RTMPBC_ERR_OUT_OF_MEMORY    = -2,
    RTMPBC_ERR_ALIGNMENT        = -3,
    RTMPBC_ERR_CONVERGENCE      = -4,
    RTMPBC_ERR_SINGULAR         = -5,
    RTMPBC_ERR_INVALID_DIM      = -6,
    RTMPBC_ERR_CAUSALITY_FAULT  = -7,
    RTMPBC_ERR_ENERGY_VIOLATION = -8
} rtmpbc_status_t;

/* Physical domain taxonomy */
typedef enum {
    RTMPBC_DOMAIN_MECHANICAL_TRANSLATIONAL = 0, /* e = Force [N],          f = Velocity [m/s]         */
    RTMPBC_DOMAIN_MECHANICAL_ROTATIONAL    = 1, /* e = Torque [N*m],       f = Angular Vel [rad/s]    */
    RTMPBC_DOMAIN_ELECTRICAL               = 2, /* e = Voltage [V],        f = Current [A]            */
    RTMPBC_DOMAIN_HYDRAULIC                = 3, /* e = Pressure [Pa],      f = Volumetric Flow [m^3/s]*/
    RTMPBC_DOMAIN_THERMAL                  = 4  /* e = Temperature [K],    f = Entropy Flow [W/K]     */
} rtmpbc_domain_t;

/* Bond Graph causality specification */
typedef enum {
    RTMPBC_CAUSALITY_UNASSIGNED = 0,
    RTMPBC_CAUSALITY_EFFORT_OUT = 1, /* Stroke at end of bond: effort is directed toward target */
    RTMPBC_CAUSALITY_FLOW_OUT   = 2  /* Effort directed toward source (target determines flow)  */
} rtmpbc_causality_t;

/* Static Vector view over contiguous memory */
typedef struct {
    rtmpbc_real_t* data;
    size_t length;
} rtmpbc_vec_t;

/* Static Matrix view over row-major contiguous memory */
typedef struct {
    rtmpbc_real_t* data;
    size_t rows;
    size_t cols;
} rtmpbc_mat_t;

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_TYPES_H */
