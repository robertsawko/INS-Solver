//
// auto-generated by op2.py
//

//user function
#include "../kernels/calc_dt.h"

// host stub function
void op_par_loop_calc_dt(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2){

  int nargs = 3;
  op_arg args[3];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(46);
  op_timers_core(&cpu_t1, &wall_t1);


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  calc_dt");
  }

  int set_size = op_mpi_halo_exchanges(set, nargs, args);

  if (set_size >0) {

    for ( int n=0; n<set_size; n++ ){
      calc_dt(
        &((double*)arg0.data)[3*n],
        &((double*)arg1.data)[3*n],
        (double*)arg2.data);
    }
  }

  // combine reduction data
  op_mpi_reduce_double(&arg2,(double*)arg2.data);
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[46].name      = name;
  OP_kernels[46].count    += 1;
  OP_kernels[46].time     += wall_t2 - wall_t1;
  OP_kernels[46].transfer += (float)set->size * arg0.size;
  OP_kernels[46].transfer += (float)set->size * arg1.size;
}
