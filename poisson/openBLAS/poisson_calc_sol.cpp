#include "cblas.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void openblas_poisson_calc_sol(const int numCells, double *sol) {
  double *temp = (double *)malloc(15 * numCells * sizeof(double));
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 15, numCells, 15, 1.0, invM, 15, sol, 15, 0.0, temp, 15);
  memcpy(sol, temp, 15 * numCells * sizeof(double));
  free(temp);
}

void poisson_calc_sol_blas(INSData *nsData) {
  // Make sure OP2 data is in the right place
  op_arg poisson_sol_args[] = {
    op_arg_dat(nsData->sol, -1, OP_ID, 15, "double", OP_RW)
  };
  op_mpi_halo_exchanges(nsData->cells, 1, poisson_sol_args);

  openblas_poisson_calc_sol(nsData->numCells, (double *)nsData->sol->data);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit(1, poisson_sol_args);
}