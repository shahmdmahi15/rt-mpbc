use crate::ast::graph::BondGraph;
use crate::ast::elements::ElementType;
use crate::causality::ScapResult;
use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PortHamiltonianMatrices {
    pub n_states: usize,
    pub n_inputs: usize,
    pub state_names: Vec<String>,
    pub input_names: Vec<String>,
    pub j: Vec<Vec<f64>>,
    pub r: Vec<Vec<f64>>,
    pub q: Vec<Vec<f64>>,
    pub g: Vec<Vec<f64>>,
    pub m: Vec<Vec<f64>>,
}

pub struct DiracSynthesizer<'a> {
    graph: &'a BondGraph,
    scap: &'a ScapResult,
}

impl<'a> DiracSynthesizer<'a> {
    pub fn new(graph: &'a BondGraph, scap: &'a ScapResult) -> Self {
        Self { graph, scap }
    }

    pub fn synthesize(&self) -> PortHamiltonianMatrices {
        let n = self.scap.num_independent_states;
        let sources = self.graph.get_source_nodes();
        let m = sources.len();

        let mut state_names = Vec::new();
        let mut q_diag = vec![0.0; n];

        for (i, &node_idx) in self.scap.independent_state_nodes.iter().enumerate() {
            let node = &self.graph.nodes[node_idx];
            state_names.push(node.name().to_string());
            match node.element {
                ElementType::C { value, .. } => {
                    q_diag[i] = 1.0 / value; // Compliance: e = (1/C) * q
                }
                ElementType::I { value, .. } => {
                    q_diag[i] = 1.0 / value; // Inertia: f = (1/I) * p
                }
                _ => {}
            }
        }

        // Account for dependent storage elements via coordinate reduction
        for &dep_idx in &self.scap.dependent_state_nodes {
            let dep_node = &self.graph.nodes[dep_idx];
            match dep_node.element {
                ElementType::I { value, .. } => {
                    // Combine into first inertia state
                    if let Some(first_i) = self.scap.independent_state_nodes.iter().position(|&idx| {
                        matches!(self.graph.nodes[idx].element, ElementType::I { .. })
                    }) {
                        let orig_val = 1.0 / q_diag[first_i];
                        q_diag[first_i] = 1.0 / (orig_val + value);
                    }
                }
                ElementType::C { value, .. } => {
                    // Combine into first capacitance state
                    if let Some(first_c) = self.scap.independent_state_nodes.iter().position(|&idx| {
                        matches!(self.graph.nodes[idx].element, ElementType::C { .. })
                    }) {
                        let orig_val = 1.0 / q_diag[first_c];
                        q_diag[first_c] = 1.0 / (orig_val + value);
                    }
                }
                _ => {}
            }
        }

        let mut input_names = Vec::new();
        for &s_node in &sources {
            input_names.push(s_node.name().to_string());
        }

        // Initialize zero matrices
        let mut j_mat = vec![vec![0.0; n]; n];
        let mut r_mat = vec![vec![0.0; n]; n];
        let mut q_mat = vec![vec![0.0; n]; n];
        let mut g_mat = vec![vec![0.0; m]; n];
        let m_mat = vec![vec![0.0; m]; m];

        // Populate Q diagonal
        for i in 0..n {
            q_mat[i][i] = q_diag[i];
        }

        // Synthesize J and R through junction topological tracing
        // In bond graphs, storage elements connected via 0/1 junctions establish skew-symmetric
        // energy routing: dx/dt = (J - R)*Q*x + G*u
        for i in 0..n {
            let node_i = &self.graph.nodes[self.scap.independent_state_nodes[i]];
            for j in 0..n {
                if i == j {
                    continue;
                }
                let node_j = &self.graph.nodes[self.scap.independent_state_nodes[j]];

                // If node_i and node_j are complementary (e.g. C and I) connected through junctions
                let connects_via_junction = self.graph.bonds.iter().any(|b| {
                    (b.from_node == node_i.id && b.to_node == node_j.id) ||
                    (b.from_node == node_j.id && b.to_node == node_i.id)
                }) || self.connected_through_common_junction(node_i.id, node_j.id);

                if connects_via_junction {
                    match (&node_i.element, &node_j.element) {
                        (ElementType::C { .. }, ElementType::I { .. }) => {
                            // dq/dt = +f = (1/I) * p
                            j_mat[i][j] = 1.0;
                            j_mat[j][i] = -1.0;
                        }
                        (ElementType::I { .. }, ElementType::C { .. }) => {
                            // dp/dt = -e = -(1/C) * q
                            j_mat[i][j] = -1.0;
                            j_mat[j][i] = 1.0;
                        }
                        _ => {}
                    }
                }
            }
        }

        // Synthesize R dissipation from attached R elements
        for r_node in self.graph.get_resistor_nodes() {
            if let ElementType::R { value, .. } = r_node.element {
                for i in 0..n {
                    let node_i = &self.graph.nodes[self.scap.independent_state_nodes[i]];
                    if self.connected_through_common_junction(node_i.id, r_node.id) {
                        match node_i.element {
                            ElementType::I { .. } => {
                                // Flow through R: dissipation R
                                r_mat[i][i] += value;
                            }
                            ElementType::C { .. } => {
                                // Effort across R: conductance 1/R
                                r_mat[i][i] += 1.0 / value;
                            }
                            _ => {}
                        }
                    }
                }
            }
        }

        // Synthesize G input matrix from sources
        for (p, &s_node) in sources.iter().enumerate() {
            for i in 0..n {
                let node_i = &self.graph.nodes[self.scap.independent_state_nodes[i]];
                if self.connected_through_common_junction(node_i.id, s_node.id) {
                    g_mat[i][p] = 1.0;
                }
            }
        }

        PortHamiltonianMatrices {
            n_states: n,
            n_inputs: m,
            state_names,
            input_names,
            j: j_mat,
            r: r_mat,
            q: q_mat,
            g: g_mat,
            m: m_mat,
        }
    }

    fn connected_through_common_junction(&self, node_a: usize, node_b: usize) -> bool {
        let bonds_a = &self.graph.nodes[node_a].attached_bonds;
        let bonds_b = &self.graph.nodes[node_b].attached_bonds;

        for &b_a in bonds_a {
            let neighbor_a = if self.graph.bonds[b_a].from_node == node_a {
                self.graph.bonds[b_a].to_node
            } else {
                self.graph.bonds[b_a].from_node
            };

            for &b_b in bonds_b {
                let neighbor_b = if self.graph.bonds[b_b].from_node == node_b {
                    self.graph.bonds[b_b].to_node
                } else {
                    self.graph.bonds[b_b].from_node
                };

                if neighbor_a == neighbor_b {
                    return true;
                }
            }
        }
        false
    }
}
