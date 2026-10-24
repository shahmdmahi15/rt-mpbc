use crate::dirac::PortHamiltonianMatrices;
use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MetriplecticThermalModel {
    pub ambient_temperature_k: f64,
    pub thermal_capacitance_j_per_k: f64,
    pub thermal_resistance_k_per_w: f64,
    pub heat_source_weights: Vec<f64>,
}

impl MetriplecticThermalModel {
    pub fn new(
        ambient_temp_k: f64,
        c_th: f64,
        r_th: f64,
        matrices: &PortHamiltonianMatrices,
    ) -> Self {
        // Extract diagonal dissipation coefficients from R matrix
        let weights: Vec<f64> = (0..matrices.n_states)
            .map(|i| matrices.r[i][i])
            .collect();

        Self {
            ambient_temperature_k: ambient_temp_k,
            thermal_capacitance_j_per_k: c_th,
            thermal_resistance_k_per_w: r_th,
            heat_source_weights: weights,
        }
    }
}
