/**
 * @file rtmpbc_fixed_arena.c
 * @brief Implementation of deterministic, zero-allocation memory arena.
 */

#include "rtmpbc_fixed_arena.h"

static inline uintptr_t align_forward(uintptr_t ptr, size_t alignment) {
    uintptr_t a = (uintptr_t)alignment;
    return (ptr + (a - 1)) & ~(a - 1);
}

rtmpbc_status_t rtmpbc_arena_init(rtmpbc_arena_t* arena, void* buffer, size_t capacity) {
    if (!arena || !buffer || capacity == 0) {
        return RTMPBC_ERR_NULL_PTR;
    }
    arena->buffer = (uint8_t*)buffer;
    arena->capacity = capacity;
    arena->offset = 0;
    arena->high_water_mark = 0;
    return RTMPBC_OK;
}

void* rtmpbc_arena_alloc(rtmpbc_arena_t* arena, size_t size, size_t alignment) {
    if (!arena || !arena->buffer || size == 0) {
        return NULL;
    }

    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        alignment = sizeof(void*);
    }

    uintptr_t current_ptr = (uintptr_t)(arena->buffer + arena->offset);
    uintptr_t aligned_ptr = align_forward(current_ptr, alignment);
    size_t padding = aligned_ptr - current_ptr;

    if (arena->offset + padding + size > arena->capacity) {
        return NULL; /* Out of memory in fixed buffer */
    }

    arena->offset += padding + size;
    if (arena->offset > arena->high_water_mark) {
        arena->high_water_mark = arena->offset;
    }

    return (void*)aligned_ptr;
}

rtmpbc_real_t* rtmpbc_arena_alloc_real(rtmpbc_arena_t* arena, size_t count) {
    if (!arena || count == 0) {
        return NULL;
    }
    /* Enforce 64-byte alignment for SIMD and cache-line efficiency */
    return (rtmpbc_real_t*)rtmpbc_arena_alloc(arena, count * sizeof(rtmpbc_real_t), 64);
}

rtmpbc_status_t rtmpbc_arena_alloc_vec(rtmpbc_arena_t* arena, size_t length, rtmpbc_vec_t* out_vec) {
    if (!arena || !out_vec) {
        return RTMPBC_ERR_NULL_PTR;
    }
    out_vec->data = rtmpbc_arena_alloc_real(arena, length);
    if (!out_vec->data && length > 0) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }
    out_vec->length = length;
    return RTMPBC_OK;
}

rtmpbc_status_t rtmpbc_arena_alloc_mat(rtmpbc_arena_t* arena, size_t rows, size_t cols, rtmpbc_mat_t* out_mat) {
    if (!arena || !out_mat) {
        return RTMPBC_ERR_NULL_PTR;
    }
    size_t count = rows * cols;
    out_mat->data = rtmpbc_arena_alloc_real(arena, count);
    if (!out_mat->data && count > 0) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }
    out_mat->rows = rows;
    out_mat->cols = cols;
    return RTMPBC_OK;
}

void rtmpbc_arena_reset(rtmpbc_arena_t* arena) {
    if (arena) {
        arena->offset = 0;
    }
}

size_t rtmpbc_arena_mark(const rtmpbc_arena_t* arena) {
    return arena ? arena->offset : 0;
}

void rtmpbc_arena_restore(rtmpbc_arena_t* arena, size_t mark) {
    if (arena && mark <= arena->capacity) {
        arena->offset = mark;
    }
}

size_t rtmpbc_arena_high_water_mark(const rtmpbc_arena_t* arena) {
    return arena ? arena->high_water_mark : 0;
}

size_t rtmpbc_arena_remaining(const rtmpbc_arena_t* arena) {
    if (!arena || arena->offset >= arena->capacity) {
        return 0;
    }
    return arena->capacity - arena->offset;
}
