#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_poisson_rhs2(cublasHandle_t handle, const int numCells,
                        const double *fluxQ_d, double *divQ_d,
                        double *rhs_d) {
  double *LIFT_d;
  cudaMalloc((void**)&LIFT_d, 15 * 15 * sizeof(double));
  cudaMemcpy(LIFT_d, LIFT, 15 * 15 * sizeof(double), cudaMemcpyHostToDevice);

  double *MASS_d;
  cudaMalloc((void**)&MASS_d, 15 * 15 * sizeof(double));
  cudaMemcpy(MASS_d, MASS, 15 * 15 * sizeof(double), cudaMemcpyHostToDevice);

  // CUBLAS_OP_T because cublas is column major but constants are stored row major
  double alpha = -1.0;
  double beta = 1.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha, LIFT_d, 15, fluxQ_d, 15, &beta, divQ_d, 15);
  double alpha2 = 1.0;
  double beta2 = 0.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha2, MASS_d, 15, divQ_d, 15, &beta2, rhs_d, 15);


  cudaFree(LIFT_d);
  cudaFree(MASS_d);
}

void poisson_rhs_blas2(INSData *nsData) {
  // Initialise cuBLAS
  cublasHandle_t handle;
  cublasCreate(&handle);
  cublasSetPointerMode(handle, CUBLAS_POINTER_MODE_HOST);
  // Make sure OP2 data is in the right place
  op_arg poisson_rhs2_args[] = {
    op_arg_dat(nsData->pFluxQ, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(nsData->pDivQ, -1, OP_ID, 15, "double", OP_RW),
    op_arg_dat(nsData->pRHSU, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 3, poisson_rhs2_args);

  cublas_poisson_rhs2(handle, nsData->numCells, (double *)nsData->pFluxQ->data_d,
                   (double *)nsData->pDivQ->data_d, (double *)nsData->pRHSU->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(3, poisson_rhs2_args);
  // Free resources used by cuBLAS
  cublasDestroy(handle);
}