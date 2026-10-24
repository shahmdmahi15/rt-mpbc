use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum Domain {
    MechanicalTranslational,
    MechanicalRotational,
    Electrical,
    Hydraulic,
    Thermal,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum Causality {
    Unassigned,
    /// Effort is directed from source to target (stroke at target)
    EffortOut,
    /// Flow is directed from source to target (stroke at source)
    FlowOut,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Bond {
    pub id: usize,
    pub from_node: usize,
    pub to_node: usize,
    pub domain: Domain,
    pub causality: Causality,
}

impl Bond {
    pub fn new(id: usize, from_node: usize, to_node: usize, domain: Domain) -> Self {
        Self {
            id,
            from_node,
            to_node,
            domain,
            causality: Causality::Unassigned,
        }
    }
}
