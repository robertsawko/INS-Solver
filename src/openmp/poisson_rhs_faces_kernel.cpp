//
// auto-generated by op2.py
//

//user function
#include "../kernels/poisson_rhs_faces.h"

// host stub function
void op_par_loop_poisson_rhs_faces(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg3,
  op_arg arg5,
  op_arg arg7){

  int nargs = 9;
  op_arg args[9];

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
    args[7 + v] = op_arg_dat(arg7.dat, v, arg7.map, 15, "double", OP_INC);
  }


  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(15);
  OP_kernels[15].name      = name;
  OP_kernels[15].count    += 1;
  op_timers_core(&cpu_t1, &wall_t1);

  int  ninds   = 4;
  int  inds[9] = {-1,0,0,1,1,2,2,3,3};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: poisson_rhs_faces\n");
  }

  // get plan
  #ifdef OP_PART_SIZE_15
    int part_size = OP_PART_SIZE_15;
  #else
    int part_size = OP_part_size;
  #endif

  int set_size = op_mpi_halo_exchanges(set, nargs, args);

  if (set_size >0) {

    op_plan *Plan = op_plan_get_stage_upload(name,set,part_size,nargs,args,ninds,inds,OP_STAGE_ALL,0);

    // execute plan
    int block_offset = 0;
    for ( int col=0; col<Plan->ncolors; col++ ){
      if (col==Plan->ncolors_core) {
        op_mpi_wait_all(nargs, args);
      }
      int nblocks = Plan->ncolblk[col];

      #pragma omp parallel for
      for ( int blockIdx=0; blockIdx<nblocks; blockIdx++ ){
        int blockId  = Plan->blkmap[blockIdx + block_offset];
        int nelem    = Plan->nelems[blockId];
        int offset_b = Plan->offset[blockId];
        for ( int n=offset_b; n<offset_b+nelem; n++ ){
          int map1idx;
          int map2idx;
          map1idx = arg1.map_data[n * arg1.map->dim + 0];
          map2idx = arg1.map_data[n * arg1.map->dim + 1];

          const double* arg1_vec[] = {
             &((double*)arg1.data)[3 * map1idx],
             &((double*)arg1.data)[3 * map2idx]};
          const double* arg3_vec[] = {
             &((double*)arg3.data)[3 * map1idx],
             &((double*)arg3.data)[3 * map2idx]};
          const double* arg5_vec[] = {
             &((double*)arg5.data)[15 * map1idx],
             &((double*)arg5.data)[15 * map2idx]};
          double* arg7_vec[] = {
             &((double*)arg7.data)[15 * map1idx],
             &((double*)arg7.data)[15 * map2idx]};

          poisson_rhs_faces(
            &((int*)arg0.data)[2 * n],
            arg1_vec,
            arg3_vec,
            arg5_vec,
            arg7_vec);
        }
      }

      block_offset += nblocks;
    }
    OP_kernels[15].transfer  += Plan->transfer;
    OP_kernels[15].transfer2 += Plan->transfer2;
  }

  if (set_size == 0 || set_size == set->core_size) {
    op_mpi_wait_all(nargs, args);
  }
  // combine reduction data
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[15].time     += wall_t2 - wall_t1;
}
