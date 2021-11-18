//
// auto-generated by op2.py
//

//user function
#include "../kernels/viscosity_bc.h"

// host stub function
void op_par_loop_viscosity_bc(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7,
  op_arg arg8,
  op_arg arg9){

  int nargs = 10;
  op_arg args[10];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;
  args[8] = arg8;
  args[9] = arg9;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(44);
  op_timers_core(&cpu_t1, &wall_t1);

  if (OP_diags>2) {
    printf(" kernel routine with indirection: viscosity_bc\n");
  }

  int set_size = op_mpi_halo_exchanges(set, nargs, args);

  if (set_size > 0) {

    for ( int n=0; n<set_size; n++ ){
      if (n<set->core_size && n>0 && n % OP_mpi_test_frequency == 0)
        op_mpi_test_all(nargs,args);
      if (n==set->core_size) {
        op_mpi_wait_all(nargs, args);
      }
      int map4idx;
      map4idx = arg4.map_data[n * arg4.map->dim + 0];


      viscosity_bc(
        &((int*)arg0.data)[1 * n],
        &((int*)arg1.data)[1 * n],
        (double*)arg2.data,
        (int*)arg3.data,
        &((double*)arg4.data)[21 * map4idx],
        &((double*)arg5.data)[21 * map4idx],
        &((double*)arg6.data)[21 * map4idx],
        &((double*)arg7.data)[21 * map4idx],
        &((double*)arg8.data)[21 * map4idx],
        &((double*)arg9.data)[21 * map4idx]);
    }
  }

  if (set_size == 0 || set_size == set->core_size) {
    op_mpi_wait_all(nargs, args);
  }
  // combine reduction data
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[44].name      = name;
  OP_kernels[44].count    += 1;
  OP_kernels[44].time     += wall_t2 - wall_t1;
  OP_kernels[44].transfer += (float)set->size * arg4.size;
  OP_kernels[44].transfer += (float)set->size * arg5.size;
  OP_kernels[44].transfer += (float)set->size * arg6.size;
  OP_kernels[44].transfer += (float)set->size * arg7.size;
  OP_kernels[44].transfer += (float)set->size * arg8.size * 2.0f;
  OP_kernels[44].transfer += (float)set->size * arg9.size * 2.0f;
  OP_kernels[44].transfer += (float)set->size * arg0.size;
  OP_kernels[44].transfer += (float)set->size * arg1.size;
  OP_kernels[44].transfer += (float)set->size * arg2.size;
  OP_kernels[44].transfer += (float)set->size * arg3.size;
  OP_kernels[44].transfer += (float)set->size * arg4.map->dim * 4.0f;
}
