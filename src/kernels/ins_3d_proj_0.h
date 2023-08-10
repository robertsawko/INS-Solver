inline void ins_3d_proj_0(DG_FP *residual, const DG_FP *rhs0, const DG_FP *rhs1,
                          const DG_FP *rhs2, const DG_FP *Ax0, const DG_FP *Ax1,
                          const DG_FP *Ax2, DG_FP *r0, DG_FP *r1, DG_FP *r2,
                          DG_FP *z0, DG_FP *z1, DG_FP *z2) {
  DG_FP res_tmp = 0.0;
  for(int i = 0; i < DG_NP; i++) {
    r0[i] = rhs0[i] - Ax0[i];
    r1[i] = rhs1[i] - Ax1[i];
    r2[i] = rhs2[i] - Ax2[i];

    res_tmp += r0[i] * r0[i] + r1[i] * r1[i] + r2[i] * r2[i];

    z0[i] = r0[i];
    z1[i] = r1[i];
    z2[i] = r2[i];
  }

  *residual += res_tmp;
}
