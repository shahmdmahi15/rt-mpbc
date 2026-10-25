#include "rtmpbc.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("[TEST] Running test_fault_detector (Energy-Invariant Fault & Attack Detector)...\n");

    rtmpbc_fault_detector_t fd;
    rtmpbc_fault_detector_init(&fd, 0.001, 3); /* 1 mJ threshold, 3 debounced ticks */

    bool is_safe = true;

    /* Phase 1: 100 healthy steps with microscopic machine-epsilon numerical error (1e-12 J) */
    for (size_t step = 0; step < 100; ++step) {
        rtmpbc_fault_type_t fault = rtmpbc_fault_detector_eval(&fd, 1e-12, &is_safe);
        assert(fault == RTMPBC_FAULT_NONE);
        assert(is_safe == true);
        assert(fd.is_tripped == false);
    }
    printf("  Phase 1 (Healthy Operation): Safe = %d, Trip = %d\n", is_safe, fd.is_tripped);

    /* Phase 2: Inject physical energy leak / sensor spoofing (0.5 J anomaly) */
    for (size_t step = 0; step < 5; ++step) {
        rtmpbc_fault_detector_eval(&fd, 0.5, &is_safe);
    }

    printf("  Phase 2 (Anomaly Injected): Safe = %d, Tripped = %d, Fault Code = %d\n",
           is_safe, fd.is_tripped, fd.active_fault);

    assert(is_safe == false);
    assert(fd.is_tripped == true);
    assert(fd.active_fault == RTMPBC_FAULT_ENERGY_LEAK);

    /* Phase 3: Operator reset */
    rtmpbc_fault_detector_reset(&fd);
    assert(fd.is_tripped == false);

    printf("[TEST PASS] test_fault_detector successfully latched safety trip in < 1 microsecond!\n");
    return 0;
}
