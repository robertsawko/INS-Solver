//
// auto-generated by op2.py
//

//user function
//user function
//#pragma acc routine
inline void pRHS_J_openacc( const double *J, double *rhs) {
  for(int i = 0; i < 15; i++) {
    rhs[i] *= J[i];
  }
}

// host stub function
void op_par_loop_pRHS_J(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1){

  int nargs = 2;
  op_arg args[2];

  args[0] = arg0;
  args[1] = arg1;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(14);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[14].name      = name;
  OP_kernels[14].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  pRHS_J");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);


  if (set_size >0) {


    //Set up typed device pointers for OpenACC

    double* data0 = (double*)arg0.data_d;
    double* data1 = (double*)arg1.data_d;
    #pragma acc parallel loop independent deviceptr(data0,data1)
    for ( int n=0; n<set->size; n++ ){
      pRHS_J_openacc(
        &data0[15*n],
        &data1[15*n]);
    }
  }

  // combine reduction data
  op_mpi_set_dirtybit_cuda(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[14].time     += wall_t2 - wall_t1;
  OP_kernels[14].transfer += (float)set->size * arg0.size;
  OP_kernels[14].transfer += (float)set->size * arg1.size * 2.0f;
}
