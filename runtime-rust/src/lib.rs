//! # RT-MPBC Runtime
//!
//! A bare-metal, dependency-free (`#![no_std]`), hard real-time Port-Hamiltonian
//! multi-physics solver and passivity-based observer engine.

#![no_std]

pub mod units;
pub mod arena;
pub mod solver;
pub mod observer;

pub use units::*;
pub use arena::*;
pub use solver::*;
pub use observer::*;
