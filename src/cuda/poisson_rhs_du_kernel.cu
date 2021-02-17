//
// auto-generated by op2.py
//

//user function
__device__ void poisson_rhs_du_gpu( const double *nx, const double *ny, const double *fscale,
                    const double *U, double *exU, double *du,
                    double *fluxXu, double *fluxYu) {
  for(int i = 0; i < 15; i++) {
    du[i] = U[FMASK_cuda[i]] - exU[i];
    fluxXu[i] = fscale[i] * (nx[i] * du[i] / 2.0);
    fluxYu[i] = fscale[i] * (ny[i] * du[i] / 2.0);
    exU[i] = 0.0;
  }

}

// CUDA kernel function
__global__ void op_cuda_poisson_rhs_du(
  const double *__restrict arg0,
  const double *__restrict arg1,
  const double *__restrict arg2,
  const double *__restrict arg3,
  double *arg4,
  double *arg5,
  double *arg6,
  double *arg7,
  int   set_size ) {


  //process set elements
  for ( int n=threadIdx.x+blockIdx.x*blockDim.x; n<set_size; n+=blockDim.x*gridDim.x ){

    //user-supplied kernel call
    poisson_rhs_du_gpu(arg0+n*15,
                   arg1+n*15,
                   arg2+n*15,
                   arg3+n*15,
                   arg4+n*15,
                   arg5+n*15,
                   arg6+n*15,
                   arg7+n*15);
  }
}


//host stub function
void op_par_loop_poisson_rhs_du(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7){

  int nargs = 8;
  op_arg args[8];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(17);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[17].name      = name;
  OP_kernels[17].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  poisson_rhs_du");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set_size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_17
      int nthread = OP_BLOCK_SIZE_17;
    #else
      int nthread = OP_block_size;
    #endif

    int nblocks = 200;

    op_cuda_poisson_rhs_du<<<nblocks,nthread>>>(
      (double *) arg0.data_d,
      (double *) arg1.data_d,
      (double *) arg2.data_d,
      (double *) arg3.data_d,
      (double *) arg4.data_d,
      (double *) arg5.data_d,
      (double *) arg6.data_d,
      (double *) arg7.data_d,
      set->size );
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[17].time     += wall_t2 - wall_t1;
  OP_kernels[17].transfer += (float)set->size * arg0.size;
  OP_kernels[17].transfer += (float)set->size * arg1.size;
  OP_kernels[17].transfer += (float)set->size * arg2.size;
  OP_kernels[17].transfer += (float)set->size * arg3.size;
  OP_kernels[17].transfer += (float)set->size * arg4.size * 2.0f;
  OP_kernels[17].transfer += (float)set->size * arg5.size * 2.0f;
  OP_kernels[17].transfer += (float)set->size * arg6.size * 2.0f;
  OP_kernels[17].transfer += (float)set->size * arg7.size * 2.0f;
}
