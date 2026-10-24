CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O3 -Iruntime-c/include
LDFLAGS ?= -lm

C_SRCS = runtime-c/src/rtmpbc_fixed_arena.c \
         runtime-c/src/rtmpbc_midpoint.c \
         runtime-c/src/rtmpbc_multirate.c \
         runtime-c/src/rtmpbc_observer.c

TEST_BINS = runtime-c/tests/test_fixed_arena \
            runtime-c/tests/test_midpoint \
            runtime-c/tests/test_multirate \
            runtime-c/tests/test_observer

BENCH_BINS = examples/01_ev_powertrain/ev_benchmark \
             examples/02_fhe_muscle_actuator/muscle_benchmark \
             examples/03_robotic_virtual_sensor/robot_benchmark

.PHONY: all test benchmark clean

all: $(TEST_BINS) $(BENCH_BINS)

runtime-c/tests/%: runtime-c/tests/%.c $(C_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

examples/01_ev_powertrain/ev_benchmark: examples/01_ev_powertrain/main_ev_benchmark.c examples/01_ev_powertrain/ev_powertrain.c $(C_SRCS)
	$(CC) $(CFLAGS) -Iexamples/01_ev_powertrain -o $@ $^ $(LDFLAGS)

examples/02_fhe_muscle_actuator/muscle_benchmark: examples/02_fhe_muscle_actuator/main_muscle_benchmark.c examples/02_fhe_muscle_actuator/muscle_actuator.c $(C_SRCS)
	$(CC) $(CFLAGS) -Iexamples/02_fhe_muscle_actuator -o $@ $^ $(LDFLAGS)

examples/03_robotic_virtual_sensor/robot_benchmark: examples/03_robotic_virtual_sensor/main_robot_benchmark.c examples/03_robotic_virtual_sensor/robotic_sensor.c $(C_SRCS)
	$(CC) $(CFLAGS) -Iexamples/03_robotic_virtual_sensor -o $@ $^ $(LDFLAGS)

test: $(TEST_BINS)
	@echo "=== Running RT-MPBC C99 Unit Tests ==="
	@./runtime-c/tests/test_fixed_arena
	@./runtime-c/tests/test_midpoint
	@./runtime-c/tests/test_multirate
	@./runtime-c/tests/test_observer
	@echo "=== All C99 Unit Tests Passed Successfully ==="

benchmark: $(BENCH_BINS)
	@echo "=== Running RT-MPBC Industrial Grand-Challenge Benchmarks ==="
	@./examples/01_ev_powertrain/ev_benchmark
	@./examples/02_fhe_muscle_actuator/muscle_benchmark
	@./examples/03_robotic_virtual_sensor/robot_benchmark
	@echo "=== All Benchmarks Completed with Sub-Microsecond Determinism ==="

clean:
	rm -f $(TEST_BINS) $(BENCH_BINS)
