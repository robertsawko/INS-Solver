inline void zero_npf_1(double *val) {
  for(int i = 0; i < DG_NUM_FACES * DG_NPF; i++) {
    val[i] = 0.0;
  }
}
