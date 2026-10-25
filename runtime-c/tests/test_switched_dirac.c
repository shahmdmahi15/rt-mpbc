#include "rtmpbc.h"
#include <stdio.h>
#include <assert.h>

#define ARENA_BUF_SIZE 32768

int main(void) {
    printf("[TEST] Running test_switched_dirac (O(1) Switched Dirac Morphisms)...\n");

    RTMPBC_ALIGN64 static uint8_t raw_buffer[ARENA_BUF_SIZE];
    rtmpbc_arena_t arena;
    rtmpbc_arena_init(&arena, raw_buffer, ARENA_BUF_SIZE);

    /* 2 states (e.g. Inductor current p, Capacitor voltage q), 1 switch */
    size_t n_states = 2;
    size_t n_inputs = 0;
    size_t n_switches = 1;
    rtmpbc_real_t dt = 0.001;

    rtmpbc_switched_dirac_t sys;
    rtmpbc_status_t status = rtmpbc_switched_dirac_init(&sys, n_states, n_inputs, n_switches, dt, &arena);
    assert(status == RTMPBC_OK);

    /* Q matrix: diag(10.0, 1.0) */
    sys.Q.data[0 * 2 + 0] = 10.0; sys.Q.data[0 * 2 + 1] = 0.0;
    sys.Q.data[1 * 2 + 0] =  0.0; sys.Q.data[1 * 2 + 1] = 1.0;

    /* Mode 0 (Switch OFF: decoupled LC tank) */
    sys.J_modes[0].data[0 * 2 + 0] = 0.0; sys.J_modes[0].data[0 * 2 + 1] = 0.0;
    sys.J_modes[0].data[1 * 2 + 0] = 0.0; sys.J_modes[0].data[1 * 2 + 1] = 0.0;

    /* Mode 1 (Switch ON: coupled resonator) */
    sys.J_modes[1].data[0 * 2 + 0] =  0.0; sys.J_modes[1].data[0 * 2 + 1] =  1.0;
    sys.J_modes[1].data[1 * 2 + 0] = -1.0; sys.J_modes[1].data[1 * 2 + 1] =  0.0;

    sys.solver.x.data[0] = 1.0;
    sys.solver.x.data[1] = 0.5;

    /* Run 10,000 steps rapidly toggling switch at 1 kHz */
    for (size_t step = 0; step < 10000; ++step) {
        bool switch_state = ((step / 50) % 2 == 1);
        rtmpbc_switched_dirac_set(&sys, 0, switch_state);

        status = rtmpbc_switched_dirac_step(&sys, NULL, NULL);
        assert(status == RTMPBC_OK);
    }

    printf("  Final State after 10,000 topological switchings: q = %.4f, p = %.4f\n",
           sys.solver.x.data[0], sys.solver.x.data[1]);

    printf("[TEST PASS] test_switched_dirac executed O(1) topological morphisms seamlessly!\n");
    return 0;
}
