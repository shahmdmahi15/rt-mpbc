use crate::ast::types::{Bond, Domain};
use crate::ast::elements::{Node, ElementType};
use serde::{Serialize, Deserialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BondGraph {
    pub name: String,
    pub nodes: Vec<Node>,
    pub bonds: Vec<Bond>,
}

impl BondGraph {
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            nodes: Vec::new(),
            bonds: Vec::new(),
        }
    }

    pub fn add_node(&mut self, element: ElementType) -> usize {
        let id = self.nodes.len();
        self.nodes.push(Node::new(id, element));
        id
    }

    pub fn connect(&mut self, from_id: usize, to_id: usize, domain: Domain) -> usize {
        assert!(from_id < self.nodes.len(), "Invalid from_node index");
        assert!(to_id < self.nodes.len(), "Invalid to_node index");

        let bond_id = self.bonds.len();
        let bond = Bond::new(bond_id, from_id, to_id, domain);
        self.bonds.push(bond);

        self.nodes[from_id].attached_bonds.push(bond_id);
        self.nodes[to_id].attached_bonds.push(bond_id);

        bond_id
    }

    pub fn get_storage_nodes(&self) -> Vec<&Node> {
        self.nodes.iter().filter(|n| n.is_storage()).collect()
    }

    pub fn get_source_nodes(&self) -> Vec<&Node> {
        self.nodes.iter().filter(|n| n.is_source()).collect()
    }

    pub fn get_resistor_nodes(&self) -> Vec<&Node> {
        self.nodes.iter().filter(|n| n.is_resistor()).collect()
    }
}
