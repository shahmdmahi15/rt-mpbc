"""
pyrtmpbc: Python bindings for Real-Time Multi-Physics Bond-Graph Compiler & Symplectic Solver
Zero mandatory external dependencies; seamless optional NumPy acceleration.
"""

import os
import sys
import ctypes
from typing import List, Optional, Tuple, Sequence

__version__ = "1.1.0"

try:
    import numpy as np
    _HAS_NUMPY = True
except ImportError:
    np = None
    _HAS_NUMPY = False

def _find_library() -> Optional[str]:
    possible_paths = [
        os.path.join(os.path.dirname(__file__), "librtmpbc.so"),
        os.path.join(os.path.dirname(__file__), "..", "..", "librtmpbc.so"),
        "/usr/local/lib/librtmpbc.so",
        "/usr/lib/librtmpbc.so",
    ]
    for p in possible_paths:
        abs_p = os.path.abspath(p)
        if os.path.exists(abs_p):
            return abs_p
    return None

_lib_path = _find_library()
_lib = ctypes.CDLL(_lib_path) if _lib_path else None


def _solve_2x2(A: Sequence[Sequence[float]], b: Sequence[float]) -> List[float]:
    """Pure-Python Cramers rule solver for 2x2 systems without numpy"""
    det = A[0][0] * A[1][1] - A[0][1] * A[1][0]
    if abs(det) < 1e-15:
        raise ValueError("Singular matrix in symplectic step")
    x0 = (b[0] * A[1][1] - A[0][1] * b[1]) / det
    x1 = (A[0][0] * b[1] - b[0] * A[1][0]) / det
    return [x0, x1]


class SymplecticMidpointSolver:
    """
    Implicit Midpoint Symplectic Integrator for Port-Hamiltonian systems.
    Preserves first-law energy and symplectic 2-form invariants.
    """
    def __init__(self, n_states: int, dt: float, J, R, Q, G=None):
        self.n = n_states
        self.dt = dt
        self.x = [0.0] * n_states
        self.u = [0.0]
        
        if _HAS_NUMPY and isinstance(J, np.ndarray):
            self.use_numpy = True
            self.J = np.array(J, dtype=np.float64)
            self.R = np.array(R, dtype=np.float64)
            self.Q = np.array(Q, dtype=np.float64)
            self.m = G.shape[1] if G is not None else 0
            self.G = np.array(G, dtype=np.float64) if G is not None else np.zeros((self.n, 1), dtype=np.float64)
            self.A = (self.J - self.R) @ self.Q
            self.I = np.eye(self.n)
            self.M_lhs = self.I - 0.5 * self.dt * self.A
            self.M_rhs = self.I + 0.5 * self.dt * self.A
            self.x_arr = np.zeros(self.n, dtype=np.float64)
        else:
            self.use_numpy = False
            self.J = [list(row) for row in J]
            self.R = [list(row) for row in R]
            self.Q = [list(row) for row in Q]
            self.m = len(G[0]) if (G and len(G) > 0 and hasattr(G[0], '__len__')) else 0
            self.G = G

    def set_state(self, x: Sequence[float]) -> None:
        self.x = [float(v) for v in x]
        if self.use_numpy:
            self.x_arr = np.array(self.x, dtype=np.float64)

    def get_state(self) -> List[float]:
        return list(self.x)

    def hamiltonian(self) -> float:
        """Returns total stored physical energy H(x) = 0.5 * x^T * Q * x"""
        if self.use_numpy:
            return float(0.5 * self.x_arr.T @ self.Q @ self.x_arr)
        # Pure-python quad form
        energy = 0.0
        for i in range(self.n):
            for j in range(self.n):
                energy += 0.5 * self.x[i] * self.Q[i][j] * self.x[j]
        return energy

    def step(self) -> List[float]:
        """Advance simulation by one time step dt with symplectic preservation."""
        if self.use_numpy:
            rhs = self.M_rhs @ self.x_arr
            if self.m > 0:
                rhs += self.dt * (self.G @ np.array(self.u[:self.m], dtype=np.float64))
            self.x_arr = np.linalg.solve(self.M_lhs, rhs)
            self.x = list(self.x_arr)
            return self.x
        else:
            if self.n == 2:
                # Optimized 2x2 implicit midpoint
                A = [[0.0, 0.0], [0.0, 0.0]]
                for i in range(2):
                    for j in range(2):
                        for k in range(2):
                            A[i][j] += (self.J[i][k] - self.R[i][k]) * self.Q[k][j]
                M_lhs = [
                    [1.0 - 0.5 * self.dt * A[0][0], -0.5 * self.dt * A[0][1]],
                    [-0.5 * self.dt * A[1][0], 1.0 - 0.5 * self.dt * A[1][1]],
                ]
                rhs = [
                    (1.0 + 0.5 * self.dt * A[0][0]) * self.x[0] + (0.5 * self.dt * A[0][1]) * self.x[1],
                    (0.5 * self.dt * A[1][0]) * self.x[0] + (1.0 + 0.5 * self.dt * A[1][1]) * self.x[1],
                ]
                self.x = _solve_2x2(M_lhs, rhs)
                return self.x
            else:
                raise NotImplementedError("Pure Python solver supports 2x2 directly; install NumPy for arbitrary dimension N.")


__all__ = ["SymplecticMidpointSolver", "__version__"]
