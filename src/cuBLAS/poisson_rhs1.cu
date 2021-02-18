#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_poisson_rhs1(cublasHandle_t handle, const int numCells,
                        const double *fluxXu_d, const double *fluxYu_d,
                        double *qx_d, double *qy_d) {
  double *LIFT_d;
  cudaMalloc((void**)&LIFT_d, 15 * 15 * sizeof(double));
  cudaMemcpy(LIFT_d, LIFT, 15 * 15 * sizeof(double), cudaMemcpyHostToDevice);

  // CUBLAS_OP_T because cublas is column major but constants are stored row major
  double alpha = -1.0;
  double beta = 1.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha, LIFT_d, 15, fluxXu_d, 15, &beta, qx_d, 15);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha, LIFT_d, 15, fluxYu_d, 15, &beta, qy_d, 15);

  cudaFree(LIFT_d);
}

void poisson_rhs_blas1(INSData *nsData, Poisson *pData) {
  // Initialise cuBLAS
  cublasHandle_t handle;
  cublasCreate(&handle);
  cublasSetPointerMode(handle, CUBLAS_POINTER_MODE_HOST);
  // Make sure OP2 data is in the right place
  op_arg poisson_rhs1_args[] = {
    op_arg_dat(pData->pFluxXu, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(pData->pFluxYu, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(pData->pDuDx, -1, OP_ID, 15, "double", OP_RW),
    op_arg_dat(pData->pDuDy, -1, OP_ID, 15, "double", OP_RW)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 4, poisson_rhs1_args);

  cublas_poisson_rhs1(handle, nsData->numCells, (double *)pData->pFluxXu->data_d,
                   (double *)pData->pFluxYu->data_d, (double *)pData->pDuDx->data_d,
                   (double *)pData->pDuDy->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(4, poisson_rhs1_args);
  // Free resources used by cuBLAS
  cublasDestroy(handle);
}