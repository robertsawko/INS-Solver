//
// auto-generated by op2.py
//

void gauss_op_omp4_kernel(
  double *data0,
  int dat0size,
  double *data1,
  int dat1size,
  double *data2,
  int dat2size,
  double *data3,
  int dat3size,
  double *data4,
  int dat4size,
  double *data5,
  int dat5size,
  double *data6,
  int dat6size,
  double *data7,
  int dat7size,
  double *data8,
  int dat8size,
  double *data9,
  int dat9size,
  double *data10,
  int dat10size,
  double *data11,
  int dat11size,
  double *data12,
  int dat12size,
  double *data13,
  int dat13size,
  double *data14,
  int dat14size,
  double *data15,
  int dat15size,
  double *data16,
  int dat16size,
  int count,
  int num_teams,
  int nthread){

  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size],data2[0:dat2size],data3[0:dat3size],data4[0:dat4size],data5[0:dat5size],data6[0:dat6size],data7[0:dat7size],data8[0:dat8size],data9[0:dat9size],data10[0:dat10size],data11[0:dat11size],data12[0:dat12size],data13[0:dat13size],data14[0:dat14size],data15[0:dat15size],data16[0:dat16size]) \
    map(to: gaussW_g_ompkernel[:7], gFInterp0_g_ompkernel[:105], gFInterp1_g_ompkernel[:105], gFInterp2_g_ompkernel[:105])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int n_op=0; n_op<count; n_op++ ){
    //variable mapping
    const double *tau = &data0[3*n_op];
    const double *sJ = &data1[21*n_op];
    const double *mD0 = &data2[105*n_op];
    double *f0_0 = &data3[105*n_op];
    double *f0_1 = &data4[105*n_op];
    double *f0_2 = &data5[105*n_op];
    const double *mD1 = &data6[105*n_op];
    double *f1_0 = &data7[105*n_op];
    double *f1_1 = &data8[105*n_op];
    double *f1_2 = &data9[105*n_op];
    const double *mD2 = &data10[105*n_op];
    double *f2_0 = &data11[105*n_op];
    double *f2_1 = &data12[105*n_op];
    double *f2_2 = &data13[105*n_op];
    double *pDy0 = &data14[105*n_op];
    double *pDy1 = &data15[105*n_op];
    double *pDy2 = &data16[105*n_op];

    //inline function
    

    for(int ind = 0; ind < 7 * 15; ind++) {
      int indT = ((ind * 15) % (15 * 7)) + (ind / 7);
      f0_0[ind] = gFInterp0_g_ompkernel[indT];
      f0_1[ind] = gFInterp0_g_ompkernel[indT];
      f0_2[ind] = mD0[indT];
    }

    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 7; n++) {
        int ind  = m * 7 + n;
        f0_0[ind] = gaussW_g_ompkernel[n] * sJ[n] * tau[0] * f0_0[ind];
        f0_1[ind] = gaussW_g_ompkernel[n] * sJ[n] * f0_1[ind];
        f0_2[ind] = gaussW_g_ompkernel[n] * sJ[n] * f0_2[ind];
      }
    }

    for(int ind = 0; ind < 7 * 15; ind++) {
      int indT = ((ind * 15) % (15 * 7)) + (ind / 7);
      f1_0[ind] = gFInterp1_g_ompkernel[indT];
      f1_1[ind] = gFInterp1_g_ompkernel[indT];
      f1_2[ind] = mD1[indT];
    }

    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 7; n++) {
        int ind = m * 7 + n;
        f1_0[ind] = gaussW_g_ompkernel[n] * sJ[n + 7] * tau[1] * f1_0[ind];
        f1_1[ind] = gaussW_g_ompkernel[n] * sJ[n + 7] * f1_1[ind];
        f1_2[ind] = gaussW_g_ompkernel[n] * sJ[n + 7] * f1_2[ind];
      }
    }

    for(int ind = 0; ind < 7 * 15; ind++) {
      int indT = ((ind * 15) % (15 * 7)) + (ind / 7);
      f2_0[ind] = gFInterp2_g_ompkernel[indT];
      f2_1[ind] = gFInterp2_g_ompkernel[indT];
      f2_2[ind] = mD2[indT];
    }

    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 7; n++) {
        int ind = m * 7 + n;
        f2_0[ind] = gaussW_g_ompkernel[n] * sJ[n + 14] * tau[2] * f2_0[ind];
        f2_1[ind] = gaussW_g_ompkernel[n] * sJ[n + 14] * f2_1[ind];
        f2_2[ind] = gaussW_g_ompkernel[n] * sJ[n + 14] * f2_2[ind];
      }
    }

    for(int i = 0; i < 7 * 15; i++) {
      pDy0[i] = 0.0;
      pDy1[i] = 0.0;
      pDy2[i] = 0.0;
    }
    //end inline func
  }

}
