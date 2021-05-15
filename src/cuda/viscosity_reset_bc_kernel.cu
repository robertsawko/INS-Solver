//
// auto-generated by op2.py
//

//user function
__device__ void viscosity_reset_bc_gpu( double *exQ0, double *exQ1) {
  for(int i = 0; i < 21; i++) {
    exQ0[i] = 0.0;
    exQ1[i] = 0.0;
  }

}

// CUDA kernel function
__global__ void op_cuda_viscosity_reset_bc(
  double *arg0,
  double *arg1,
  int   set_size ) {


  //process set elements
  for ( int n=threadIdx.x+blockIdx.x*blockDim.x; n<set_size; n+=blockDim.x*gridDim.x ){

    //user-supplied kernel call
    viscosity_reset_bc_gpu(arg0+n*21,
                       arg1+n*21);
  }
}


//host stub function
void op_par_loop_viscosity_reset_bc(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1){

  int nargs = 2;
  op_arg args[2];

  args[0] = arg0;
  args[1] = arg1;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(47);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[47].name      = name;
  OP_kernels[47].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  viscosity_reset_bc");
  }

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set_size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_47
      int nthread = OP_BLOCK_SIZE_47;
    #else
      int nthread = OP_block_size;
    #endif

    int nblocks = 200;

    op_cuda_viscosity_reset_bc<<<nblocks,nthread>>>(
      (double *) arg0.data_d,
      (double *) arg1.data_d,
      set->size );
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[47].time     += wall_t2 - wall_t1;
  OP_kernels[47].transfer += (float)set->size * arg0.size * 2.0f;
  OP_kernels[47].transfer += (float)set->size * arg1.size * 2.0f;
}
