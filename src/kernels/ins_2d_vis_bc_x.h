inline void ins_2d_vis_bc_x(const DG_FP *t, const int *bedge_type,
                            const int *bedgeNum, const DG_FP *nx, const DG_FP *ny,
                            const DG_FP *x, const DG_FP *y, DG_FP *out) {
  const DG_FP PI = 3.141592653589793238463;

  for(int i = 0; i < DG_NPF; i++) {
    out[i] = 0.0;
  }

  if(*bedge_type == 0) {
    // Inflow - BC function dependant on time
    for(int i = 0; i < DG_NPF; i++) {
      out[i] = 1.0;
    }
  } else if(*bedge_type == 1) {
    // Outflow - Natural boundary condition
  } else {
    // Wall - No slip
  }
}
