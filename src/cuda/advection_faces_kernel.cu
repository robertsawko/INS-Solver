//
// auto-generated by op2.py
//

//user function
__device__ void advection_faces_gpu( const int *edgeNum, const double **x,
                            const double **y, const double **q0,
                            const double **q1, double **exQ0, double **exQ1) {

  int edgeL = edgeNum[0];
  int edgeR = edgeNum[1];
  bool reverse;

  if(edgeR == 0) {
    if(edgeL == 0) {
      reverse = !(x[0][0] == x[1][0] && y[0][0] == y[1][0]);
    } else if(edgeL == 1) {
      reverse = !(x[0][1] == x[1][0] && y[0][1] == y[1][0]);
    } else {
      reverse = !(x[0][2] == x[1][0] && y[0][2] == y[1][0]);
    }
  } else if(edgeR == 1) {
    if(edgeL == 0) {
      reverse = !(x[0][0] == x[1][1] && y[0][0] == y[1][1]);
    } else if(edgeL == 1) {
      reverse = !(x[0][1] == x[1][1] && y[0][1] == y[1][1]);
    } else {
      reverse = !(x[0][2] == x[1][1] && y[0][2] == y[1][1]);
    }
  } else {
    if(edgeL == 0) {
      reverse = !(x[0][0] == x[1][2] && y[0][0] == y[1][2]);
    } else if(edgeL == 1) {
      reverse = !(x[0][1] == x[1][2] && y[0][1] == y[1][2]);
    } else {
      reverse = !(x[0][2] == x[1][2] && y[0][2] == y[1][2]);
    }
  }

  int exInd = 0;
  if(edgeL == 1) exInd = 5;
  else if(edgeL == 2) exInd = 2 * 5;

  int *fmask;

  if(edgeR == 0) {
    fmask = FMASK_cuda;
  } else if(edgeR == 1) {
    fmask = &FMASK_cuda[5];
  } else {
    fmask = &FMASK_cuda[2 * 5];
  }

  for(int i = 0; i < 5; i++) {
    int rInd;
    if(reverse) {
      rInd = fmask[5 - i - 1];
    } else {
      rInd = fmask[i];
    }
    exQ0[0][exInd + i] += q0[1][rInd];
    exQ1[0][exInd + i] += q1[1][rInd];
  }

  exInd = 0;
  if(edgeR == 1) exInd = 5;
  else if(edgeR == 2) exInd = 2 * 5;

  if(edgeL == 0) {
    fmask = FMASK_cuda;
  } else if(edgeL == 1) {
    fmask = &FMASK_cuda[5];
  } else {
    fmask = &FMASK_cuda[2 * 5];
  }

  for(int i = 0; i < 5; i++) {
    int lInd;
    if(reverse) {
      lInd = fmask[5 - i - 1];
    } else {
      lInd = fmask[i];
    }
    exQ0[1][exInd + i] += q0[0][lInd];
    exQ1[1][exInd + i] += q1[0][lInd];
  }

}

