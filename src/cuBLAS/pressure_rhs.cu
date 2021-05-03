#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_pressure_rhs(cublasHandle_t handle, const int numCells,
                                double *div_d, const double *dPdN_d,
                                double *rhs_d) {
  // CUBLAS_OP_T because cublas is column major but constants are stored row major
  double alpha = 1.0;
  double beta1 = 1.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha, constants->lift_d, 15, dPdN_d, 15, &beta1, div_d, 15);
  double beta2 = 0.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha, constants->mass_d, 15, div_d, 15, &beta2, rhs_d, 15);
}

void pressure_rhs_blas(INSData *nsData, int ind) {
  // Make sure OP2 data is in the right place
  op_arg pressure_rhs_args[] = {
    op_arg_dat(nsData->divVelT, -1, OP_ID, 15, "double", OP_RW),
    op_arg_dat(nsData->dPdN[(ind + 1) % 2], -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(nsData->pRHS, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 3, pressure_rhs_args);

  cublas_pressure_rhs(constants->handle, nsData->numCells, (double *)nsData->divVelT->data_d,
                      (double *)nsData->dPdN[(ind + 1) % 2]->data_d,
                      (double *)nsData->pRHS->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(3, pressure_rhs_args);
}
