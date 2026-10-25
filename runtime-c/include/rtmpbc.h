/**
 * @file rtmpbc.h
 * @brief Master header for RT-MPBC (Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver).
 *
 * Bare-metal, zero-dependency, malloc-free C99/C11 multi-physics engine.
 */

#ifndef RTMPBC_H
#define RTMPBC_H

#include "rtmpbc_types.h"
#include "rtmpbc_fixed_arena.h"
#include "rtmpbc_simd.h"
#include "rtmpbc_midpoint.h"
#include "rtmpbc_nonlinear.h"
#include "rtmpbc_switched_dirac.h"
#include "rtmpbc_multirate.h"
#include "rtmpbc_observer.h"
#include "rtmpbc_fault_detector.h"

#define RTMPBC_VERSION_MAJOR 1
#define RTMPBC_VERSION_MINOR 1
#define RTMPBC_VERSION_PATCH 0

#endif /* RTMPBC_H */
