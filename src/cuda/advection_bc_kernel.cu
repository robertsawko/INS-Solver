//
// auto-generated by op2.py
//

//user function
__device__ void advection_bc_gpu( const int *bedge_type, const int *bedgeNum,
                         const double *t, const double *x, const double *y,
                         const double *q0, const double *q1, double *exQ0, double *exQ1) {
  int exInd = 0;
  if(*bedgeNum == 1) {
    exInd = 5;
  } else if(*bedgeNum == 2) {
    exInd = 2 * 5;
  }

  int *fmask;

  if(*bedgeNum == 0) {
    fmask = FMASK_cuda;
  } else if(*bedgeNum == 1) {
    fmask = &FMASK_cuda[5];
  } else {
    fmask = &FMASK_cuda[2 * 5];
  }

  if(*bedge_type == 0) {

    const double PI = 3.141592653589793238463;
    for(int i = 0; i < 5; i++) {
      int qInd = fmask[i];
      double y1 = y[qInd];
      exQ0[exInd + i] += pow(0.41, -2.0) * sin((PI * *t) / 8.0) * 6.0 * y1 * (0.41 - y1);



    }
  } else if(*bedge_type == 1) {

    for(int i = 0; i < 5; i++) {
      int qInd = fmask[i];
      exQ0[exInd + i] += q0[qInd];
      exQ1[exInd + i] += q1[qInd];
    }
  } else {






  }

}

// CUDA kernel function
__global__ void op_cuda_advection_bc(
  const double *__restrict ind_arg0,
  const double *__restrict ind_arg1,
  const double *__restrict ind_arg2,
  const double *__restrict ind_arg3,
  double *__restrict ind_arg4,
  double *__restrict ind_arg5,
  const int *__restrict opDat3Map,
  const int *__restrict arg0,
  const int *__restrict arg1,
  const double *arg2,
  int start,
  int end,
  int   set_size) {
  double arg7_l[15];
  double arg8_l[15];
  int tid = threadIdx.x + blockIdx.x * blockDim.x;
  if (tid + start < end) {
    int n = tid + start;
    //initialise local variables
    double arg7_l[15];
    for ( int d=0; d<15; d++ ){
      arg7_l[d] = ZERO_double;
    }
    double arg8_l[15];
    for ( int d=0; d<15; d++ ){
      arg8_l[d] = ZERO_double;
    }
    int map3idx;
    map3idx = opDat3Map[n + set_size * 0];

    //user-supplied kernel call
    advection_bc_gpu(arg0+n*1,
                 arg1+n*1,
                 arg2,
                 ind_arg0+map3idx*15,
                 ind_arg1+map3idx*15,
                 ind_arg2+map3idx*15,
                 ind_arg3+map3idx*15,
                 arg7_l,
                 arg8_l);
    atomicAdd(&ind_arg4[0+map3idx*15],arg7_l[0]);
    atomicAdd(&ind_arg4[1+map3idx*15],arg7_l[1]);
    atomicAdd(&ind_arg4[2+map3idx*15],arg7_l[2]);
    atomicAdd(&ind_arg4[3+map3idx*15],arg7_l[3]);
    atomicAdd(&ind_arg4[4+map3idx*15],arg7_l[4]);
    atomicAdd(&ind_arg4[5+map3idx*15],arg7_l[5]);
    atomicAdd(&ind_arg4[6+map3idx*15],arg7_l[6]);
    atomicAdd(&ind_arg4[7+map3idx*15],arg7_l[7]);
    atomicAdd(&ind_arg4[8+map3idx*15],arg7_l[8]);
    atomicAdd(&ind_arg4[9+map3idx*15],arg7_l[9]);
    atomicAdd(&ind_arg4[10+map3idx*15],arg7_l[10]);
    atomicAdd(&ind_arg4[11+map3idx*15],arg7_l[11]);
    atomicAdd(&ind_arg4[12+map3idx*15],arg7_l[12]);
    atomicAdd(&ind_arg4[13+map3idx*15],arg7_l[13]);
    atomicAdd(&ind_arg4[14+map3idx*15],arg7_l[14]);
    atomicAdd(&ind_arg5[0+map3idx*15],arg8_l[0]);
    atomicAdd(&ind_arg5[1+map3idx*15],arg8_l[1]);
    atomicAdd(&ind_arg5[2+map3idx*15],arg8_l[2]);
    atomicAdd(&ind_arg5[3+map3idx*15],arg8_l[3]);
    atomicAdd(&ind_arg5[4+map3idx*15],arg8_l[4]);
    atomicAdd(&ind_arg5[5+map3idx*15],arg8_l[5]);
    atomicAdd(&ind_arg5[6+map3idx*15],arg8_l[6]);
    atomicAdd(&ind_arg5[7+map3idx*15],arg8_l[7]);
    atomicAdd(&ind_arg5[8+map3idx*15],arg8_l[8]);
    atomicAdd(&ind_arg5[9+map3idx*15],arg8_l[9]);
    atomicAdd(&ind_arg5[10+map3idx*15],arg8_l[10]);
    atomicAdd(&ind_arg5[11+map3idx*15],arg8_l[11]);
    atomicAdd(&ind_arg5[12+map3idx*15],arg8_l[12]);
    atomicAdd(&ind_arg5[13+map3idx*15],arg8_l[13]);
    atomicAdd(&ind_arg5[14+map3idx*15],arg8_l[14]);
  }
}


//host stub function
void op_par_loop_advection_bc(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7,
  op_arg arg8){

  double*arg2h = (double *)arg2.data;
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
  op_timing_realloc(39);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[39].name      = name;
  OP_kernels[39].count    += 1;


  int    ninds   = 6;
  int    inds[9] = {-1,-1,-1,0,1,2,3,4,5};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: advection_bc\n");
  }
  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set_size > 0) {

    //transfer constants to GPU
    int consts_bytes = 0;
    consts_bytes += ROUND_UP(1*sizeof(double));
    reallocConstArrays(consts_bytes);
    consts_bytes = 0;
    arg2.data   = OP_consts_h + consts_bytes;
    arg2.data_d = OP_consts_d + consts_bytes;
    for ( int d=0; d<1; d++ ){
      ((double *)arg2.data)[d] = arg2h[d];
    }
    consts_bytes += ROUND_UP(1*sizeof(double));
    mvConstArraysToDevice(consts_bytes);

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_39
      int nthread = OP_BLOCK_SIZE_39;
    #else
      int nthread = OP_block_size;
    #endif

    for ( int round=0; round<2; round++ ){
      if (round==1) {
        op_mpi_wait_all_cuda(nargs, args);
      }
      int start = round==0 ? 0 : set->core_size;
      int end = round==0 ? set->core_size : set->size + set->exec_size;
      if (end-start>0) {
        int nblocks = (end-start-1)/nthread+1;
        op_cuda_advection_bc<<<nblocks,nthread>>>(
        (double *)arg3.data_d,
        (double *)arg4.data_d,
        (double *)arg5.data_d,
        (double *)arg6.data_d,
        (double *)arg7.data_d,
        (double *)arg8.data_d,
        arg3.map_data_d,
        (int*)arg0.data_d,
        (int*)arg1.data_d,
        (double*)arg2.data_d,
        start,end,set->size+set->exec_size);
      }
    }
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[39].time     += wall_t2 - wall_t1;
}