// CUDA kernel function
__global__ void op_cuda_advection_faces(
  const double *__restrict ind_arg0,
  const double *__restrict ind_arg1,
  const double *__restrict ind_arg2,
  const double *__restrict ind_arg3,
  double *__restrict ind_arg4,
  double *__restrict ind_arg5,
  const int *__restrict opDat1Map,
  const int *__restrict arg0,
  int start,
  int end,
  int   set_size) {
  double arg9_l[15];
  double arg10_l[15];
  double arg11_l[15];
  double arg12_l[15];
  double *arg9_vec[2] = {
    arg9_l,
    arg10_l,
  };
  double *arg11_vec[2] = {
    arg11_l,
    arg12_l,
  };
  int tid = threadIdx.x + blockIdx.x * blockDim.x;
  if (tid + start < end) {
    int n = tid + start;
    //initialise local variables
    double arg9_l[15];
    for ( int d=0; d<15; d++ ){
      arg9_l[d] = ZERO_double;
    }
    double arg10_l[15];
    for ( int d=0; d<15; d++ ){
      arg10_l[d] = ZERO_double;
    }
    double arg11_l[15];
    for ( int d=0; d<15; d++ ){
      arg11_l[d] = ZERO_double;
    }
    double arg12_l[15];
    for ( int d=0; d<15; d++ ){
      arg12_l[d] = ZERO_double;
    }
    int map1idx;
    int map2idx;
    map1idx = opDat1Map[n + set_size * 0];
    map2idx = opDat1Map[n + set_size * 1];
    const double* arg1_vec[] = {
       &ind_arg0[3 * map1idx],
       &ind_arg0[3 * map2idx]};
    const double* arg3_vec[] = {
       &ind_arg1[3 * map1idx],
       &ind_arg1[3 * map2idx]};
    const double* arg5_vec[] = {
       &ind_arg2[15 * map1idx],
       &ind_arg2[15 * map2idx]};
    const double* arg7_vec[] = {
       &ind_arg3[15 * map1idx],
       &ind_arg3[15 * map2idx]};
    double* arg9_vec[] = {
       &ind_arg4[15 * map1idx],
       &ind_arg4[15 * map2idx]};
    double* arg11_vec[] = {
       &ind_arg5[15 * map1idx],
       &ind_arg5[15 * map2idx]};

    //user-supplied kernel call
    advection_faces_gpu(arg0+n*2,
                    arg1_vec,
                    arg3_vec,
                    arg5_vec,
                    arg7_vec,
                    arg9_vec,
                    arg11_vec);
    atomicAdd(&ind_arg4[0+map1idx*15],arg9_l[0]);
    atomicAdd(&ind_arg4[1+map1idx*15],arg9_l[1]);
    atomicAdd(&ind_arg4[2+map1idx*15],arg9_l[2]);
    atomicAdd(&ind_arg4[3+map1idx*15],arg9_l[3]);
    atomicAdd(&ind_arg4[4+map1idx*15],arg9_l[4]);
    atomicAdd(&ind_arg4[5+map1idx*15],arg9_l[5]);
    atomicAdd(&ind_arg4[6+map1idx*15],arg9_l[6]);
    atomicAdd(&ind_arg4[7+map1idx*15],arg9_l[7]);
    atomicAdd(&ind_arg4[8+map1idx*15],arg9_l[8]);
    atomicAdd(&ind_arg4[9+map1idx*15],arg9_l[9]);
    atomicAdd(&ind_arg4[10+map1idx*15],arg9_l[10]);
    atomicAdd(&ind_arg4[11+map1idx*15],arg9_l[11]);
    atomicAdd(&ind_arg4[12+map1idx*15],arg9_l[12]);
    atomicAdd(&ind_arg4[13+map1idx*15],arg9_l[13]);
    atomicAdd(&ind_arg4[14+map1idx*15],arg9_l[14]);
    atomicAdd(&ind_arg4[0+map2idx*15],arg10_l[0]);
    atomicAdd(&ind_arg4[1+map2idx*15],arg10_l[1]);
    atomicAdd(&ind_arg4[2+map2idx*15],arg10_l[2]);
    atomicAdd(&ind_arg4[3+map2idx*15],arg10_l[3]);
    atomicAdd(&ind_arg4[4+map2idx*15],arg10_l[4]);
    atomicAdd(&ind_arg4[5+map2idx*15],arg10_l[5]);
    atomicAdd(&ind_arg4[6+map2idx*15],arg10_l[6]);
    atomicAdd(&ind_arg4[7+map2idx*15],arg10_l[7]);
    atomicAdd(&ind_arg4[8+map2idx*15],arg10_l[8]);
    atomicAdd(&ind_arg4[9+map2idx*15],arg10_l[9]);
    atomicAdd(&ind_arg4[10+map2idx*15],arg10_l[10]);
    atomicAdd(&ind_arg4[11+map2idx*15],arg10_l[11]);
    atomicAdd(&ind_arg4[12+map2idx*15],arg10_l[12]);
    atomicAdd(&ind_arg4[13+map2idx*15],arg10_l[13]);
    atomicAdd(&ind_arg4[14+map2idx*15],arg10_l[14]);
    atomicAdd(&ind_arg5[0+map1idx*15],arg11_l[0]);
    atomicAdd(&ind_arg5[1+map1idx*15],arg11_l[1]);
    atomicAdd(&ind_arg5[2+map1idx*15],arg11_l[2]);
    atomicAdd(&ind_arg5[3+map1idx*15],arg11_l[3]);
    atomicAdd(&ind_arg5[4+map1idx*15],arg11_l[4]);
    atomicAdd(&ind_arg5[5+map1idx*15],arg11_l[5]);
    atomicAdd(&ind_arg5[6+map1idx*15],arg11_l[6]);
    atomicAdd(&ind_arg5[7+map1idx*15],arg11_l[7]);
    atomicAdd(&ind_arg5[8+map1idx*15],arg11_l[8]);
    atomicAdd(&ind_arg5[9+map1idx*15],arg11_l[9]);
    atomicAdd(&ind_arg5[10+map1idx*15],arg11_l[10]);
    atomicAdd(&ind_arg5[11+map1idx*15],arg11_l[11]);
    atomicAdd(&ind_arg5[12+map1idx*15],arg11_l[12]);
    atomicAdd(&ind_arg5[13+map1idx*15],arg11_l[13]);
    atomicAdd(&ind_arg5[14+map1idx*15],arg11_l[14]);
    atomicAdd(&ind_arg5[0+map2idx*15],arg12_l[0]);
    atomicAdd(&ind_arg5[1+map2idx*15],arg12_l[1]);
    atomicAdd(&ind_arg5[2+map2idx*15],arg12_l[2]);
    atomicAdd(&ind_arg5[3+map2idx*15],arg12_l[3]);
    atomicAdd(&ind_arg5[4+map2idx*15],arg12_l[4]);
    atomicAdd(&ind_arg5[5+map2idx*15],arg12_l[5]);
    atomicAdd(&ind_arg5[6+map2idx*15],arg12_l[6]);
    atomicAdd(&ind_arg5[7+map2idx*15],arg12_l[7]);
    atomicAdd(&ind_arg5[8+map2idx*15],arg12_l[8]);
    atomicAdd(&ind_arg5[9+map2idx*15],arg12_l[9]);
    atomicAdd(&ind_arg5[10+map2idx*15],arg12_l[10]);
    atomicAdd(&ind_arg5[11+map2idx*15],arg12_l[11]);
    atomicAdd(&ind_arg5[12+map2idx*15],arg12_l[12]);
    atomicAdd(&ind_arg5[13+map2idx*15],arg12_l[13]);
    atomicAdd(&ind_arg5[14+map2idx*15],arg12_l[14]);
  }
}


