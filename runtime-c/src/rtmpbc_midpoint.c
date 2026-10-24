/**
 * @file rtmpbc_midpoint.c
 * @brief Implementation of Symplectic Implicit Midpoint solver for Port-Hamiltonian systems.
 */

#include "rtmpbc_midpoint.h"
#include <math.h>
#include <string.h>

/* Static helper: Dense matrix-vector product y = A * x */
static void mat_vec_mul(const rtmpbc_mat_t* A, const rtmpbc_vec_t* x, rtmpbc_vec_t* y) {
    for (size_t i = 0; i < A->rows; ++i) {
        rtmpbc_real_t sum = 0.0;
        const rtmpbc_real_t* row = &A->data[i * A->cols];
        for (size_t j = 0; j < A->cols; ++j) {
            sum += row[j] * x->data[j];
        }
        y->data[i] = sum;
    }
}

/* Static helper: In-place LU decomposition with partial pivoting (zero heap) */
static rtmpbc_status_t lu_decompose(rtmpbc_mat_t* A, int32_t* pivot) {
    size_t n = A->rows;
    for (size_t i = 0; i < n; ++i) {
        pivot[i] = (int32_t)i;
    }

    for (size_t k = 0; k < n; ++k) {
        /* Find pivot row */
        size_t max_row = k;
        rtmpbc_real_t max_val = fabs(A->data[k * n + k]);
        for (size_t i = k + 1; i < n; ++i) {
            rtmpbc_real_t val = fabs(A->data[i * n + k]);
            if (val > max_val) {
                max_val = val;
                max_row = i;
            }
        }

        if (max_val < RTMPBC_EPSILON) {
            return RTMPBC_ERR_SINGULAR;
        }

        /* Swap pivot indices and rows */
        if (max_row != k) {
            int32_t temp_p = pivot[k];
            pivot[k] = pivot[max_row];
            pivot[max_row] = temp_p;

            for (size_t j = 0; j < n; ++j) {
                rtmpbc_real_t temp = A->data[k * n + j];
                A->data[k * n + j] = A->data[max_row * n + j];
                A->data[max_row * n + j] = temp;
            }
        }

        /* Gaussian elimination */
        rtmpbc_real_t diag = A->data[k * n + k];
        for (size_t i = k + 1; i < n; ++i) {
            A->data[i * n + k] /= diag;
            rtmpbc_real_t factor = A->data[i * n + k];
            for (size_t j = k + 1; j < n; ++j) {
                A->data[i * n + j] -= factor * A->data[k * n + j];
            }
        }
    }

    return RTMPBC_OK;
}

/* Static helper: Solve A * x = b using precomputed LU decomposition */
static void lu_solve(const rtmpbc_mat_t* LU, const int32_t* pivot, const rtmpbc_vec_t* b, rtmpbc_vec_t* x) {
    size_t n = LU->rows;

    /* Forward substitution: L * y = P * b */
    for (size_t i = 0; i < n; ++i) {
        size_t p = (size_t)pivot[i];
        rtmpbc_real_t sum = b->data[p];
        for (size_t j = 0; j < i; ++j) {
            sum -= LU->data[i * n + j] * x->data[j];
        }
        x->data[i] = sum;
    }

    /* Back substitution: U * x = y */
    for (size_t i = n; i-- > 0;) {
        rtmpbc_real_t sum = x->data[i];
        for (size_t j = i + 1; j < n; ++j) {
            sum -= LU->data[i * n + j] * x->data[j];
        }
        x->data[i] = sum / LU->data[i * n + i];
    }
}

rtmpbc_status_t rtmpbc_midpoint_init(
    rtmpbc_midpoint_solver_t* solver,
    rtmpbc_phs_linear_t* phs,
    rtmpbc_real_t dt,
    size_t max_newton_iters,
    rtmpbc_arena_t* arena
) {
    if (!solver || !phs || !arena || dt <= 0.0) {
        return RTMPBC_ERR_NULL_PTR;
    }

    size_t n = phs->n_states;
    solver->phs = phs;
    solver->dt = dt;
    solver->max_newton_iters = (max_newton_iters > 0) ? max_newton_iters : 3;
    solver->tolerance = RTMPBC_EPSILON;

    /* Allocate static memory buffers from arena */
    if (rtmpbc_arena_alloc_vec(arena, n, &solver->x) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n, &solver->x_next) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n, &solver->x_mid) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n, &solver->grad_H) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n, &solver->residual) != RTMPBC_OK ||
        rtmpbc_arena_alloc_vec(arena, n, &solver->b_vec) != RTMPBC_OK ||
        rtmpbc_arena_alloc_mat(arena, n, n, &solver->A_mat) != RTMPBC_OK ||
        rtmpbc_arena_alloc_mat(arena, n, n, &solver->LU_scratch) != RTMPBC_OK) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    solver->pivot_scratch = (int32_t*)rtmpbc_arena_alloc(arena, n * sizeof(int32_t), sizeof(int32_t));
    if (!solver->pivot_scratch) {
        return RTMPBC_ERR_OUT_OF_MEMORY;
    }

    /* Initialize state vectors to zero */
    memset(solver->x.data, 0, n * sizeof(rtmpbc_real_t));
    memset(solver->x_next.data, 0, n * sizeof(rtmpbc_real_t));

    solver->last_energy = 0.0;
    solver->next_energy = 0.0;
    solver->power_dissipated = 0.0;
    solver->power_input = 0.0;
    solver->energy_error = 0.0;

    return RTMPBC_OK;
}

