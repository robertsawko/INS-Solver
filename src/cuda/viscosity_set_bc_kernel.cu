//
// auto-generated by op2.py
//

//user function
__device__ void viscosity_set_bc_gpu( double *qtt0, double *qtt1, double *exQ0, double *exQ1) {
  for(int i = 0; i < 15; i++) {
    qtt0[FMASK_cuda[i]] = exQ0[i];
    qtt1[FMASK_cuda[i]] = exQ1[i];
    exQ0[i] = 0.0;
    exQ1[i] = 0.0;
  }

}

// CUDA kernel function
__global__ void op_cuda_viscosity_set_bc(
  double *arg0,
  double *arg1,
  double *arg2,
  double *arg3,
  int   set_size ) {


  //process set elements
  for ( int n=threadIdx.x+blockIdx.x*blockDim.x; n<set_size; n+=blockDim.x*gridDim.x ){

    //user-supplied kernel call
    viscosity_set_bc_gpu(arg0+n*15,
                     arg1+n*15,
                     arg2+n*15,
                     arg3+n*15);
  }
}


//host stub function
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
  op_timing_realloc(14);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[14].name      = name;
  OP_kernels[14].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  viscosity_set_bc");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set_size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_14
      int nthread = OP_BLOCK_SIZE_14;
    #else
      int nthread = OP_block_size;
    #endif

    int nblocks = 200;

    op_cuda_viscosity_set_bc<<<nblocks,nthread>>>(
      (double *) arg0.data_d,
      (double *) arg1.data_d,
      (double *) arg2.data_d,
      (double *) arg3.data_d,
      set->size );
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[14].time     += wall_t2 - wall_t1;
  OP_kernels[14].transfer += (float)set->size * arg0.size * 2.0f;
  OP_kernels[14].transfer += (float)set->size * arg1.size * 2.0f;
  OP_kernels[14].transfer += (float)set->size * arg2.size * 2.0f;
  OP_kernels[14].transfer += (float)set->size * arg3.size * 2.0f;
}