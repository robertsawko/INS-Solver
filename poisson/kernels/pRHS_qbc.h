inline void pRHS_qbc(const int *bedge_type, const int *bedgeNum, const double *q,
                    double *exq) {
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

  if(*bedge_type == 4) {
    for(int i = 0; i < 5; i++) {
      exq[exInd + i] += -q[fmask[i]];
    }
  }
}