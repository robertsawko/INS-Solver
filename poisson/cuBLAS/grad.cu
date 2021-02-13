#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_grad(cublasHandle_t handle, const int numCells, const double *u_d,
                        double *div0_d, double *div1_d) {
  double *Dr_d;
  cudaMalloc((void**)&Dr_d, 15 * 15 * sizeof(double));
  cudaMemcpy(Dr_d, Dr, 15 * 15 * sizeof(double), cudaMemcpyHostToDevice);

  double *Ds_d;
  cudaMalloc((void**)&Ds_d, 15 * 15 * sizeof(double));
  cudaMemcpy(Ds_d, Ds, 15 * 15 * sizeof(double), cudaMemcpyHostToDevice);

  // CUBLAS_OP_T because cublas is column major but constants are stored row major
  double alpha = 1.0;
  double beta = 0.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha, Dr_d, 15, u_d, 15, &beta, div0_d, 15);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_N, 15, numCells, 15, &alpha, Ds_d, 15, u_d, 15, &beta, div1_d, 15);

  cudaFree(Dr_d);
  cudaFree(Ds_d);
}

void grad_blas(INSData *nsData, op_dat u) {
  // Initialise cuBLAS
  cublasHandle_t handle;
  cublasCreate(&handle);
  cublasSetPointerMode(handle, CUBLAS_POINTER_MODE_HOST);
  // Make sure OP2 data is in the right place
  op_arg grad_args[] = {
    op_arg_dat(u, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(nsData->div[0], -1, OP_ID, 15, "double", OP_WRITE),
    op_arg_dat(nsData->div[1], -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 3, grad_args);

  cublas_grad(handle, nsData->numCells, (double *)u->data_d,
              (double *)nsData->div[0]->data_d, (double *)nsData->div[1]->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(3, grad_args);
  // Free resources used by cuBLAS
  cublasDestroy(handle);
}