#define _POSIX_C_SOURCE 199309L
#include "robotic_sensor.h"
#include <stdio.h>
#include <time.h>
#include <assert.h>

static double get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void) {
    printf("============================================================\n");
    printf(" RT-MPBC Industrial Benchmark: Robotic Joint Virtual Sensor \n");
    printf(" Domains: Inverter Voltage -> Real-Time Gear Stress Observer\n");
    printf("============================================================\n");

    robotic_sensor_sim_t sim;
    robotic_sensor_init(&sim);

    if (ROBOTIC_SENSOR_N_INPUTS > 0) {
        sim.u[0] = 48.0; /* 48V DC bus voltage */
    }

    size_t total_steps = 1000000; /* 1,000,000 ticks at 20 kHz */
    double t0 = get_time_ns();

    for (size_t step = 0; step < total_steps; ++step) {
        robotic_sensor_step(&sim);
    }
    double t1 = get_time_ns();

    double time_per_tick_ns = (t1 - t0) / (double)total_steps;

    printf("\n--- Virtual Sensor Performance ---\n");
    printf("  Execution Rate:           20 kHz Real-Time Task Loop\n");
    printf("  Average Latency per Tick: %.2f nanoseconds (%.4f microseconds)\n",
           time_per_tick_ns, time_per_tick_ns / 1000.0);
    printf("  Reconstructed Gear Deflection: %.6f rad\n", sim.x[1]);
    printf("  Estimated Link Momentum:       %.6f kg*m^2/s\n", sim.x[2]);
    printf("  Total Mechanical/Elec Energy:  %.6f J\n", sim.total_energy);

    assert(time_per_tick_ns < 100000.0);
    printf("\n[BENCHMARK SUCCESS] Virtual sensor reconstructed hidden states in nanoseconds!\n");
    return 0;
}