//host stub function
void op_par_loop_advection_faces(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg3,
  op_arg arg5,
  op_arg arg7,
  op_arg arg9,
  op_arg arg11){

  int nargs = 13;
  op_arg args[13];

  args[0] = arg0;
  arg1.idx = 0;
  args[1] = arg1;
  for ( int v=1; v<2; v++ ){
    args[1 + v] = op_arg_dat(arg1.dat, v, arg1.map, 3, "double", OP_READ);
  }

  arg3.idx = 0;
  args[3] = arg3;
  for ( int v=1; v<2; v++ ){
    args[3 + v] = op_arg_dat(arg3.dat, v, arg3.map, 3, "double", OP_READ);
  }

  arg5.idx = 0;
  args[5] = arg5;
  for ( int v=1; v<2; v++ ){
    args[5 + v] = op_arg_dat(arg5.dat, v, arg5.map, 15, "double", OP_READ);
  }

  arg7.idx = 0;
  args[7] = arg7;
  for ( int v=1; v<2; v++ ){
    args[7 + v] = op_arg_dat(arg7.dat, v, arg7.map, 15, "double", OP_READ);
  }

  arg9.idx = 0;
  args[9] = arg9;
  for ( int v=1; v<2; v++ ){
    args[9 + v] = op_arg_dat(arg9.dat, v, arg9.map, 15, "double", OP_INC);
  }

  arg11.idx = 0;
  args[11] = arg11;
  for ( int v=1; v<2; v++ ){
    args[11 + v] = op_arg_dat(arg11.dat, v, arg11.map, 15, "double", OP_INC);
  }


  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(5);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[5].name      = name;
  OP_kernels[5].count    += 1;


  int    ninds   = 6;
  int    inds[13] = {-1,0,0,1,1,2,2,3,3,4,4,5,5};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: advection_faces\n");
  }
  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set_size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_5
      int nthread = OP_BLOCK_SIZE_5;
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
        op_cuda_advection_faces<<<nblocks,nthread>>>(
        (double *)arg1.data_d,
        (double *)arg3.data_d,
        (double *)arg5.data_d,
        (double *)arg7.data_d,
        (double *)arg9.data_d,
        (double *)arg11.data_d,
        arg1.map_data_d,
        (int*)arg0.data_d,
        start,end,set->size+set->exec_size);
      }
    }
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[5].time     += wall_t2 - wall_t1;
}
