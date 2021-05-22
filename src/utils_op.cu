//
// auto-generated by op2.py
//

#include  "op_lib_cpp.h"

//
// op_par_loop declarations
//
#ifdef OPENACC
#ifdef __cplusplus
extern "C" {
#endif
#endif
#ifdef OPENACC
#ifdef __cplusplus
}
#endif
#endif


#include <memory>

double *getOP2Array(op_dat dat) {
  op_arg args[] = {
    op_arg_dat(dat, -1, OP_ID, 16, "double", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(dat->set, 1, args);
  double *res = (double *)malloc(dat->set->size * 16 * sizeof(double));
  cudaMemcpy(res, dat->data_d, dat->set->size * 16 * sizeof(double), cudaMemcpyDeviceToHost);
  op_mpi_set_dirtybit_cuda(1, args);
  return res;
}
