//
// auto-generated by op2.py
//

//user function
__device__ void init_nodes_gpu( const double **nc, double *nodeX, double *nodeY) {
  nodeX[0] = nc[0][0];
  nodeX[1] = nc[1][0];
  nodeX[2] = nc[2][0];
  nodeY[0] = nc[0][1];
  nodeY[1] = nc[1][1];
  nodeY[2] = nc[2][1];

}

// CUDA kernel function
__global__ void op_cuda_init_nodes(
  const double *__restrict ind_arg0,
  const int *__restrict opDat0Map,
  double *arg3,
  double *arg4,
  int start,
  int end,
  int   set_size) {
  int tid = threadIdx.x + blockIdx.x * blockDim.x;
  if (tid + start < end) {
    int n = tid + start;
    //initialise local variables
    int map0idx;
    int map1idx;
    int map2idx;
    map0idx = opDat0Map[n + set_size * 0];
    map1idx = opDat0Map[n + set_size * 1];
    map2idx = opDat0Map[n + set_size * 2];
    const double* arg0_vec[] = {
       &ind_arg0[2 * map0idx],
       &ind_arg0[2 * map1idx],
       &ind_arg0[2 * map2idx]};

    //user-supplied kernel call
    init_nodes_gpu(arg0_vec,
               arg3+n*3,
               arg4+n*3);
  }
}


//host stub function
void op_par_loop_init_nodes(char const *name, op_set set,
  op_arg arg0,
  op_arg arg3,
  op_arg arg4){

  int nargs = 5;
  op_arg args[5];

  arg0.idx = 0;
  args[0] = arg0;
  for ( int v=1; v<3; v++ ){
    args[0 + v] = op_arg_dat(arg0.dat, v, arg0.map, 2, "double", OP_READ);
  }

  args[3] = arg3;
  args[4] = arg4;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(0);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[0].name      = name;
  OP_kernels[0].count    += 1;


  int    ninds   = 1;
  int    inds[5] = {0,0,0,-1,-1};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: init_nodes\n");
  }
  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);
  if (set_size > 0) {

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_0
      int nthread = OP_BLOCK_SIZE_0;
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
        op_cuda_init_nodes<<<nblocks,nthread>>>(
        (double *)arg0.data_d,
        arg0.map_data_d,
        (double*)arg3.data_d,
        (double*)arg4.data_d,
        start,end,set->size+set->exec_size);
      }
    }
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[0].time     += wall_t2 - wall_t1;
}
