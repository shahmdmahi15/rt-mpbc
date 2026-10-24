//! Const-generic Passivity-Based Port-Hamiltonian State Observer (PH-PBO).
//!
//! Provides a "Virtual Sensor Array" that reconstructs hidden internal states
//! in sub-microsecond loops with guaranteed Lyapunov stability in `#![no_std]`.

use crate::solver::{PortHamiltonianSystem, MidpointSolver};

#[derive(Debug)]
pub struct PortHamiltonianObserver<const N: usize, const M: usize> {
    pub solver: MidpointSolver<N, M>,
    pub lambda_diag: [f64; M],
    pub x_hat: [f64; N],
    pub y_hat: [f64; M],
    pub innovation: [f64; M],
}

impl<const N: usize, const M: usize> PortHamiltonianObserver<N, M> {
    pub fn new(phs: PortHamiltonianSystem<N, M>, dt: f64, lambda_diag: [f64; M]) -> Self {
        // Construct augmented dissipation R_obs = R + G * Lambda * G^T
        let mut r_obs = phs.r;
        for i in 0..N {
            for j in 0..N {
                let mut sum = 0.0;
                for p in 0..M {
                    sum += phs.g[i][p] * lambda_diag[p] * phs.g[j][p];
                }
                r_obs[i][j] += sum;
            }
        }

        let shadow_phs = PortHamiltonianSystem {
            j: phs.j,
            r: r_obs,
            q: phs.q,
            g: phs.g,
        };

        Self {
            solver: MidpointSolver::new(shadow_phs, dt),
            lambda_diag,
            x_hat: [0.0; N],
            y_hat: [0.0; M],
            innovation: [0.0; M],
        }
    }

    /// Update observer estimate using input u and measurement y_meas
    pub fn update(&mut self, u_input: &[f64; M], y_meas: &[f64; M]) -> Result<(), &'static str> {
        // u_effective = u + Lambda * y_meas
        let mut u_effective = [0.0; M];
        for p in 0..M {
            u_effective[p] = u_input[p] + self.lambda_diag[p] * y_meas[p];
        }

        let mut y_pred = [0.0; M];
        self.solver.step(&u_effective, &mut y_pred)?;

        self.x_hat = self.solver.x;
        self.y_hat = y_pred;

        for p in 0..M {
            self.innovation[p] = y_meas[p] - self.y_hat[p];
        }

        Ok(())
    }
}
