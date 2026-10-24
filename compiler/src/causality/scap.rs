use crate::ast::graph::BondGraph;
use crate::ast::types::Causality;
use crate::ast::elements::ElementType;
use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ScapResult {
    pub success: bool,
    pub num_independent_states: usize,
    pub independent_state_nodes: Vec<usize>,
    pub dependent_state_nodes: Vec<usize>,
    pub algebraic_loops: Vec<usize>,
}

pub struct ScapSolver<'a> {
    graph: &'a mut BondGraph,
}

impl<'a> ScapSolver<'a> {
    pub fn new(graph: &'a mut BondGraph) -> Self {
        Self { graph }
    }

    /// Determines if effort on `bond_id` is directed towards `node_id`.
    fn effort_into_node(&self, bond_id: usize, node_id: usize) -> Option<bool> {
        let bond = &self.graph.bonds[bond_id];
        match bond.causality {
            Causality::Unassigned => None,
            Causality::EffortOut => {
                // Effort flows from from_node to to_node
                Some(bond.to_node == node_id)
            }
            Causality::FlowOut => {
                // Effort flows from to_node to from_node
                Some(bond.from_node == node_id)
            }
        }
    }

    /// Set effort on `bond_id` to be directed towards `node_id` (effort in).
    fn set_effort_into_node(&mut self, bond_id: usize, node_id: usize) {
        let bond = &mut self.graph.bonds[bond_id];
        if bond.to_node == node_id {
            bond.causality = Causality::EffortOut;
        } else {
            bond.causality = Causality::FlowOut;
        }
    }

    /// Set flow on `bond_id` to be directed towards `node_id` (flow in / effort out).
    fn set_flow_into_node(&mut self, bond_id: usize, node_id: usize) {
        let bond = &mut self.graph.bonds[bond_id];
        if bond.from_node == node_id {
            bond.causality = Causality::EffortOut;
        } else {
            bond.causality = Causality::FlowOut;
        }
    }

    /// Propagate causality across all junctions, transformers, and gyrators.
    pub fn propagate(&mut self) -> bool {
        let mut changed = true;
        let mut any_change = false;

        while changed {
            changed = false;
            let num_nodes = self.graph.nodes.len();

            for node_idx in 0..num_nodes {
                let node = &self.graph.nodes[node_idx];
                let attached = node.attached_bonds.clone();

                match &node.element {
                    ElementType::Junction0 { .. } => {
                        // 0-Junction: Exactly ONE bond has effort in.
                        let mut effort_in_count = 0;
                        let mut unassigned = Vec::new();

                        for &b_id in &attached {
                            match self.effort_into_node(b_id, node_idx) {
                                Some(true) => effort_in_count += 1,
                                Some(false) => {},
                                None => unassigned.push(b_id),
                            }
                        }

                        if effort_in_count == 1 && !unassigned.is_empty() {
                            // All other unassigned must have effort OUT (flow in)
                            for &b_id in &unassigned {
                                self.set_flow_into_node(b_id, node_idx);
                                changed = true;
                                any_change = true;
                            }
                        } else if effort_in_count == 0 && unassigned.len() == 1 {
                            // Exactly one remaining must have effort IN
                            self.set_effort_into_node(unassigned[0], node_idx);
                            changed = true;
                            any_change = true;
                        }
                    }

                    ElementType::Junction1 { .. } => {
                        // 1-Junction: Exactly ONE bond has flow in (effort out).
                        let mut flow_in_count = 0;
                        let mut unassigned = Vec::new();

                        for &b_id in &attached {
                            match self.effort_into_node(b_id, node_idx) {
                                Some(false) => flow_in_count += 1, // Effort out means flow in
                                Some(true) => {},
                                None => unassigned.push(b_id),
                            }
                        }

                        if flow_in_count == 1 && !unassigned.is_empty() {
                            // All other unassigned must have flow OUT (effort in)
                            for &b_id in &unassigned {
                                self.set_effort_into_node(b_id, node_idx);
                                changed = true;
                                any_change = true;
                            }
                        } else if flow_in_count == 0 && unassigned.len() == 1 {
                            // Exactly one remaining must have flow IN (effort out)
                            self.set_flow_into_node(unassigned[0], node_idx);
                            changed = true;
                            any_change = true;
                        }
                    }

                    ElementType::TF { .. } => {
                        // Transformer: preserves causality (effort in port 0 -> effort out port 1)
                        if attached.len() == 2 {
                            let b0 = attached[0];
                            let b1 = attached[1];
                            let c0 = self.effort_into_node(b0, node_idx);
                            let c1 = self.effort_into_node(b1, node_idx);

                            if let (Some(eff0), None) = (c0, c1) {
                                if eff0 {
                                    self.set_flow_into_node(b1, node_idx);
                                } else {
                                    self.set_effort_into_node(b1, node_idx);
                                }
                                changed = true;
                                any_change = true;
                            } else if let (None, Some(eff1)) = (c0, c1) {
                                if eff1 {
                                    self.set_flow_into_node(b0, node_idx);
                                } else {
                                    self.set_effort_into_node(b0, node_idx);
                                }
                                changed = true;
                                any_change = true;
                            }
                        }
                    }

                    ElementType::GY { .. } => {
                        // Gyrator: swaps causality (effort in port 0 -> flow out port 1 = effort in port 1)
                        if attached.len() == 2 {
                            let b0 = attached[0];
                            let b1 = attached[1];
                            let c0 = self.effort_into_node(b0, node_idx);
                            let c1 = self.effort_into_node(b1, node_idx);

                            if let (Some(eff0), None) = (c0, c1) {
                                if eff0 {
                                    self.set_effort_into_node(b1, node_idx);
                                } else {
                                    self.set_flow_into_node(b1, node_idx);
                                }
                                changed = true;
                                any_change = true;
                            } else if let (None, Some(eff1)) = (c0, c1) {
                                if eff1 {
                                    self.set_effort_into_node(b0, node_idx);
                                } else {
                                    self.set_flow_into_node(b0, node_idx);
                                }
                                changed = true;
                                any_change = true;
                            }
                        }
                    }

                    _ => {}
                }
            }
        }

        any_change
    }

