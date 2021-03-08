//
// auto-generated by op2.py
//

//user function
//user function
//#pragma acc routine
inline void gauss_op_openacc( const double *tau, const double *sJ,
                     const double *mD0, double *f0_0, double *f0_1, double *f0_2,
                     const double *mD1, double *f1_0, double *f1_1, double *f1_2,
                     const double *mD2, double *f2_0, double *f2_1, double *f2_2) {

  for(int m = 0; m < 7; m++) {
    for(int n = 0; n < 15; n++) {
      int ind = m * 15 + n;
      f0_0[ind] = gaussW[m] * sJ[m] * tau[0] * gFInterp0[ind];
      f0_1[ind] = gaussW[m] * sJ[m] * gFInterp0[ind];
      f0_2[ind] = gaussW[m] * sJ[m] * mD0[ind];
    }
  }

  for(int m = 0; m < 7; m++) {
    for(int n = 0; n < 15; n++) {
      int ind = m * 15 + n;
      f1_0[ind] = gaussW[m] * sJ[m + 7] * tau[1] * gFInterp1[ind];
      f1_1[ind] = gaussW[m] * sJ[m + 7] * gFInterp1[ind];
      f1_2[ind] = gaussW[m] * sJ[m + 7] * mD1[ind];
    }
  }

  for(int m = 0; m < 7; m++) {
    for(int n = 0; n < 15; n++) {
      int ind = m * 15 + n;
      f2_0[ind] = gaussW[m] * sJ[m + 14] * tau[2] * gFInterp2[ind];
      f2_1[ind] = gaussW[m] * sJ[m + 14] * gFInterp2[ind];
      f2_2[ind] = gaussW[m] * sJ[m + 14] * mD2[ind];
    }
  }
}

// host stub function
void op_par_loop_gauss_op(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7,
  op_arg arg8,
  op_arg arg9,
  op_arg arg10,
  op_arg arg11,
  op_arg arg12,
  op_arg arg13){

  int nargs = 14;
  op_arg args[14];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;
  args[8] = arg8;
  args[9] = arg9;
  args[10] = arg10;
  args[11] = arg11;
  args[12] = arg12;
  args[13] = arg13;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(22);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[22].name      = name;
  OP_kernels[22].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  gauss_op");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);


  if (set_size >0) {


    //Set up typed device pointers for OpenACC

    double* data0 = (double*)arg0.data_d;
    double* data1 = (double*)arg1.data_d;
    double* data2 = (double*)arg2.data_d;
    double* data3 = (double*)arg3.data_d;
    double* data4 = (double*)arg4.data_d;
    double* data5 = (double*)arg5.data_d;
    double* data6 = (double*)arg6.data_d;
    double* data7 = (double*)arg7.data_d;
    double* data8 = (double*)arg8.data_d;
    double* data9 = (double*)arg9.data_d;
    double* data10 = (double*)arg10.data_d;
    double* data11 = (double*)arg11.data_d;
    double* data12 = (double*)arg12.data_d;
    double* data13 = (double*)arg13.data_d;
    #pragma acc parallel loop independent deviceptr(data0,data1,data2,data3,data4,data5,data6,data7,data8,data9,data10,data11,data12,data13)
    for ( int n=0; n<set->size; n++ ){
      gauss_op_openacc(
        &data0[3*n],
        &data1[21*n],
        &data2[105*n],
        &data3[105*n],
        &data4[105*n],
        &data5[105*n],
        &data6[105*n],
        &data7[105*n],
        &data8[105*n],
        &data9[105*n],
        &data10[105*n],
        &data11[105*n],
        &data12[105*n],
        &data13[105*n]);
    }
  }

  // combine reduction data
  op_mpi_set_dirtybit_cuda(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[22].time     += wall_t2 - wall_t1;
  OP_kernels[22].transfer += (float)set->size * arg0.size;
  OP_kernels[22].transfer += (float)set->size * arg1.size;
  OP_kernels[22].transfer += (float)set->size * arg2.size;
  OP_kernels[22].transfer += (float)set->size * arg3.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg4.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg5.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg6.size;
  OP_kernels[22].transfer += (float)set->size * arg7.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg8.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg9.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg10.size;
  OP_kernels[22].transfer += (float)set->size * arg11.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg12.size * 2.0f;
  OP_kernels[22].transfer += (float)set->size * arg13.size * 2.0f;
}
