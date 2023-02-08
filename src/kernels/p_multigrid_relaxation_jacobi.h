inline void p_multigrid_relaxation_jacobi(const double *dt, const int *order,
                                          const double *Au, const double *f,
                                          const double *op, double *u) {
  // Au = u + (F - Au)
  const int dg_np = DG_CONSTANTS[(*order - 1) * 5];

  for(int i = 0; i < dg_np; i++) {
    const int op_ind = i * dg_np + i;
    u[i] += *dt * (f[i] - Au[i]) / op[op_ind];
  }
}
