//
// auto-generated by op2.py
//

//user function
//user function
//#pragma acc routine
inline void viscosity_set_bc_openacc( double *qtt0, double *qtt1, double *exQ0, double *exQ1) {
  for(int i = 0; i < 15; i++) {


    exQ0[i] = 0.0;
    exQ1[i] = 0.0;
  }
}

// host stub function
void op_par_loop_viscosity_set_bc(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3){

  int nargs = 4;
  op_arg args[4];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(16);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[16].name      = name;
  OP_kernels[16].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  viscosity_set_bc");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);


  if (set_size >0) {


    //Set up typed device pointers for OpenACC

    double* data0 = (double*)arg0.data_d;
    double* data1 = (double*)arg1.data_d;
    double* data2 = (double*)arg2.data_d;
    double* data3 = (double*)arg3.data_d;
    #pragma acc parallel loop independent deviceptr(data0,data1,data2,data3)
    for ( int n=0; n<set->size; n++ ){
      viscosity_set_bc_openacc(
        &data0[15*n],
        &data1[15*n],
        &data2[15*n],
        &data3[15*n]);
    }
  }

  // combine reduction data
  op_mpi_set_dirtybit_cuda(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[16].time     += wall_t2 - wall_t1;
  OP_kernels[16].transfer += (float)set->size * arg0.size * 2.0f;
  OP_kernels[16].transfer += (float)set->size * arg1.size * 2.0f;
  OP_kernels[16].transfer += (float)set->size * arg2.size * 2.0f;
  OP_kernels[16].transfer += (float)set->size * arg3.size * 2.0f;
}
