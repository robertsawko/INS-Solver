inline void factor_poisson_coarse_matrix_3d_op2(const DG_FP *dr,
                    const DG_FP *ds, const DG_FP *dt, const DG_FP *mmF0,
                    const DG_FP *mmF1, const DG_FP *mmF2, const DG_FP *mmF3,
                    const int *faceNum, const int *fmaskL_corrected,
                    const int *fmaskR_corrected, const DG_FP *nx,
                    const DG_FP *ny, const DG_FP *nz, const DG_FP *fscale,
                    const DG_FP *sJ, const DG_FP **rx, const DG_FP **sx,
                    const DG_FP **tx, const DG_FP **ry, const DG_FP **sy,
                    const DG_FP **ty, const DG_FP **rz, const DG_FP **sz,
                    const DG_FP **tz, const DG_FP **factor, DG_FP *op1L,
                    DG_FP *op1R, DG_FP *op2L, DG_FP *op2R) {
  const DG_FP *dr_mat = dr;
  const DG_FP *ds_mat = ds;
  const DG_FP *dt_mat = dt;
  const DG_FP *mmF0_mat = mmF0;
  const DG_FP *mmF1_mat = mmF1;
  const DG_FP *mmF2_mat = mmF2;
  const DG_FP *mmF3_mat = mmF3;

  const DG_FP tau_order = 1.0; // (DG_FP) DG_ORDER;

  const DG_FP *mmFL, *mmFR;
  if(faceNum[0] == 0)
    mmFL = mmF0_mat;
  else if(faceNum[0] == 1)
    mmFL = mmF1_mat;
  else if(faceNum[0] == 2)
    mmFL = mmF2_mat;
  else
    mmFL = mmF3_mat;

  if(faceNum[1] == 0)
    mmFR = mmF0_mat;
  else if(faceNum[1] == 1)
    mmFR = mmF1_mat;
  else if(faceNum[1] == 2)
    mmFR = mmF2_mat;
  else
    mmFR = mmF3_mat;

  const int findL = faceNum[0] * DG_NPF_N1;
  const int findR = faceNum[1] * DG_NPF_N1;
  const int *fmask  = FMASK;
  const int *fmaskL = &fmask[faceNum[0] * DG_NPF_N1];
  const int *fmaskR = &fmask[faceNum[1] * DG_NPF_N1];

  DG_FP DL[DG_NP_N1 * DG_NP_N1], DR[DG_NP_N1 * DG_NP_N1];
  const DG_FP r_fact_0 = nx[0] * rx[0][0] + ny[0] * ry[0][0] + nz[0] * rz[0][0];
  const DG_FP s_fact_0 = nx[0] * sx[0][0] + ny[0] * sy[0][0] + nz[0] * sz[0][0];
  const DG_FP t_fact_0 = nx[0] * tx[0][0] + ny[0] * ty[0][0] + nz[0] * tz[0][0];
  const DG_FP r_fact_1 = nx[1] * rx[1][0] + ny[1] * ry[1][0] + nz[1] * rz[1][0];
  const DG_FP s_fact_1 = nx[1] * sx[1][0] + ny[1] * sy[1][0] + nz[1] * sz[1][0];
  const DG_FP t_fact_1 = nx[1] * tx[1][0] + ny[1] * ty[1][0] + nz[1] * tz[1][0];
  for(int i = 0; i < DG_NP_N1; i++) {
    for(int j = 0; j < DG_NP_N1; j++) {
      // int ind = i + j * DG_NP_N1;
      int ind = DG_MAT_IND(i, j, DG_NP_N1, DG_NP_N1);

      DL[ind] = r_fact_0 * dr_mat[ind] + s_fact_0 * ds_mat[ind] + t_fact_0 * dt_mat[ind];
      DL[ind] *= factor[0][i];

      DR[ind] = r_fact_1 * dr_mat[ind] + s_fact_1 * ds_mat[ind] + t_fact_1 * dt_mat[ind];
      DR[ind] *= factor[1][i];
    }
  }

  DG_FP gtau = 0.0;
  for(int i = 0; i < DG_NPF_N1; i++) {
    DG_FP tmp = 2.0 * (tau_order + 1) * (tau_order + 2) * fmax(fscale[0] * factor[0][fmaskL[i]], fscale[1] * factor[1][fmaskR_corrected[i]]);
    gtau = fmax(gtau, tmp);
  }

  op2_in_kernel_gemm(false, false, DG_NP_N1, DG_NP_N1, DG_NP_N1, -0.5 * sJ[0], mmFL, DG_NP_N1, DL, DG_NP_N1, 1.0, op1L, DG_NP_N1);
  op2_in_kernel_gemm(true, false, DG_NP_N1, DG_NP_N1, DG_NP_N1, -0.5 * sJ[0], DL, DG_NP_N1, mmFL, DG_NP_N1, 1.0, op1L, DG_NP_N1);

  const DG_FP tmp_constL = 0.5 * sJ[0] * gtau;
  for(int i = 0; i < DG_NP_N1; i++) {
    for(int j = 0; j < DG_NP_N1; j++) {
      // int op_ind = i + j * DG_NP_N1;
      int op_ind = DG_MAT_IND(i, j, DG_NP_N1, DG_NP_N1);
      op1L[op_ind] += tmp_constL * mmFL[op_ind];
    }
  }

  for(int i = 0; i < DG_NP_N1 * DG_NP_N1; i++) {
    op2L[i] = 0.0;
  }

  for(int i = 0; i < DG_NP_N1; i++) {
    for(int j = 0; j < DG_NPF_N1; j++) {
      // int op_ind = i + fmaskR_corrected[j] * DG_NP_N1;
      int op_ind = DG_MAT_IND(i, fmaskR_corrected[j], DG_NP_N1, DG_NP_N1);
      // int find = i + fmaskL[j] * DG_NP_N1;
      int find = DG_MAT_IND(i, fmaskL[j], DG_NP_N1, DG_NP_N1);
      op2L[op_ind] -= 0.5 * gtau * sJ[0] * mmFL[find];
    }
  }

  for(int i = 0; i < DG_NPF_N1; i++) {
    for(int j = 0; j < DG_NP_N1; j++) {
      // int op_ind = fmaskL[i] + j * DG_NP_N1;
      int op_ind = DG_MAT_IND(fmaskL[i], j, DG_NP_N1, DG_NP_N1);
      for(int k = 0; k < DG_NPF_N1; k++) {
        // int a_ind = fmaskL[i] + fmaskL[k] * DG_NP_N1;
        int a_ind = DG_MAT_IND(fmaskL[i], fmaskL[k], DG_NP_N1, DG_NP_N1);
        // int b_ind = j * DG_NP_N1 + fmaskR_corrected[k];
        int b_ind = DG_MAT_IND(fmaskR_corrected[k], j, DG_NP_N1, DG_NP_N1);
        op2L[op_ind] -= 0.5 * sJ[0] * mmFL[a_ind] * -DR[b_ind];
      }
    }
  }

  for(int i = 0; i < DG_NP_N1; i++) {
    for(int j = 0; j < DG_NPF_N1; j++) {
      // int op_ind = i + fmaskR_corrected[j] * DG_NP_N1;
      int op_ind = DG_MAT_IND(i, fmaskR_corrected[j], DG_NP_N1, DG_NP_N1);
      for(int k = 0; k < DG_NP_N1; k++) {
        // int a_ind = i * DG_NP_N1 + k;
        int a_ind = DG_MAT_IND(k, i, DG_NP_N1, DG_NP_N1);
        // int b_ind = fmaskL[j] * DG_NP_N1 + k;
        int b_ind = DG_MAT_IND(k, fmaskL[j], DG_NP_N1, DG_NP_N1);
        op2L[op_ind] -= -0.5 * DL[a_ind] * sJ[0] * mmFL[b_ind];
      }
    }
  }

  // Do right face
  op2_in_kernel_gemm(false, false, DG_NP_N1, DG_NP_N1, DG_NP_N1, -0.5 * sJ[1], mmFR, DG_NP_N1, DR, DG_NP_N1, 1.0, op1R, DG_NP_N1);
  op2_in_kernel_gemm(true, false, DG_NP_N1, DG_NP_N1, DG_NP_N1, -0.5 * sJ[1], DR, DG_NP_N1, mmFR, DG_NP_N1, 1.0, op1R, DG_NP_N1);

  const DG_FP tmp_constR = 0.5 * sJ[1] * gtau;
  for(int i = 0; i < DG_NP_N1; i++) {
    for(int j = 0; j < DG_NP_N1; j++) {
      // int op_ind = i + j * DG_NP_N1;
      int op_ind = DG_MAT_IND(i, j, DG_NP_N1, DG_NP_N1);
      op1R[op_ind] += tmp_constR * mmFR[op_ind];
    }
  }

  for(int i = 0; i < DG_NP_N1 * DG_NP_N1; i++) {
    op2R[i] = 0.0;
  }

  for(int i = 0; i < DG_NP_N1; i++) {
    for(int j = 0; j < DG_NPF_N1; j++) {
      // int op_ind = i + fmaskL_corrected[j] * DG_NP_N1;
      int op_ind = DG_MAT_IND(i, fmaskL_corrected[j], DG_NP_N1, DG_NP_N1);
      // int find = i + fmaskR[j] * DG_NP_N1;
      int find = DG_MAT_IND(i, fmaskR[j], DG_NP_N1, DG_NP_N1);
      op2R[op_ind] -= 0.5 * gtau * sJ[1] * mmFR[find];
    }
  }

  for(int i = 0; i < DG_NPF_N1; i++) {
    for(int j = 0; j < DG_NP_N1; j++) {
      // int op_ind = fmaskR[i] + j * DG_NP_N1;
      int op_ind = DG_MAT_IND(fmaskR[i], j, DG_NP_N1, DG_NP_N1);
      for(int k = 0; k < DG_NPF_N1; k++) {
        // int a_ind = fmaskR[i] + fmaskR[k] * DG_NP_N1;
        int a_ind = DG_MAT_IND(fmaskR[i], fmaskR[k], DG_NP_N1, DG_NP_N1);
        // int b_ind = j * DG_NP_N1 + fmaskL_corrected[k];
        int b_ind = DG_MAT_IND(fmaskL_corrected[k], j, DG_NP_N1, DG_NP_N1);
        op2R[op_ind] -= 0.5 * sJ[1] * mmFR[a_ind] * -DL[b_ind];
      }
    }
  }

  for(int i = 0; i < DG_NP_N1; i++) {
    for(int j = 0; j < DG_NPF_N1; j++) {
      // int op_ind = i + fmaskL_corrected[j] * DG_NP_N1;
      int op_ind = DG_MAT_IND(i, fmaskL_corrected[j], DG_NP_N1, DG_NP_N1);
      for(int k = 0; k < DG_NP_N1; k++) {
        // int a_ind = i * DG_NP_N1 + k;
        int a_ind = DG_MAT_IND(k, i, DG_NP_N1, DG_NP_N1);
        // int b_ind = fmaskR[j] * DG_NP_N1 + k;
        int b_ind = DG_MAT_IND(k, fmaskR[j], DG_NP_N1, DG_NP_N1);
        op2R[op_ind] -= -0.5 * DR[a_ind] * sJ[1] * mmFR[b_ind];
      }
    }
  }
}
