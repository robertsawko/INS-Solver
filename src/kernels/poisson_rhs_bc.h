inline void poisson_rhs_bc(const int *bedge_type, const int *bedgeNum,
                           const double *U, double *exU) {
  int exInd = 0;
  if(*bedgeNum == 1) {
    exInd = 5;
  } else if(*bedgeNum == 2) {
    exInd = 2 * 5;
  }

  int *fmask;

  if(*bedgeNum == 0) {
    fmask = FMASK;
  } else if(*bedgeNum == 1) {
    fmask = &FMASK[5];
  } else {
    fmask = &FMASK[2 * 5];
  }

  // TODO Change this to bc types that are Dirichlet in this app
  if(*bedge_type == 0 || *bedge_type == 1 || *bedge_type == 2 || *bedge_type == 3) {
    for(int i = 0; i < 5; i++) {
      exU[exInd + i] += -U[fmask[i]];
    }
  }
}