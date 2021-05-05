//
// auto-generated by op2.py
//

//user function
//user function
//#pragma acc routine
inline void poisson_mf2_op_openacc( const double *cub_op, const double *tol, double *op1) {
  for(int m = 0; m < 15; m++) {
    for(int n = 0; n < 15; n++) {
      int ind = m * 15 + n;
      int colInd = n * 15 + m;
      if(fabs(cub_op[colInd]) > *tol) {
        op1[ind] = cub_op[colInd];
      }
    }
  }
}

// host stub function
void op_par_loop_poisson_mf2_op(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2){

  double*arg1h = (double *)arg1.data;
  int nargs = 3;
  op_arg args[3];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(52);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[52].name      = name;
  OP_kernels[52].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  poisson_mf2_op");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);

  double arg1_l = arg1h[0];

  if (set_size >0) {


    //Set up typed device pointers for OpenACC

    double* data0 = (double*)arg0.data_d;
    double* data2 = (double*)arg2.data_d;
    #pragma acc parallel loop independent deviceptr(data0,data2)
    for ( int n=0; n<set->size; n++ ){
      poisson_mf2_op_openacc(
        &data0[225*n],
        &arg1_l,
        &data2[225*n]);
    }
  }

  // combine reduction data
  op_mpi_set_dirtybit_cuda(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[52].time     += wall_t2 - wall_t1;
  OP_kernels[52].transfer += (float)set->size * arg0.size;
  OP_kernels[52].transfer += (float)set->size * arg2.size * 2.0f;
}