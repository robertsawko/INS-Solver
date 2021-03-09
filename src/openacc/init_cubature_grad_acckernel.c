//
// auto-generated by op2.py
//

//user function
//user function
//#pragma acc routine
inline void init_cubature_grad_openacc( double *rx, double *sx, double *ry,  double *sy,
                               double *Dx, double *Dy) {

  double J[46];
  for(int i = 0; i < 46; i++) {
    J[i] = -sx[i] * ry[i] + rx[i] * sy[i];
  }

  for(int i = 0; i < 46; i++) {
    double rx_n = sy[i] / J[i];
    double sx_n = -ry[i] / J[i];
    double ry_n = -sx[i] / J[i];
    double sy_n = rx[i] / J[i];
    rx[i] = rx_n;
    sx[i] = sx_n;
    ry[i] = ry_n;
    sy[i] = sy_n;
  }

  for(int m = 0; m < 46; m++) {
    for(int n = 0; n < 15; n++) {
      int ind = m * 15 + n;
      Dx[ind] = rx[m] * cubVDr[ind] + sx[m] * cubVDs[ind];
      Dy[ind] = ry[m] * cubVDr[ind] + sy[m] * cubVDs[ind];
    }
  }
}

// host stub function
void op_par_loop_init_cubature_grad(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5){

  int nargs = 6;
  op_arg args[6];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(13);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[13].name      = name;
  OP_kernels[13].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  init_cubature_grad");
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
    #pragma acc parallel loop independent deviceptr(data0,data1,data2,data3,data4,data5)
    for ( int n=0; n<set->size; n++ ){
      init_cubature_grad_openacc(
        &data0[46*n],
        &data1[46*n],
        &data2[46*n],
        &data3[46*n],
        &data4[690*n],
        &data5[690*n]);
    }
  }

  // combine reduction data
  op_mpi_set_dirtybit_cuda(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[13].time     += wall_t2 - wall_t1;
  OP_kernels[13].transfer += (float)set->size * arg0.size * 2.0f;
  OP_kernels[13].transfer += (float)set->size * arg1.size * 2.0f;
  OP_kernels[13].transfer += (float)set->size * arg2.size * 2.0f;
  OP_kernels[13].transfer += (float)set->size * arg3.size * 2.0f;
  OP_kernels[13].transfer += (float)set->size * arg4.size * 2.0f;
  OP_kernels[13].transfer += (float)set->size * arg5.size * 2.0f;
}
