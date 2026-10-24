use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum ElementType {
    /// Effort Source (e.g. Voltage source, External Force)
    Se { name: String, value: f64 },
    /// Flow Source (e.g. Current source, Velocity source)
    Sf { name: String, value: f64 },
    /// Compliance / Capacitance (Stores potential energy: q = integral(f), e = q/C)
    C { name: String, value: f64 },
    /// Inertia / Inductance (Stores kinetic energy: p = integral(e), f = p/I)
    I { name: String, value: f64 },
    /// Resistance / Dissipation (e = R * f or f = e/R)
    R { name: String, value: f64 },
    /// 0-Junction: Common Effort (e1 = e2 = ... = en, sum(f) = 0)
    Junction0 { name: String },
    /// 1-Junction: Common Flow (f1 = f2 = ... = fn, sum(e) = 0)
    Junction1 { name: String },
    /// Transformer (e2 = n * e1, f1 = n * f2)
    TF { name: String, modulus: f64 },
    /// Gyrator (e2 = r * f1, e1 = r * f2)
    GY { name: String, modulus: f64 },
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Node {
    pub id: usize,
    pub element: ElementType,
    pub attached_bonds: Vec<usize>,
}

impl Node {
    pub fn new(id: usize, element: ElementType) -> Self {
        Self {
            id,
            element,
            attached_bonds: Vec::new(),
        }
    }

    pub fn name(&self) -> &str {
        match &self.element {
            ElementType::Se { name, .. } => name,
            ElementType::Sf { name, .. } => name,
            ElementType::C { name, .. } => name,
            ElementType::I { name, .. } => name,
            ElementType::R { name, .. } => name,
            ElementType::Junction0 { name } => name,
            ElementType::Junction1 { name } => name,
            ElementType::TF { name, .. } => name,
            ElementType::GY { name, .. } => name,
        }
    }

    pub fn is_storage(&self) -> bool {
        matches!(self.element, ElementType::C { .. } | ElementType::I { .. })
    }

    pub fn is_source(&self) -> bool {
        matches!(self.element, ElementType::Se { .. } | ElementType::Sf { .. })
    }

    pub fn is_resistor(&self) -> bool {
        matches!(self.element, ElementType::R { .. })
    }
}
