# pyrtmpbc

**Python interface for RT-MPBC: Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver**

`pyrtmpbc` brings sub-50 nanosecond Port-Hamiltonian simulation and energy-preserving geometric integration to Python.

## Installation

```bash
pip install pyrtmpbc
```

Or from source:
```bash
git clone https://github.com/shahmdmahi15/rt-mpbc.git
cd rt-mpbc/bindings/python
pip install .
```

## Quickstart

```python
import numpy as np
from pyrtmpbc import SymplecticMidpointSolver

# Define an LC Tank / Mass-Spring Oscillator
# Hamiltonian H = 0.5 * q^2 / C + 0.5 * p^2 / L
n_states = 2
dt = 1e-4

# Skew-symmetric interconnection J = [0, -1; 1, 0]
J = np.array([[0.0, -1.0],
              [1.0,  0.0]])

# Resistive dissipation R = [0, 0; 0, 0.05] (small resistance)
R = np.array([[0.0, 0.0],
              [0.0, 0.05]])

# Energy matrix Q = diag(1/C, 1/L)
Q = np.diag([1.0, 1.0])

solver = SymplecticMidpointSolver(n_states=n_states, dt=dt, J=J, R=R, Q=Q)
solver.set_state([1.0, 0.0])

for _ in range(1000):
    solver.step()
    x = solver.get_state()
    h = solver.hamiltonian()
    print(f"Energy: {h:.10f} J")
```
