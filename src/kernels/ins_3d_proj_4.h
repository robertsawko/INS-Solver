inline void ins_3d_proj_4(const DG_FP *_beta, const DG_FP *z0, const DG_FP *z1,
                          const DG_FP *z2, DG_FP *p0, DG_FP *p1, DG_FP *p2) {
  const DG_FP beta = *_beta;
  for(int i = 0; i < DG_NP; i++) {
    p0[i] = z0[i] + beta * p0[i];
    p1[i] = z1[i] + beta * p1[i];
    p2[i] = z2[i] + beta * p2[i];
  }
}
