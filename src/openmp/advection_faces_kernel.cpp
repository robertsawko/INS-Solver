//
// auto-generated by op2.py
//

//user function
#include "../kernels/advection_faces.h"

// host stub function
void op_par_loop_advection_faces(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg4,
  op_arg arg6,
  op_arg arg8){

  int nargs = 10;
  op_arg args[10];

  args[0] = arg0;
  args[1] = arg1;
  arg2.idx = 0;
  args[2] = arg2;
  for ( int v=1; v<2; v++ ){
    args[2 + v] = op_arg_dat(arg2.dat, v, arg2.map, 15, "double", OP_READ);
  }

  arg4.idx = 0;
  args[4] = arg4;
  for ( int v=1; v<2; v++ ){
    args[4 + v] = op_arg_dat(arg4.dat, v, arg4.map, 15, "double", OP_READ);
  }

  arg6.idx = 0;
  args[6] = arg6;
  for ( int v=1; v<2; v++ ){
    args[6 + v] = op_arg_dat(arg6.dat, v, arg6.map, 15, "double", OP_INC);
  }

  arg8.idx = 0;
  args[8] = arg8;
  for ( int v=1; v<2; v++ ){
    args[8 + v] = op_arg_dat(arg8.dat, v, arg8.map, 15, "double", OP_INC);
  }


  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(36);
  OP_kernels[36].name      = name;
  OP_kernels[36].count    += 1;
  op_timers_core(&cpu_t1, &wall_t1);

  int  ninds   = 4;
  int  inds[10] = {-1,-1,0,0,1,1,2,2,3,3};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: advection_faces\n");
  }

  // get plan
  #ifdef OP_PART_SIZE_36
    int part_size = OP_PART_SIZE_36;
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
          int map2idx;
          int map3idx;
          map2idx = arg2.map_data[n * arg2.map->dim + 0];
          map3idx = arg2.map_data[n * arg2.map->dim + 1];

          const double* arg2_vec[] = {
             &((double*)arg2.data)[15 * map2idx],
             &((double*)arg2.data)[15 * map3idx]};
          const double* arg4_vec[] = {
             &((double*)arg4.data)[15 * map2idx],
             &((double*)arg4.data)[15 * map3idx]};
          double* arg6_vec[] = {
             &((double*)arg6.data)[15 * map2idx],
             &((double*)arg6.data)[15 * map3idx]};
          double* arg8_vec[] = {
             &((double*)arg8.data)[15 * map2idx],
             &((double*)arg8.data)[15 * map3idx]};

          advection_faces(
            &((int*)arg0.data)[2 * n],
            &((bool*)arg1.data)[1 * n],
            arg2_vec,
            arg4_vec,
            arg6_vec,
            arg8_vec);
        }
      }

      block_offset += nblocks;
    }
    OP_kernels[36].transfer  += Plan->transfer;
    OP_kernels[36].transfer2 += Plan->transfer2;
  }

  if (set_size == 0 || set_size == set->core_size) {
    op_mpi_wait_all(nargs, args);
  }
  // combine reduction data
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[36].time     += wall_t2 - wall_t1;
}
