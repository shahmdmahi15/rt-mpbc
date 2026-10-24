//! Const-generic Port-Hamiltonian Implicit Midpoint Solver in `#![no_std]`.
//!
//! Guarantees energy preservation and thermodynamic consistency with zero heap allocation.

#[derive(Clone, Copy, Debug)]
pub struct PortHamiltonianSystem<const N: usize, const M: usize> {
    pub j: [[f64; N]; N], // Skew-symmetric (J = -J^T)
    pub r: [[f64; N]; N], // Dissipation (R = R^T >= 0)
    pub q: [[f64; N]; N], // Energy matrix (Q = Q^T > 0)
    pub g: [[f64; M]; N], // Input matrix (N x M)
}

#[derive(Debug)]
pub struct MidpointSolver<const N: usize, const M: usize> {
    pub phs: PortHamiltonianSystem<N, M>,
    pub dt: f64,
    pub x: [f64; N],
    pub last_energy: f64,
    pub next_energy: f64,
    pub power_dissipated: f64,
    pub power_input: f64,
    pub energy_error: f64,
}

impl<const N: usize, const M: usize> MidpointSolver<N, M> {
    pub fn new(phs: PortHamiltonianSystem<N, M>, dt: f64) -> Self {
        Self {
            phs,
            dt,
            x: [0.0; N],
            last_energy: 0.0,
            next_energy: 0.0,
            power_dissipated: 0.0,
            power_input: 0.0,
            energy_error: 0.0,
        }
    }

    /// Calculate total stored Hamiltonian energy H(x) = 1/2 * x^T * Q * x
    pub fn hamiltonian(&self, state: &[f64; N]) -> f64 {
        let mut h = 0.0;
        for i in 0..N {
            let mut q_x_i = 0.0;
            for j in 0..N {
                q_x_i += self.phs.q[i][j] * state[j];
            }
            h += 0.5 * state[i] * q_x_i;
        }
        h
    }

    /// Execute one symplectic implicit midpoint step
    pub fn step(&mut self, u_input: &[f64; M], y_output: &mut [f64; M]) -> Result<(), &'static str> {
        let half_dt = 0.5 * self.dt;
        self.last_energy = self.hamiltonian(&self.x);

        // Compute W = (J - R) * Q
        let mut w = [[0.0; N]; N];
        for i in 0..N {
            for j in 0..N {
                let mut sum = 0.0;
                for k in 0..N {
                    let j_minus_r = self.phs.j[i][k] - self.phs.r[i][k];
                    sum += j_minus_r * self.phs.q[k][j];
                }
                w[i][j] = sum;
            }
        }

        // Construct iteration matrix A = I - half_dt * W
        // and RHS vector b = (I + half_dt * W) * x + dt * G * u
        let mut a = [[0.0; N]; N];
        let mut b = [0.0; N];

        for i in 0..N {
            let mut sum_b = 0.0;
            for j in 0..N {
                let delta = if i == j { 1.0 } else { 0.0 };
                a[i][j] = delta - half_dt * w[i][j];
                sum_b += (delta + half_dt * w[i][j]) * self.x[j];
            }
            for p in 0..M {
                sum_b += self.dt * self.phs.g[i][p] * u_input[p];
            }
            b[i] = sum_b;
        }

        // Solve A * x_next = b via in-place LU with partial pivoting
        let x_next = solve_linear_system::<N>(&mut a, &b)?;

        // Compute midpoint state: x_mid = (x + x_next) / 2
        let mut x_mid = [0.0; N];
        for i in 0..N {
            x_mid[i] = 0.5 * (self.x[i] + x_next[i]);
        }

        // Compute gradient grad_H = Q * x_mid
        let mut grad_h = [0.0; N];
        for i in 0..N {
            let mut sum = 0.0;
            for j in 0..N {
                sum += self.phs.q[i][j] * x_mid[j];
            }
            grad_h[i] = sum;
        }

        // Compute conjugated output y = G^T * grad_H
        self.power_input = 0.0;
        for p in 0..M {
            let mut y_p = 0.0;
            for i in 0..N {
                y_p += self.phs.g[i][p] * grad_h[i];
            }
            y_output[p] = y_p;
            self.power_input += u_input[p] * y_p;
        }

        // Compute dissipated power P_diss = grad_H^T * R * grad_H
        self.power_dissipated = 0.0;
        for i in 0..N {
            let mut r_grad_i = 0.0;
            for j in 0..N {
                r_grad_i += self.phs.r[i][j] * grad_h[j];
            }
            self.power_dissipated += grad_h[i] * r_grad_i;
        }

        self.x = x_next;
        self.next_energy = self.hamiltonian(&self.x);

        let delta_h = self.next_energy - self.last_energy;
        let expected_delta_h = (self.power_input - self.power_dissipated) * self.dt;
        self.energy_error = (delta_h - expected_delta_h).abs();

        Ok(())
    }
}

/// Helper: Solve A * x = b in-place without dynamic memory
fn solve_linear_system<const N: usize>(
    a: &mut [[f64; N]; N],
    b: &[f64; N],
) -> Result<[f64; N], &'static str> {
    let mut pivot = [0usize; N];
    for i in 0..N {
        pivot[i] = i;
    }

    // LU decomposition with partial pivoting
    for k in 0..N {
        let mut max_val = a[k][k].abs();
        let mut max_row = k;
        for i in (k + 1)..N {
            let val = a[i][k].abs();
            if val > max_val {
                max_val = val;
                max_row = i;
            }
        }

        if max_val < 1e-12 {
            return Err("Singular matrix encountered in midpoint solver");
        }

        if max_row != k {
            pivot.swap(k, max_row);
            for j in 0..N {
                let tmp = a[k][j];
                a[k][j] = a[max_row][j];
                a[max_row][j] = tmp;
            }
        }

        let diag = a[k][k];
        for i in (k + 1)..N {
            a[i][k] /= diag;
            let factor = a[i][k];
            for j in (k + 1)..N {
                a[i][j] -= factor * a[k][j];
            }
        }
    }

    // Forward substitution
    let mut y = [0.0; N];
    for i in 0..N {
        let p = pivot[i];
        let mut sum = b[p];
        for j in 0..i {
            sum -= a[i][j] * y[j];
        }
        y[i] = sum;
    }

    // Back substitution
    let mut x = [0.0; N];
    for i in (0..N).rev() {
        let mut sum = y[i];
        for j in (i + 1)..N {
            sum -= a[i][j] * x[j];
        }
        x[i] = sum / a[i][i];
    }

    Ok(x)
}
