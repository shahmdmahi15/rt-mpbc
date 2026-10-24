use rtmpbc_runtime::*;

#[test]
fn test_rust_midpoint_harmonic_oscillator() {
    // 2-State mass-spring system: m = 2.0 kg, k = 50.0 N/m
    let phs = PortHamiltonianSystem {
        j: [
            [0.0, 1.0],
            [-1.0, 0.0],
        ],
        r: [
            [0.0, 0.0],
            [0.0, 0.0],
        ],
        q: [
            [50.0, 0.0],
            [0.0, 0.5],
        ],
        g: [
            [0.0],
            [0.0],
        ],
    };

    let dt = 0.005;
    let mut solver = MidpointSolver::new(phs, dt);
    solver.x = [1.0, 0.0]; // Initial condition: q=1.0m, p=0.0

    let h_initial = solver.hamiltonian(&solver.x);
    assert!((h_initial - 25.0).abs() < 1e-10);

    let u_in = [0.0];
    let mut y_out = [0.0];

    let mut max_drift = 0.0;
    for _ in 0..10_000 {
        solver.step(&u_in, &mut y_out).expect("Solver step failed");
        let drift = (solver.next_energy - h_initial).abs();
        if drift > max_drift {
            max_drift = drift;
        }
    }

    assert!(max_drift < 1e-8, "Symplectic midpoint must conserve energy forever");
}

#[test]
fn test_rust_passivity_observer_convergence() {
    // Damped oscillator: k = 100, m = 1, b = 2.0
    let phs = PortHamiltonianSystem {
        j: [
            [0.0, 1.0],
            [-1.0, 0.0],
        ],
        r: [
            [0.0, 0.0],
            [0.0, 2.0],
        ],
        q: [
            [100.0, 0.0],
            [0.0, 1.0],
        ],
        g: [
            [0.0],
            [1.0],
        ],
    };

    let dt = 0.001;
    let mut plant = MidpointSolver::new(phs, dt);
    plant.x = [2.0, 5.0]; // Initial plant state

    let mut observer = PortHamiltonianObserver::new(phs, dt, [10.0]);
    observer.x_hat = [0.0, 0.0]; // Zero knowledge

    let u_in = [0.0];
    let mut y_meas = [0.0];

    for _ in 0..2000 {
        plant.step(&u_in, &mut y_meas).expect("Plant step failed");
        observer.update(&u_in, &y_meas).expect("Observer update failed");
    }

    let err_q = (plant.x[0] - observer.x_hat[0]).abs();
    let err_p = (plant.x[1] - observer.x_hat[1]).abs();

    assert!(err_q < 1e-3, "Observer q must converge");
    assert!(err_p < 1e-3, "Observer p must converge");
}
