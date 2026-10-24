#include "rtmpbc.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ARENA_BUF_SIZE 16384

int main(void) {
    printf("[TEST] Running test_multirate (Coupled Multi-Rate Boundary Commutator)...\n");

    RTMPBC_ALIGN64 static uint8_t raw_buffer[ARENA_BUF_SIZE];
    rtmpbc_arena_t arena;
    rtmpbc_arena_init(&arena, raw_buffer, ARENA_BUF_SIZE);

    /* 1 coupled boundary port, 10:1 sub-stepping ratio */
    size_t n_ports = 1;
    size_t sub_ratio = 10;
    rtmpbc_real_t dt_micro = 0.0001; /* 10 kHz fast loop */

    rtmpbc_multirate_coupler_t coupler;
    rtmpbc_status_t status = rtmpbc_multirate_init(&coupler, n_ports, sub_ratio, dt_micro, &arena);
    assert(status == RTMPBC_OK);
    assert(fabs(coupler.dt_macro - 0.001) < 1e-12); /* 1 kHz slow loop */

    /* Run 100 macro steps = 1,000 micro steps */
    rtmpbc_real_t slow_flow = 2.5; /* Constant or slowly varying reaction flow */
    coupler.last_macro_flow[0] = slow_flow; /* Initial condition for boundary flow */

    for (size_t macro = 0; macro < 100; ++macro) {
        /* Micro-steps */
        for (size_t sub = 0; sub < sub_ratio; ++sub) {
            /* High-frequency oscillating effort: e = 10.0 * sin(omega * t) + 5.0 */
            rtmpbc_real_t t = (rtmpbc_real_t)(macro * sub_ratio + sub) * dt_micro;
            rtmpbc_real_t e_fast = 10.0 * sin(2.0 * M_PI * 100.0 * t) + 5.0;
            rtmpbc_real_t f_fast_received = 0.0;

            status = rtmpbc_multirate_micro_step(&coupler, &e_fast, &f_fast_received);
            assert(status == RTMPBC_OK);
        }

        /* Macro-step */
        rtmpbc_real_t e_slow_received = 0.0;
        status = rtmpbc_multirate_macro_step(&coupler, &e_slow_received, &slow_flow);
        assert(status == RTMPBC_OK);
    }

    printf("  Micro energy transferred: %.8f J\n", coupler.energy_transferred_micro);
    printf("  Macro energy transferred: %.8f J\n", coupler.energy_transferred_macro);

    rtmpbc_real_t energy_mismatch = fabs(coupler.energy_transferred_micro - coupler.energy_transferred_macro);
    printf("  Multi-rate boundary energy mismatch = %.2e J\n", energy_mismatch);

    /* Must conserve boundary energy down to machine precision */
    assert(energy_mismatch < 1e-10);

    printf("[TEST PASS] test_multirate boundary commutator verified lossless energy transfer!\n");
    return 0;
}
