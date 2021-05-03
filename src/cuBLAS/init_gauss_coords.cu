#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_gauss_coords(cublasHandle_t handle, const int numCells,
                                const double *x_d, const double *y_d,
                                double *gx_d, double *gy_d) {
  // CUBLAS_OP_T because cublas is column major but constants are stored row major
  double alpha = 1.0;
  double beta = 0.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 21, numCells, 15, &alpha, constants->gInterp_d, 15, x_d, 15, &beta, gx_d, 21);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 21, numCells, 15, &alpha, constants->gInterp_d, 15, y_d, 15, &beta, gy_d, 21);
}

void init_gauss_coords_blas(INSData *nsData, GaussData *gaussData) {
  // Make sure OP2 data is in the right place
  op_arg gauss_args[] = {
    op_arg_dat(nsData->x, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(nsData->y, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(gaussData->x, -1, OP_ID, 21, "double", OP_WRITE),
    op_arg_dat(gaussData->y, -1, OP_ID, 21, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 4, gauss_args);

  cublas_gauss_coords(constants->handle, nsData->numCells, (double *)nsData->x->data_d,
                      (double *)nsData->y->data_d, (double *)gaussData->x->data_d,
                      (double *)gaussData->y->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(4, gauss_args);
}
