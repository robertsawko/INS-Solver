//
// auto-generated by op2.py
//

//user function
#include "../kernels/poisson_rhs_du.h"

// host stub function
void op_par_loop_poisson_rhs_du(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7){

  int nargs = 8;
  op_arg args[8];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(15);
  op_timers_core(&cpu_t1, &wall_t1);


  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  poisson_rhs_du");
  }

  int set_size = op_mpi_halo_exchanges(set, nargs, args);

  if (set_size >0) {

    for ( int n=0; n<set_size; n++ ){
      poisson_rhs_du(
        &((double*)arg0.data)[15*n],
        &((double*)arg1.data)[15*n],
        &((double*)arg2.data)[15*n],
        &((double*)arg3.data)[15*n],
        &((double*)arg4.data)[15*n],
        &((double*)arg5.data)[15*n],
        &((double*)arg6.data)[15*n],
        &((double*)arg7.data)[15*n]);
    }
  }

  // combine reduction data
  op_mpi_set_dirtybit(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[15].name      = name;
  OP_kernels[15].count    += 1;
  OP_kernels[15].time     += wall_t2 - wall_t1;
  OP_kernels[15].transfer += (float)set->size * arg0.size;
  OP_kernels[15].transfer += (float)set->size * arg1.size;
  OP_kernels[15].transfer += (float)set->size * arg2.size;
  OP_kernels[15].transfer += (float)set->size * arg3.size;
  OP_kernels[15].transfer += (float)set->size * arg4.size * 2.0f;
  OP_kernels[15].transfer += (float)set->size * arg5.size * 2.0f;
  OP_kernels[15].transfer += (float)set->size * arg6.size * 2.0f;
  OP_kernels[15].transfer += (float)set->size * arg7.size * 2.0f;
}
