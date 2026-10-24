/**
 * @file rtmpbc_fixed_arena.h
 * @brief Deterministic, zero-allocation memory arena for hard real-time execution.
 *
 * Provides strictly 64-byte aligned slices from a statically allocated buffer.
 * Completely eliminates malloc(), free(), page faults, and heap fragmentation.
 */

#ifndef RTMPBC_FIXED_ARENA_H
#define RTMPBC_FIXED_ARENA_H

#include "rtmpbc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
    size_t high_water_mark;
} rtmpbc_arena_t;

/**
 * @brief Initialize an arena with a caller-provided raw buffer.
 *
 * @param arena Pointer to the arena structure.
 * @param buffer Pointer to raw memory (should preferably be 64-byte aligned).
 * @param capacity Size of the buffer in bytes.
 * @return RTMPBC_OK or error code.
 */
rtmpbc_status_t rtmpbc_arena_init(rtmpbc_arena_t* arena, void* buffer, size_t capacity);

/**
 * @brief Allocate a block of memory from the arena with custom alignment.
 *
 * @param arena Pointer to initialized arena.
 * @param size Number of bytes requested.
 * @param alignment Required alignment in bytes (must be power of two).
 * @return Pointer to aligned block, or NULL if capacity exceeded.
 */
void* rtmpbc_arena_alloc(rtmpbc_arena_t* arena, size_t size, size_t alignment);

/**
 * @brief Allocate an array of real_t values, guaranteed 64-byte cache-line aligned.
 *
 * @param arena Pointer to initialized arena.
 * @param count Number of real_t elements.
 * @return Pointer to real_t array, or NULL if capacity exceeded.
 */
rtmpbc_real_t* rtmpbc_arena_alloc_real(rtmpbc_arena_t* arena, size_t count);

/**
 * @brief Allocate a static vector view from the arena.
 */
rtmpbc_status_t rtmpbc_arena_alloc_vec(rtmpbc_arena_t* arena, size_t length, rtmpbc_vec_t* out_vec);

/**
 * @brief Allocate a static 2D matrix view from the arena.
 */
rtmpbc_status_t rtmpbc_arena_alloc_mat(rtmpbc_arena_t* arena, size_t rows, size_t cols, rtmpbc_mat_t* out_mat);

/**
 * @brief Reset the arena allocation offset back to zero.
 * (High-water mark is preserved for telemetry).
 */
void rtmpbc_arena_reset(rtmpbc_arena_t* arena);

/**
 * @brief Get the current allocation watermark to enable scoped rollback.
 */
size_t rtmpbc_arena_mark(const rtmpbc_arena_t* arena);

/**
 * @brief Restore the arena allocation offset to a previously recorded mark.
 */
void rtmpbc_arena_restore(rtmpbc_arena_t* arena, size_t mark);

/**
 * @brief Query the peak memory usage observed in this arena.
 */
size_t rtmpbc_arena_high_water_mark(const rtmpbc_arena_t* arena);

/**
 * @brief Query remaining bytes available in the arena.
 */
size_t rtmpbc_arena_remaining(const rtmpbc_arena_t* arena);

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_FIXED_ARENA_H */