rtmpbc_real_t rtmpbc_midpoint_hamiltonian(const rtmpbc_phs_linear_t* phs, const rtmpbc_vec_t* x) {
    if (!phs || !x) {
        return 0.0;
    }
    size_t n = phs->n_states;
    rtmpbc_real_t H = 0.0;
    for (size_t i = 0; i < n; ++i) {
        rtmpbc_real_t q_i = 0.0;
        for (size_t j = 0; j < n; ++j) {
            q_i += phs->Q.data[i * n + j] * x->data[j];
        }
        H += 0.5 * x->data[i] * q_i;
    }
    return H;
}

rtmpbc_status_t rtmpbc_midpoint_step(
    rtmpbc_midpoint_solver_t* solver,
    const rtmpbc_vec_t* u_input,
    rtmpbc_vec_t* y_output
) {
    if (!solver || !solver->phs) {
        return RTMPBC_ERR_NULL_PTR;
    }

    rtmpbc_phs_linear_t* phs = solver->phs;
    size_t n = phs->n_states;
    size_t m = phs->n_inputs;
    rtmpbc_real_t dt = solver->dt;
    rtmpbc_real_t half_dt = 0.5 * dt;

    /* Record initial energy H(x_k) */
    solver->last_energy = rtmpbc_midpoint_hamiltonian(phs, &solver->x);

    /* Construct W = (J - R) * Q */
    /* A_mat = I - (dt/2) * W */
    /* b_vec = (I + (dt/2) * W) * x_k + dt * G * u */
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            /* Compute element (J - R)[i, :] * Q[:, j] */
            rtmpbc_real_t W_ij = 0.0;
            for (size_t k = 0; k < n; ++k) {
                rtmpbc_real_t J_minus_R = phs->J.data[i * n + k] - phs->R.data[i * n + k];
                W_ij += J_minus_R * phs->Q.data[k * n + j];
            }

            rtmpbc_real_t delta_ij = (i == j) ? 1.0 : 0.0;
            solver->A_mat.data[i * n + j] = delta_ij - half_dt * W_ij;
            solver->LU_scratch.data[i * n + j] = solver->A_mat.data[i * n + j];
        }
    }

    /* Compute RHS: b_vec = (I + half_dt * W) * x_k */
    for (size_t i = 0; i < n; ++i) {
        rtmpbc_real_t sum = 0.0;
        for (size_t j = 0; j < n; ++j) {
            rtmpbc_real_t W_ij = 0.0;
            for (size_t k = 0; k < n; ++k) {
                rtmpbc_real_t J_minus_R = phs->J.data[i * n + k] - phs->R.data[i * n + k];
                W_ij += J_minus_R * phs->Q.data[k * n + j];
            }
            rtmpbc_real_t delta_ij = (i == j) ? 1.0 : 0.0;
            sum += (delta_ij + half_dt * W_ij) * solver->x.data[j];
        }

        /* Add input contribution dt * G * u */
        if (u_input && u_input->data && m > 0) {
            for (size_t p = 0; p < m; ++p) {
                sum += dt * phs->G.data[i * m + p] * u_input->data[p];
            }
        }
        solver->b_vec.data[i] = sum;
    }

    /* Decompose A_mat and solve for x_{k+1} */
    rtmpbc_status_t status = lu_decompose(&solver->LU_scratch, solver->pivot_scratch);
    if (status != RTMPBC_OK) {
        return status;
    }

    lu_solve(&solver->LU_scratch, solver->pivot_scratch, &solver->b_vec, &solver->x_next);

    /* Compute midpoint state x_mid = (x_k + x_{k+1}) / 2 */
    for (size_t i = 0; i < n; ++i) {
        solver->x_mid.data[i] = 0.5 * (solver->x.data[i] + solver->x_next.data[i]);
    }

    /* Compute grad_H = Q * x_mid */
    mat_vec_mul(&phs->Q, &solver->x_mid, &solver->grad_H);

    /* Compute conjugate output y = G^T * grad_H + M * u */
    solver->power_input = 0.0;
    if (y_output && y_output->data && m > 0) {
        for (size_t p = 0; p < m; ++p) {
            rtmpbc_real_t y_p = 0.0;
            for (size_t i = 0; i < n; ++i) {
                /* G^T[p, i] = G[i, p] */
                y_p += phs->G.data[i * m + p] * solver->grad_H.data[i];
            }
            if (phs->M.data && u_input && u_input->data) {
                for (size_t q = 0; q < m; ++q) {
                    y_p += phs->M.data[p * m + q] * u_input->data[q];
                }
            }
            y_output->data[p] = y_p;

            if (u_input && u_input->data) {
                solver->power_input += u_input->data[p] * y_p;
            }
        }
    }

    /* Compute dissipated power P_diss = grad_H^T * R * grad_H */
    solver->power_dissipated = 0.0;
    for (size_t i = 0; i < n; ++i) {
        rtmpbc_real_t R_grad_i = 0.0;
        for (size_t j = 0; j < n; ++j) {
            R_grad_i += phs->R.data[i * n + j] * solver->grad_H.data[j];
        }
        solver->power_dissipated += solver->grad_H.data[i] * R_grad_i;
    }

    /* Advance state x_k = x_{k+1} */
    memcpy(solver->x.data, solver->x_next.data, n * sizeof(rtmpbc_real_t));

    /* Record final energy H(x_{k+1}) and verify First-Law energy invariant */
    solver->next_energy = rtmpbc_midpoint_hamiltonian(phs, &solver->x);
    rtmpbc_real_t delta_H = solver->next_energy - solver->last_energy;
    rtmpbc_real_t energy_balance = (solver->power_input - solver->power_dissipated) * dt;
    solver->energy_error = fabs(delta_H - energy_balance);

    return RTMPBC_OK;
}
