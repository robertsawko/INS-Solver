#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_init_gauss(cublasHandle_t handle, const int numCells,
                              const double *x_d, const double *y_d, double *gxr_d,
                              double *gxs_d, double *gyr_d, double *gys_d) {
  double *dVMdr0_d;
  cudaMalloc((void**)&dVMdr0_d, 7 * 15 * sizeof(double));
  double *dVMdr1_d;
  cudaMalloc((void**)&dVMdr1_d, 7 * 15 * sizeof(double));
  double *dVMdr2_d;
  cudaMalloc((void**)&dVMdr2_d, 7 * 15 * sizeof(double));

  double *dVMds0_d;
  cudaMalloc((void**)&dVMds0_d, 7 * 15 * sizeof(double));
  double *dVMds1_d;
  cudaMalloc((void**)&dVMds1_d, 7 * 15 * sizeof(double));
  double *dVMds2_d;
  cudaMalloc((void**)&dVMds2_d, 7 * 15 * sizeof(double));

  // double *Dr_d;
  // cudaMalloc((void**)&Dr_d, 15 * 15 * sizeof(double));
  // cudaMemcpy(Dr_d, Dr, 15 * 15 * sizeof(double), cudaMemcpyHostToDevice);
  //
  // double *Ds_d;
  // cudaMalloc((void**)&Ds_d, 15 * 15 * sizeof(double));
  // cudaMemcpy(Ds_d, Ds, 15 * 15 * sizeof(double), cudaMemcpyHostToDevice);
  //
  // double *gFInterp0_d;
  // cudaMalloc((void**)&gFInterp0_d, 7 * 15 * sizeof(double));
  // cudaMemcpy(gFInterp0_d, gFInterp0, 7 * 15 * sizeof(double), cudaMemcpyHostToDevice);
  // double *gFInterp1_d;
  // cudaMalloc((void**)&gFInterp1_d, 7 * 15 * sizeof(double));
  // cudaMemcpy(gFInterp1_d, gFInterp1, 7 * 15 * sizeof(double), cudaMemcpyHostToDevice);
  // double *gFInterp2_d;
  // cudaMalloc((void**)&gFInterp2_d, 7 * 15 * sizeof(double));
  // cudaMemcpy(gFInterp2_d, gFInterp2, 7 * 15 * sizeof(double), cudaMemcpyHostToDevice);

  double alpha = 1.0;
  double beta = 0.0;
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 7, 15, 15, &alpha, constants->gFInterp0_d, 15, constants->Dr_d, 15, &beta, dVMdr0_d, 7);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 7, 15, 15, &alpha, constants->gFInterp1_d, 15, constants->Dr_d, 15, &beta, dVMdr1_d, 7);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 7, 15, 15, &alpha, constants->gFInterp2_d, 15, constants->Dr_d, 15, &beta, dVMdr2_d, 7);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 7, 15, 15, &alpha, constants->gFInterp0_d, 15, constants->Ds_d, 15, &beta, dVMds0_d, 7);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 7, 15, 15, &alpha, constants->gFInterp1_d, 15, constants->Ds_d, 15, &beta, dVMds1_d, 7);
  cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 7, 15, 15, &alpha, constants->gFInterp2_d, 15, constants->Ds_d, 15, &beta, dVMds2_d, 7);

  for(int c = 0; c < numCells; c++) {
    const double *x = x_d + c * 15;
    const double *y = y_d + c * 15;
    double *gxr = gxr_d + c * 21;
    double *gxs = gxs_d + c * 21;
    double *gyr = gyr_d + c * 21;
    double *gys = gys_d + c * 21;

    // Face 0
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMdr0_d, 7, x, 1, &beta, gxr, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMds0_d, 7, x, 1, &beta, gxs, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMdr0_d, 7, y, 1, &beta, gyr, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMds0_d, 7, y, 1, &beta, gys, 1);

    // Face 1
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMdr1_d, 7, x, 1, &beta, gxr + 7, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMds1_d, 7, x, 1, &beta, gxs + 7, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMdr1_d, 7, y, 1, &beta, gyr + 7, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMds1_d, 7, y, 1, &beta, gys + 7, 1);

    // Face 2
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMdr2_d, 7, x, 1, &beta, gxr + 14, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMds2_d, 7, x, 1, &beta, gxs + 14, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMdr2_d, 7, y, 1, &beta, gyr + 14, 1);
    cublasDgemv(handle, CUBLAS_OP_N, 7, 15, &alpha, dVMds2_d, 7, y, 1, &beta, gys + 14, 1);
  }

  // cudaFree(gFInterp0_d);
  // cudaFree(gFInterp1_d);
  // cudaFree(gFInterp2_d);
  // cudaFree(Dr_d);
  // cudaFree(Ds_d);
  cudaFree(dVMdr0_d);
  cudaFree(dVMdr1_d);
  cudaFree(dVMdr2_d);
  cudaFree(dVMds0_d);
  cudaFree(dVMds1_d);
  cudaFree(dVMds2_d);
}

void init_gauss_blas(INSData *nsData, GaussData *gaussData) {
  // Initialise cuBLAS
  // cublasHandle_t handle;
  // cublasCreate(&handle);
  // cublasSetPointerMode(handle, CUBLAS_POINTER_MODE_HOST);
  // Make sure OP2 data is in the right place
  op_arg init_gauss_args[] = {
    op_arg_dat(nsData->x, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(nsData->y, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(gaussData->rx, -1, OP_ID, 21, "double", OP_WRITE),
    op_arg_dat(gaussData->sx, -1, OP_ID, 21, "double", OP_WRITE),
    op_arg_dat(gaussData->ry, -1, OP_ID, 21, "double", OP_WRITE),
    op_arg_dat(gaussData->sy, -1, OP_ID, 21, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 6, init_gauss_args);

  cublas_init_gauss(constants->handle, nsData->numCells, (double *)nsData->x->data_d,
                   (double *)nsData->y->data_d, (double *)gaussData->rx->data_d,
                   (double *)gaussData->sx->data_d, (double *)gaussData->ry->data_d,
                   (double *)gaussData->sy->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(6, init_gauss_args);
  // Free resources used by cuBLAS
  // cublasDestroy(handle);
}
