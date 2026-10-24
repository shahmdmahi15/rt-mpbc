#include "rtmpbc.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define ARENA_BUF_SIZE 4096

int main(void) {
    printf("[TEST] Running test_fixed_arena...\n");

    RTMPBC_ALIGN64 static uint8_t raw_buffer[ARENA_BUF_SIZE];
    rtmpbc_arena_t arena;

    /* 1. Init check */
    rtmpbc_status_t status = rtmpbc_arena_init(&arena, raw_buffer, ARENA_BUF_SIZE);
    assert(status == RTMPBC_OK);
    assert(arena.capacity == ARENA_BUF_SIZE);
    assert(arena.offset == 0);
    assert(arena.high_water_mark == 0);

    /* 2. Alignment check: alloc 64-byte aligned reals */
    rtmpbc_real_t* reals = rtmpbc_arena_alloc_real(&arena, 10);
    assert(reals != NULL);
    assert(((uintptr_t)reals % 64) == 0);

    /* 3. High water mark check */
    size_t mark1 = rtmpbc_arena_mark(&arena);
    assert(mark1 > 0);
    assert(rtmpbc_arena_high_water_mark(&arena) == mark1);

    /* 4. Alloc vector view */
    rtmpbc_vec_t vec;
    status = rtmpbc_arena_alloc_vec(&arena, 8, &vec);
    assert(status == RTMPBC_OK);
    assert(vec.length == 8);
    assert(((uintptr_t)vec.data % 64) == 0);

    /* 5. Alloc matrix view */
    rtmpbc_mat_t mat;
    status = rtmpbc_arena_alloc_mat(&arena, 4, 4, &mat);
    assert(status == RTMPBC_OK);
    assert(mat.rows == 4 && mat.cols == 4);
    assert(((uintptr_t)mat.data % 64) == 0);

    /* 6. Mark and Restore */
    size_t mark2 = rtmpbc_arena_mark(&arena);
    void* temp = rtmpbc_arena_alloc(&arena, 512, 16);
    assert(temp != NULL);
    assert(rtmpbc_arena_mark(&arena) > mark2);

    rtmpbc_arena_restore(&arena, mark2);
    assert(rtmpbc_arena_mark(&arena) == mark2);

    /* 7. Full reset */
    rtmpbc_arena_reset(&arena);
    assert(rtmpbc_arena_mark(&arena) == 0);
    /* High water mark persists */
    assert(rtmpbc_arena_high_water_mark(&arena) > 0);

    /* 8. Out of memory check */
    void* huge = rtmpbc_arena_alloc(&arena, ARENA_BUF_SIZE + 100, 8);
    assert(huge == NULL);

    printf("[TEST PASS] test_fixed_arena completed successfully!\n");
    return 0;
}
