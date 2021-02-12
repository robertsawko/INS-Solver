//
// auto-generated by op2.py
//

//user function
#include "../kernels/init_grid.h"

// host stub function
void op_par_loop_init_grid(char const *name, op_set set,
  op_arg arg0,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7,
  op_arg arg8,
  op_arg arg9,
  op_arg arg10,
  op_arg arg11,
  op_arg arg12,
  op_arg arg13,
  op_arg arg14,
  op_arg arg15,
  op_arg arg16,
  op_arg arg17){

  int nargs = 18;
  op_arg args[18];

  arg0.idx = 0;
  args[0] = arg0;
  for ( int v=1; v<3; v++ ){
    args[0 + v] = op_arg_dat(arg0.dat, v, arg0.map, 2, "double", OP_READ);
  }

  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;
  args[8] = arg8;
  args[9] = arg9;
  args[10] = arg10;
  args[11] = arg11;
  args[12] = arg12;
  args[13] = arg13;
  args[14] = arg14;
  args[15] = arg15;
  args[16] = arg16;
  args[17] = arg17;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(0);
  op_timers_core(&cpu_t1, &wall_t1);

  if (OP_diags>2) {
    printf(" kernel routine with indirection: init_grid\n");
  }

  int set_size = op_mpi_halo_exchanges(set, nargs, args);

  if (set_size >0) {

    for ( int n=0; n<set_size; n++ ){
      if (n==set->core_size) {
        op_mpi_wait_all(nargs, args);
      }
      int map0idx;
      int map1idx;
      int map2idx;
      map0idx = arg0.map_data[n * arg0.map->dim + 0];
      map1idx = arg0.map_data[n * arg0.map->dim + 1];
      map2idx = arg0.map_data[n * arg0.map->dim + 2];

      const double* arg0_vec[] = {
         &((double*)arg0.data)[2 * map0idx],
         &((double*)arg0.data)[2 * map1idx],
         &((double*)arg0.data)[2 * map2idx]};

      init_grid(
        arg0_vec,
        &((double*)arg3.data)[3 * n],
        &((double*)arg4.data)[3 * n],
        &((double*)arg5.data)[15 * n],
        &((double*)arg6.data)[15 * n],
        &((double*)arg7.data)[15 * n],
        &((double*)arg8.data)[15 * n],
        &((double*)arg9.data)[15 * n],
        &((double*)arg10.data)[15 * n],
        &((double*)arg11.data)[15 * n],
        &((double*)arg12.data)[15 * n],
        &((double*)arg13.data)[15 * n],
        &((double*)arg14.data)[15 * n],
        &((double*)arg15.data)[15 * n],
        &((double*)arg16.data)[15 * n],
        &((double*)arg17.data)[15 * n]);
    }
  }

  if (set_size == 0 || set_size == set->core_size) {
    op_mpi_wait_all(nargs, args);
  }
  // combine reduction data
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[0].name      = name;
  OP_kernels[0].count    += 1;
  OP_kernels[0].time     += wall_t2 - wall_t1;
  OP_kernels[0].transfer += (float)set->size * arg0.size;
  OP_kernels[0].transfer += (float)set->size * arg3.size;
  OP_kernels[0].transfer += (float)set->size * arg4.size;
  OP_kernels[0].transfer += (float)set->size * arg5.size;
  OP_kernels[0].transfer += (float)set->size * arg6.size;
  OP_kernels[0].transfer += (float)set->size * arg7.size;
  OP_kernels[0].transfer += (float)set->size * arg8.size;
  OP_kernels[0].transfer += (float)set->size * arg9.size;
  OP_kernels[0].transfer += (float)set->size * arg10.size;
  OP_kernels[0].transfer += (float)set->size * arg11.size;
  OP_kernels[0].transfer += (float)set->size * arg12.size;
  OP_kernels[0].transfer += (float)set->size * arg13.size;
  OP_kernels[0].transfer += (float)set->size * arg14.size;
  OP_kernels[0].transfer += (float)set->size * arg15.size;
  OP_kernels[0].transfer += (float)set->size * arg16.size;
  OP_kernels[0].transfer += (float)set->size * arg17.size;
  OP_kernels[0].transfer += (float)set->size * arg0.map->dim * 4.0f;
}
