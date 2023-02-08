inline void fact_poisson_gauss_op2(const int **p, const double *gF0Dr,
                                   const double *gF0Ds, const double *gF1Dr,
                                   const double *gF1Ds, const double *gF2Dr,
                                   const double *gF2Ds, const double *gFInterp0,
                                   const double *gFInterp1, const double *gFInterp2,
                                   const int *edgeNum, const bool *reverse,
                                   const double **x, const double **y,
                                   const double **sJ, const double **nx,
                                   const double **ny, const double **h,
                                   const double **factor, double *op1L,
                                   double *op1R, double *op2L, double *op2R) {
  int edgeL = edgeNum[0];
  int edgeR = edgeNum[1];

  // Get constants
  // Using same Gauss points so should be able to replace dg_gf_npL and
  // dg_gf_npR with DG_GF_NP
  const int dg_npL      = DG_CONSTANTS[(p[0][0] - 1) * 5];
  const int dg_npfL     = DG_CONSTANTS[(p[0][0] - 1) * 5 + 1];
  const int dg_gf_npL   = DG_CONSTANTS[(p[0][0] - 1) * 5 + 4];
  const double *gaussWL = &gaussW_g[(p[0][0] - 1) * DG_GF_NP];
  const int dg_npR      = DG_CONSTANTS[(p[1][0] - 1) * 5];
  const int dg_npfR     = DG_CONSTANTS[(p[1][0] - 1) * 5 + 1];
  const int dg_gf_npR   = DG_CONSTANTS[(p[1][0] - 1) * 5 + 4];
  const double *gaussWR = &gaussW_g[(p[1][0] - 1) * DG_GF_NP];

  const double *gDrL, *gDsL, *gDrR, *gDsR, *gVML, *gVMR;

  if(edgeL == 0) {
    gDrL = &gF0Dr[(p[0][0] - 1) * DG_GF_NP * DG_NP];
    gDsL = &gF0Ds[(p[0][0] - 1) * DG_GF_NP * DG_NP];
    gVML = &gFInterp0[(p[0][0] - 1) * DG_GF_NP * DG_NP];
  } else if(edgeL == 1) {
    gDrL = &gF1Dr[(p[0][0] - 1) * DG_GF_NP * DG_NP];
    gDsL = &gF1Ds[(p[0][0] - 1) * DG_GF_NP * DG_NP];
    gVML = &gFInterp1[(p[0][0] - 1) * DG_GF_NP * DG_NP];
  } else {
    gDrL = &gF2Dr[(p[0][0] - 1) * DG_GF_NP * DG_NP];
    gDsL = &gF2Ds[(p[0][0] - 1) * DG_GF_NP * DG_NP];
    gVML = &gFInterp2[(p[0][0] - 1) * DG_GF_NP * DG_NP];
  }

  if(edgeR == 0) {
    gDrR = &gF0Dr[(p[1][0] - 1) * DG_GF_NP * DG_NP];
    gDsR = &gF0Ds[(p[1][0] - 1) * DG_GF_NP * DG_NP];
    gVMR = &gFInterp0[(p[1][0] - 1) * DG_GF_NP * DG_NP];
  } else if(edgeR == 1) {
    gDrR = &gF1Dr[(p[1][0] - 1) * DG_GF_NP * DG_NP];
    gDsR = &gF1Ds[(p[1][0] - 1) * DG_GF_NP * DG_NP];
    gVMR = &gFInterp1[(p[1][0] - 1) * DG_GF_NP * DG_NP];
  } else {
    gDrR = &gF2Dr[(p[1][0] - 1) * DG_GF_NP * DG_NP];
    gDsR = &gF2Ds[(p[1][0] - 1) * DG_GF_NP * DG_NP];
    gVMR = &gFInterp2[(p[1][0] - 1) * DG_GF_NP * DG_NP];
  }

  double rxL[DG_GF_NP], sxL[DG_GF_NP], ryL[DG_GF_NP], syL[DG_GF_NP];
  double rxR[DG_GF_NP], sxR[DG_GF_NP], ryR[DG_GF_NP], syR[DG_GF_NP];

  // Left edge
  for(int m = 0; m < DG_GF_NP; m++) {
    rxL[m] = 0.0;
    sxL[m] = 0.0;
    ryL[m] = 0.0;
    syL[m] = 0.0;
    for(int n = 0; n < dg_npL; n++) {
      int ind = m + n * DG_GF_NP;
      rxL[m] += gDrL[ind] * x[0][n];
      sxL[m] += gDsL[ind] * x[0][n];
      ryL[m] += gDrL[ind] * y[0][n];
      syL[m] += gDsL[ind] * y[0][n];
    }
    double JL = -sxL[m] * ryL[m] + rxL[m] * syL[m];
    double rx_nL = syL[m] / JL;
    double sx_nL = -ryL[m] / JL;
    double ry_nL = -sxL[m] / JL;
    double sy_nL = rxL[m] / JL;
    rxL[m] = rx_nL;
    sxL[m] = sx_nL;
    ryL[m] = ry_nL;
    syL[m] = sy_nL;
  }

  // Right edge
  for(int m = 0; m < DG_GF_NP; m++) {
    rxR[m] = 0.0;
    sxR[m] = 0.0;
    ryR[m] = 0.0;
    syR[m] = 0.0;
    for(int n = 0; n < dg_npR; n++) {
      int ind = m + n * DG_GF_NP;
      rxR[m] += gDrR[ind] * x[1][n];
      sxR[m] += gDsR[ind] * x[1][n];
      ryR[m] += gDrR[ind] * y[1][n];
      syR[m] += gDsR[ind] * y[1][n];
    }
    double JR = -sxR[m] * ryR[m] + rxR[m] * syR[m];
    double rx_nR = syR[m] / JR;
    double sx_nR = -ryR[m] / JR;
    double ry_nR = -sxR[m] / JR;
    double sy_nR = rxR[m] / JR;
    rxR[m] = rx_nR;
    sxR[m] = sx_nR;
    ryR[m] = ry_nR;
    syR[m] = sy_nR;
  }

  // Left edge
  const int exIndL = edgeL * DG_GF_NP;
  const int exIndR = edgeR * DG_GF_NP;
  double mDL[DG_GF_NP * DG_NP], mDR[DG_GF_NP * DG_NP], pDL[DG_GF_NP * DG_NP], pDR[DG_GF_NP * DG_NP];
  for(int m = 0; m < DG_GF_NP; m++) {
    for(int n = 0; n < dg_npL; n++) {
      int ind = m + n * DG_GF_NP;
      int p_ind, p_norm_indR;
      if(*reverse) {
        p_ind = DG_GF_NP - 1 - m + n * DG_GF_NP;
        p_norm_indR = exIndR + DG_GF_NP - 1 - m;
      } else {
        p_ind = ind;
        p_norm_indR = exIndR + m;
      }

      double DxL = rxL[m] * gDrL[ind] + sxL[m] * gDsL[ind];
      double DyL = ryL[m] * gDrL[ind] + syL[m] * gDsL[ind];
      mDL[ind]   = factor[0][exIndL + m] * (nx[0][exIndL + m] * DxL + ny[0][exIndL + m] * DyL);
      pDR[p_ind] = factor[0][exIndL + m] * (nx[1][p_norm_indR] * DxL + ny[1][p_norm_indR] * DyL);
    }
  }

  // Right edge
  for(int m = 0; m < DG_GF_NP; m++) {
    for(int n = 0; n < dg_npR; n++) {
      int ind = m + n * DG_GF_NP;
      int p_ind, p_norm_indL;
      if(*reverse) {
        p_ind = DG_GF_NP - 1 - m + n * DG_GF_NP;
        p_norm_indL = exIndL + DG_GF_NP - 1 - m;
      } else {
        p_ind = ind;
        p_norm_indL = exIndL + m;
      }

      double DxR = rxR[m] * gDrR[ind] + sxR[m] * gDsR[ind];
      double DyR = ryR[m] * gDrR[ind] + syR[m] * gDsR[ind];
      mDR[ind]   = factor[1][exIndR + m] * (nx[1][exIndR + m] * DxR + ny[1][exIndR + m] * DyR);
      pDL[p_ind] = factor[1][exIndR + m] * (nx[0][p_norm_indL] * DxR + ny[0][p_norm_indL] * DyR);
    }
  }


  // Left edge
  double tauL[DG_GF_NP];
  double maxtau = 0.0;
  for(int i = 0; i < DG_GF_NP; i++) {
    int indL = edgeL * DG_GF_NP + i;
    int indR;
    if(reverse)
      indR = edgeR * DG_GF_NP + DG_GF_NP - 1 - i;
    else
      indR = edgeR * DG_GF_NP + i;

    tauL[i] = 0.5 * fmax((p[0][0] + 1) * (p[0][0] + 2) * h[0][0] * factor[0][indL], (p[1][0] + 1) * (p[1][0] + 2) * h[1][0] * factor[1][indR]);
    if(tauL[i] > maxtau) maxtau = tauL[i];
  }

  for(int i = 0; i < DG_GF_NP; i++) {
    tauL[i] = maxtau;
  }

  // Right edge
  double tauR[DG_GF_NP];
  maxtau = 0.0;
  for(int i = 0; i < DG_GF_NP; i++) {
    int indR = edgeR * DG_GF_NP + i;
    int indL;
    if(reverse)
      indL = edgeL * DG_GF_NP + DG_GF_NP - 1 - i;
    else
      indL = edgeL * DG_GF_NP + i;

    tauR[i] = 0.5 * fmax((p[0][0] + 1) * (p[0][0] + 2) * h[0][0] * factor[0][indL], (p[1][0] + 1) * (p[1][0] + 2) * h[1][0] * factor[1][indR]);
    if(tauR[i] > maxtau) maxtau = tauR[i];
  }

  for(int i = 0; i < DG_GF_NP; i++) {
    tauR[i] = maxtau;
  }

  // Left edge
  for(int m = 0; m < dg_npL; m++) {
    for(int n = 0; n < dg_npL; n++) {
      // op col-major
      int c_ind = m + n * dg_npL;
      for(int k = 0; k < DG_GF_NP; k++) {
        // Dx' and Dy'
        int a_ind = m * DG_GF_NP + k;
        // Dx and Dy
        int b_ind = n * DG_GF_NP + k;

        op1L[c_ind] += 0.5 * gaussWL[k] * sJ[0][exIndL + k] * tauL[k] * gVML[a_ind] * gVML[b_ind];
        op1L[c_ind] += -0.5 * gaussWL[k] * sJ[0][exIndL + k] * gVML[a_ind] * mDL[b_ind];
        op1L[c_ind] += -0.5 * gaussWL[k] * sJ[0][exIndL + k] * mDL[a_ind] * gVML[b_ind];
      }
    }
  }

  for(int m = 0; m < dg_npL; m++) {
    for(int n = 0; n < dg_npR; n++) {
      // op col-major
      int c_ind = m + n * dg_npL;
      op2L[c_ind] = 0.0;
      for(int k = 0; k < DG_GF_NP; k++) {
        // Dx' and Dy'
        int a_ind = m * DG_GF_NP + k;
        // Dx and Dy
        int b_ind = n * DG_GF_NP + k;

        int b_indP;
        if(*reverse) {
          b_indP = n * DG_GF_NP + DG_GF_NP - k - 1;
        } else {
          b_indP = n * DG_GF_NP + k;
        }

        op2L[c_ind] += -0.5 * gaussWL[k] * sJ[0][exIndL + k] * tauL[k] * gVML[a_ind] * gVMR[b_indP];
        op2L[c_ind] += -0.5 * gaussWL[k] * sJ[0][exIndL + k] * gVML[a_ind] * pDL[b_ind];
        op2L[c_ind] += 0.5 * gaussWL[k] * sJ[0][exIndL + k] * mDL[a_ind] * gVMR[b_indP];
      }
    }
  }

  // Right edge
  for(int m = 0; m < dg_npR; m++) {
    for(int n = 0; n < dg_npR; n++) {
      // op col-major
      int c_ind = m + n * dg_npR;
      for(int k = 0; k < DG_GF_NP; k++) {
        // Dx' and Dy'
        int a_ind = m * DG_GF_NP + k;
        // Dx and Dy
        int b_ind = n * DG_GF_NP + k;

        op1R[c_ind] += 0.5 * gaussWR[k] * sJ[1][exIndR + k] * tauR[k] * gVMR[a_ind] * gVMR[b_ind];
        op1R[c_ind] += -0.5 * gaussWR[k] * sJ[1][exIndR + k] * gVMR[a_ind] * mDR[b_ind];
        op1R[c_ind] += -0.5 * gaussWR[k] * sJ[1][exIndR + k] * mDR[a_ind] * gVMR[b_ind];
      }
    }
  }

  for(int m = 0; m < dg_npR; m++) {
    for(int n = 0; n < dg_npL; n++) {
      // op col-major
      int c_ind = m + n * dg_npR;
      op2R[c_ind] = 0.0;
      for(int k = 0; k < DG_GF_NP; k++) {
        // Dx' and Dy'
        int a_ind = m * DG_GF_NP + k;
        // Dx and Dy
        int b_ind = n * DG_GF_NP + k;

        int b_indP;
        if(*reverse) {
          b_indP = n * DG_GF_NP + DG_GF_NP - k - 1;
        } else {
          b_indP = n * DG_GF_NP + k;
        }

        op2R[c_ind] += -0.5 * gaussWR[k] * sJ[1][exIndR + k] * tauR[k] * gVMR[a_ind] * gVML[b_indP];
        op2R[c_ind] += -0.5 * gaussWR[k] * sJ[1][exIndR + k] * gVMR[a_ind] * pDR[b_ind];
        op2R[c_ind] += 0.5 * gaussWR[k] * sJ[1][exIndR + k] * mDR[a_ind] * gVML[b_indP];
      }
    }
  }
}