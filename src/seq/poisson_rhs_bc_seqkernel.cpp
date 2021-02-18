//
// auto-generated by op2.py
//

//user function
#include "../kernels/poisson_rhs_bc.h"

// host stub function
void op_par_loop_poisson_rhs_bc(char const *name, op_set set,
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
  op_timing_realloc(16);
  op_timers_core(&cpu_t1, &wall_t1);

  if (OP_diags>2) {
    printf(" kernel routine with indirection: poisson_rhs_bc\n");
  }

  int set_size = op_mpi_halo_exchanges(set, nargs, args);

  if (set_size >0) {

    for ( int n=0; n<set_size; n++ ){
      if (n==set->core_size) {
        op_mpi_wait_all(nargs, args);
      }
      int map3idx;
      map3idx = arg3.map_data[n * arg3.map->dim + 0];


      poisson_rhs_bc(
        &((int*)arg0.data)[1 * n],
        &((int*)arg1.data)[1 * n],
        (int*)arg2.data,
        &((double*)arg3.data)[15 * map3idx],
        &((double*)arg4.data)[15 * map3idx]);
    }
  }

  if (set_size == 0 || set_size == set->core_size) {
    op_mpi_wait_all(nargs, args);
  }
  // combine reduction data
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[16].name      = name;
  OP_kernels[16].count    += 1;
  OP_kernels[16].time     += wall_t2 - wall_t1;
  OP_kernels[16].transfer += (float)set->size * arg3.size;
  OP_kernels[16].transfer += (float)set->size * arg4.size * 2.0f;
  OP_kernels[16].transfer += (float)set->size * arg0.size;
  OP_kernels[16].transfer += (float)set->size * arg1.size;
  OP_kernels[16].transfer += (float)set->size * arg2.size;
  OP_kernels[16].transfer += (float)set->size * arg3.map->dim * 4.0f;
}
