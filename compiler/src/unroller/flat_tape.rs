use crate::dirac::PortHamiltonianMatrices;
use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum FmaOp {
    /// dest = c
    SetConst { dest: String, val: f64 },
    /// dest = src1 * src2
    Mul { dest: String, src1: String, src2: String },
    /// dest = dest + src1 * src2 (Fused Multiply-Add)
    Fma { dest: String, src1: String, src2: String },
    /// dest = dest + src
    Add { dest: String, src: String },
    /// dest = dest - src
    Sub { dest: String, src: String },
    /// dest = src / val
    DivConst { dest: String, src: String, val: f64 },
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct InstructionTape {
    pub ops: Vec<FmaOp>,
    pub n_states: usize,
    pub n_inputs: usize,
}

pub struct SparseUnroller<'a> {
    matrices: &'a PortHamiltonianMatrices,
    dt: f64,
}

impl<'a> SparseUnroller<'a> {
    pub fn new(matrices: &'a PortHamiltonianMatrices, dt: f64) -> Self {
        Self { matrices, dt }
    }

    pub fn compile_tape(&self) -> InstructionTape {
        let n = self.matrices.n_states;
        let m = self.matrices.n_inputs;
        let mut ops = Vec::new();

        // 1. Compute non-zero entries of W = (J - R) * Q
        let mut w = vec![vec![0.0; n]; n];
        for i in 0..n {
            for j in 0..n {
                let mut sum = 0.0;
                for k in 0..n {
                    let j_minus_r = self.matrices.j[i][k] - self.matrices.r[i][k];
                    sum += j_minus_r * self.matrices.q[k][j];
                }
                w[i][j] = sum;
            }
        }

        let half_dt = 0.5 * self.dt;

        // 2. Unroll RHS b_vec calculation: b[i] = x[i] + half_dt * sum_j(W[i][j] * x[j]) + dt * sum_p(G[i][p] * u[p])
        for i in 0..n {
            let b_dest = format!("b[{}]", i);
            ops.push(FmaOp::SetConst {
                dest: b_dest.clone(),
                val: 0.0,
            });

            // Self term (I + half_dt * W_ii) * x_i
            let diag_coeff = 1.0 + half_dt * w[i][i];
            if diag_coeff.abs() > 1e-12 {
                ops.push(FmaOp::Fma {
                    dest: b_dest.clone(),
                    src1: format!("{:.12}", diag_coeff),
                    src2: format!("x[{}]", i),
                });
            }

            // Off-diagonal terms half_dt * W_ij * x_j
            for j in 0..n {
                if i != j && w[i][j].abs() > 1e-12 {
                    let coeff = half_dt * w[i][j];
                    ops.push(FmaOp::Fma {
                        dest: b_dest.clone(),
                        src1: format!("{:.12}", coeff),
                        src2: format!("x[{}]", j),
                    });
                }
            }

            // Input coupling dt * G_ip * u_p
            for p in 0..m {
                if self.matrices.g[i][p].abs() > 1e-12 {
                    let coeff = self.dt * self.matrices.g[i][p];
                    ops.push(FmaOp::Fma {
                        dest: b_dest.clone(),
                        src1: format!("{:.12}", coeff),
                        src2: format!("u[{}]", p),
                    });
                }
            }
        }

        InstructionTape {
            ops,
            n_states: n,
            n_inputs: m,
        }
    }
}
