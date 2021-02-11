//
// auto-generated by op2.py
//

//user function
//user function
//#pragma acc routine
inline void set_ic_openacc( double *q0, double *q1, double *q2, double *exQ0,
                   double *exQ1) {
  for(int i = 0; i < 15; i++) {
    q0[i] = bc_u;
    q1[i] = bc_v;
    q2[i] = bc_p;
    exQ0[i] = 0.0;
    exQ1[i] = 0.0;
  }
}

// host stub function
void op_par_loop_set_ic(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4){

  int nargs = 5;
  op_arg args[5];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(1);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[1].name      = name;
  OP_kernels[1].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  set_ic");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);


  if (set_size >0) {


    //Set up typed device pointers for OpenACC

    double* data0 = (double*)arg0.data_d;
    double* data1 = (double*)arg1.data_d;
    double* data2 = (double*)arg2.data_d;
    double* data3 = (double*)arg3.data_d;
    double* data4 = (double*)arg4.data_d;
    #pragma acc parallel loop independent deviceptr(data0,data1,data2,data3,data4)
    for ( int n=0; n<set->size; n++ ){
      set_ic_openacc(
        &data0[15*n],
        &data1[15*n],
        &data2[15*n],
        &data3[15*n],
        &data4[15*n]);
    }
  }

  // combine reduction data
  op_mpi_set_dirtybit_cuda(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[1].time     += wall_t2 - wall_t1;
  OP_kernels[1].transfer += (float)set->size * arg0.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg1.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg2.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg3.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg4.size * 2.0f;
}
