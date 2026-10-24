//! # RT-MPBC Compiler
//!
//! Real-Time Multi-Physics Bond-Graph Compiler.
//! Computes causality using Karnopp's SCAP, derives Dirac structure Port-Hamiltonian
//! matrices, and unrolls sparse matrices into flat zero-branch instruction tapes.

pub mod ast;
pub mod causality;
pub mod dirac;
pub mod metriplectic;
pub mod unroller;
pub mod codegen;

pub use ast::*;
pub use causality::*;
pub use dirac::*;
pub use metriplectic::*;
pub use unroller::*;
pub use codegen::*;
