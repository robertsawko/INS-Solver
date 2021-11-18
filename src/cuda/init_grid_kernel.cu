//
// auto-generated by op2.py
//

//user function
__device__ void init_grid_gpu( double *rx, double *ry, double *sx, double *sy,
                      double *nx, double *ny, double *J, double *sJ, double *fscale) {


  for(int i = 0; i < 5; i++) {
    nx[i] = ry[FMASK_cuda[i]];
    ny[i] = -rx[FMASK_cuda[i]];
  }

  for(int i = 0; i < 5; i++) {
    nx[5 + i] = sy[FMASK_cuda[5 + i]] - ry[FMASK_cuda[5 + i]];
    ny[5 + i] = rx[FMASK_cuda[5 + i]] - sx[FMASK_cuda[5 + i]];
  }

  for(int i = 0; i < 5; i++) {
    nx[2 * 5 + i] = -sy[FMASK_cuda[2 * 5 + i]];
    ny[2 * 5 + i] = sx[FMASK_cuda[2 * 5 + i]];
  }

  for(int i = 0; i < 15; i++) {
    J[i] = -sx[i] * ry[i] + rx[i] * sy[i];
  }

  for(int i = 0; i < 15; i++) {
    double rx_n = sy[i] / J[i];
    double sx_n = -ry[i] / J[i];
    double ry_n = -sx[i] / J[i];
    double sy_n = rx[i] / J[i];
    rx[i] = rx_n;
    sx[i] = sx_n;
    ry[i] = ry_n;
    sy[i] = sy_n;
  }

  for(int i = 0; i < 3 * 5; i++) {
    sJ[i] = sqrt(nx[i] * nx[i] + ny[i] * ny[i]);
    nx[i] = nx[i] / sJ[i];
    ny[i] = ny[i] / sJ[i];
    fscale[i] = sJ[i] / J[FMASK_cuda[i]];
  }

}

// CUDA kernel function
__global__ void op_cuda_init_grid(
  double *arg0,
  double *arg1,
  double *arg2,
  double *arg3,
  double *arg4,
  double *arg5,
  double *arg6,
  double *arg7,
  double *arg8,
  int   set_size ) {


  //process set elements
  for ( int n=threadIdx.x+blockIdx.x*blockDim.x; n<set_size; n+=blockDim.x*gridDim.x ){

    //user-supplied kernel call
    init_grid_gpu(arg0+n*15,
              arg1+n*15,
              arg2+n*15,
              arg3+n*15,
              arg4+n*15,
              arg5+n*15,
              arg6+n*15,
              arg7+n*15,
              arg8+n*15);
  }
}


//host stub function
void op_par_loop_init_grid(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7,
  op_arg arg8){

  int nargs = 9;
  op_arg args[9];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;
  args[8] = arg8;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(1);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[1].name      = name;
  OP_kernels[1].count    += 1;


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  init_grid");
  }

  int set_size = op_mpi_halo_exchanges_grouped(set, nargs, args, 2);
  if (set_size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_1
      int nthread = OP_BLOCK_SIZE_1;
    #else
      int nthread = OP_block_size;
    #endif

    int nblocks = 200;

    op_cuda_init_grid<<<nblocks,nthread>>>(
      (double *) arg0.data_d,
      (double *) arg1.data_d,
      (double *) arg2.data_d,
      (double *) arg3.data_d,
      (double *) arg4.data_d,
      (double *) arg5.data_d,
      (double *) arg6.data_d,
      (double *) arg7.data_d,
      (double *) arg8.data_d,
      set->size );
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[1].time     += wall_t2 - wall_t1;
  OP_kernels[1].transfer += (float)set->size * arg0.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg1.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg2.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg3.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg4.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg5.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg6.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg7.size * 2.0f;
  OP_kernels[1].transfer += (float)set->size * arg8.size * 2.0f;
}
