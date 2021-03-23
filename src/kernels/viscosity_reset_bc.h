inline void viscosity_reset_bc(double *exQ0, double *exQ1) {
  for(int i = 0; i < 21; i++) {
    exQ0[i] = 0.0;
    exQ1[i] = 0.0;
  }
}