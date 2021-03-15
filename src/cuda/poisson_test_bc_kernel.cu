//
// auto-generated by op2.py
//

//user function
__device__ void poisson_test_bc_gpu( const int *bedge_type, const int *bedgeNum,
                            const double *x, const double *y, double *dBC) {
  int exInd = 0;
  if(*bedgeNum == 1) {
    exInd = 7;
  } else if(*bedgeNum == 2) {
    exInd = 2 * 7;
  }

  if(*bedge_type == 0) {
    for(int i = 0; i < 7; i++) {
      double y1 = y[exInd + i];
      dBC[exInd + i] += y1 * (1.0 - y1);

    }
  }

}

// CUDA kernel function
__global__ void op_cuda_poisson_test_bc(
  const double *__restrict ind_arg0,
  const double *__restrict ind_arg1,
  double *__restrict ind_arg2,
  const int *__restrict opDat2Map,
  const int *__restrict arg0,
  const int *__restrict arg1,
  int start,
  int end,
  int   set_size) {
  double arg4_l[21];
  int tid = threadIdx.x + blockIdx.x * blockDim.x;
  if (tid + start < end) {
    int n = tid + start;
    //initialise local variables
    double arg4_l[21];
    for ( int d=0; d<21; d++ ){
      arg4_l[d] = ZERO_double;
    }
    int map2idx;
    map2idx = opDat2Map[n + set_size * 0];

    //user-supplied kernel call
    poisson_test_bc_gpu(arg0+n*1,
                    arg1+n*1,
                    ind_arg0+map2idx*21,
                    ind_arg1+map2idx*21,
                    arg4_l);
    atomicAdd(&ind_arg2[0+map2idx*21],arg4_l[0]);
    atomicAdd(&ind_arg2[1+map2idx*21],arg4_l[1]);
    atomicAdd(&ind_arg2[2+map2idx*21],arg4_l[2]);
    atomicAdd(&ind_arg2[3+map2idx*21],arg4_l[3]);
    atomicAdd(&ind_arg2[4+map2idx*21],arg4_l[4]);
    atomicAdd(&ind_arg2[5+map2idx*21],arg4_l[5]);
    atomicAdd(&ind_arg2[6+map2idx*21],arg4_l[6]);
    atomicAdd(&ind_arg2[7+map2idx*21],arg4_l[7]);
    atomicAdd(&ind_arg2[8+map2idx*21],arg4_l[8]);
    atomicAdd(&ind_arg2[9+map2idx*21],arg4_l[9]);
    atomicAdd(&ind_arg2[10+map2idx*21],arg4_l[10]);
    atomicAdd(&ind_arg2[11+map2idx*21],arg4_l[11]);
    atomicAdd(&ind_arg2[12+map2idx*21],arg4_l[12]);
    atomicAdd(&ind_arg2[13+map2idx*21],arg4_l[13]);
    atomicAdd(&ind_arg2[14+map2idx*21],arg4_l[14]);
    atomicAdd(&ind_arg2[15+map2idx*21],arg4_l[15]);
    atomicAdd(&ind_arg2[16+map2idx*21],arg4_l[16]);
    atomicAdd(&ind_arg2[17+map2idx*21],arg4_l[17]);
    atomicAdd(&ind_arg2[18+map2idx*21],arg4_l[18]);
    atomicAdd(&ind_arg2[19+map2idx*21],arg4_l[19]);
    atomicAdd(&ind_arg2[20+map2idx*21],arg4_l[20]);
  }
}


//host stub function
void op_par_loop_poisson_test_bc(char const *name, op_set set,
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
  op_timing_realloc(34);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[34].name      = name;
  OP_kernels[34].count    += 1;


  int    ninds   = 3;
  int    inds[5] = {-1,-1,0,1,2};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: poisson_test_bc\n");
  }
  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set_size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_34
      int nthread = OP_BLOCK_SIZE_34;
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
        op_cuda_poisson_test_bc<<<nblocks,nthread>>>(
        (double *)arg2.data_d,
        (double *)arg3.data_d,
        (double *)arg4.data_d,
        arg2.map_data_d,
        (int*)arg0.data_d,
        (int*)arg1.data_d,
        start,end,set->size+set->exec_size);
      }
    }
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[34].time     += wall_t2 - wall_t1;
}