    /// Execute the full Karnopp Sequential Causality Assignment Procedure.
    pub fn solve(&mut self) -> ScapResult {
        // Step 1: Assign fixed causality to Sources
        for node_idx in 0..self.graph.nodes.len() {
            let element = self.graph.nodes[node_idx].element.clone();
            let attached = self.graph.nodes[node_idx].attached_bonds.clone();
            match element {
                ElementType::Se { .. } => {
                    // Effort source dictates effort OUT to system
                    for &b_id in &attached {
                        self.set_flow_into_node(b_id, node_idx); // Effort out
                    }
                }
                ElementType::Sf { .. } => {
                    // Flow source dictates flow OUT to system (effort in)
                    for &b_id in &attached {
                        self.set_effort_into_node(b_id, node_idx); // Flow out
                    }
                }
                _ => {}
            }
        }
        self.propagate();

        // Step 2: Assign preferred integral causality to Storage Elements (C and I)
        let mut independent_states = Vec::new();
        let mut dependent_states = Vec::new();

        for node_idx in 0..self.graph.nodes.len() {
            if !self.graph.nodes[node_idx].is_storage() {
                continue;
            }

            let element = self.graph.nodes[node_idx].element.clone();
            let bond_id = self.graph.nodes[node_idx].attached_bonds[0];
            let current_causality = self.effort_into_node(bond_id, node_idx);

            match (&element, current_causality) {
                // C element prefers Effort IN (integral causality)
                (ElementType::C { .. }, None) => {
                    self.set_effort_into_node(bond_id, node_idx);
                    independent_states.push(node_idx);
                    self.propagate();
                }
                (ElementType::C { .. }, Some(true)) => {
                    // Already assigned effort in
                    independent_states.push(node_idx);
                }
                (ElementType::C { .. }, Some(false)) => {
                    // Forced into flow in -> DERIVATIVE CAUSALITY
                    dependent_states.push(node_idx);
                }

                // I element prefers Flow IN (effort out, integral causality)
                (ElementType::I { .. }, None) => {
                    self.set_flow_into_node(bond_id, node_idx);
                    independent_states.push(node_idx);
                    self.propagate();
                }
                (ElementType::I { .. }, Some(false)) => {
                    // Already assigned flow in
                    independent_states.push(node_idx);
                }
                (ElementType::I { .. }, Some(true)) => {
                    // Forced into effort in -> DERIVATIVE CAUSALITY
                    dependent_states.push(node_idx);
                }

                _ => {}
            }
        }

        // Step 3: Assign causality to Resistive Elements (R)
        let mut algebraic_loops = Vec::new();
        for node_idx in 0..self.graph.nodes.len() {
            if !self.graph.nodes[node_idx].is_resistor() {
                continue;
            }

            let bond_id = self.graph.nodes[node_idx].attached_bonds[0];
            if self.effort_into_node(bond_id, node_idx).is_none() {
                // Arbitrary assignment (effort in)
                self.set_effort_into_node(bond_id, node_idx);
                algebraic_loops.push(node_idx);
                self.propagate();
            }
        }

        ScapResult {
            success: true,
            num_independent_states: independent_states.len(),
            independent_state_nodes: independent_states,
            dependent_state_nodes: dependent_states,
            algebraic_loops,
        }
    }
}
