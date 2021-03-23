#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_gauss_opf(cublasHandle_t handle, const int numCells,
                            double *op_d, const double *gf_d, const double *pD_d, const double *term0_d,
                            const double *term1_d, const double *term2_d) {
  for(int c = 0; c < numCells; c++) {
    double *op = op_d + c * 15 * 15;
    const double *gf = gf_d + c * 7 * 15;
    const double *pD = pD_d + c * 7 * 15;
    const double *term0 = term0_d + c * 7 * 15;
    const double *term1 = term1_d + c * 7 * 15;
    const double *term2 = term2_d + c * 7 * 15;

    double alpha = 1.0;
    double beta = 0.0;
    cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 15, 15, 7, &alpha, term0, 7, gf, 15, &beta, op, 15);
    double alpha2 = -1.0;
    double beta2 = 1.0;
    cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 15, 15, 7, &alpha, term1, 7, pD, 15, &beta2, op, 15);
    cublasDgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, 15, 15, 7, &alpha2, term2, 7, gf, 15, &beta2, op, 15);
  }
}

void gauss_opf_blas(INSData *nsData, GaussData *gaussData) {
  // Initialise cuBLAS
  cublasHandle_t handle;
  cublasCreate(&handle);
  cublasSetPointerMode(handle, CUBLAS_POINTER_MODE_HOST);
  // Make sure OP2 data is in the right place
  op_arg gauss_args[] = {
    // Face 0
    op_arg_dat(gaussData->OPf[0], -1, OP_ID, 15 * 15, "double", OP_WRITE),
    op_arg_dat(gaussData->pDy[0], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->pD[0], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->mDx[0], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->mDx[1], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->mDx[2], -1, OP_ID, 7 * 15, "double", OP_READ),
    // Face 1
    op_arg_dat(gaussData->OPf[1], -1, OP_ID, 15 * 15, "double", OP_WRITE),
    op_arg_dat(gaussData->pDy[1], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->pD[1], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->mDy[0], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->mDy[1], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->mDy[2], -1, OP_ID, 7 * 15, "double", OP_READ),
    // Face 2
    op_arg_dat(gaussData->OPf[2], -1, OP_ID, 15 * 15, "double", OP_WRITE),
    op_arg_dat(gaussData->pDy[2], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->pD[2], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->pDx[0], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->pDx[1], -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(gaussData->pDx[2], -1, OP_ID, 7 * 15, "double", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 18, gauss_args);

  cublas_gauss_opf(handle, nsData->numCells, (double *)gaussData->OPf[0]->data_d,
                   (double *)gaussData->pDy[0]->data_d, (double *)gaussData->pD[0]->data_d, (double *)gaussData->mDx[0]->data_d,
                   (double *)gaussData->mDx[1]->data_d, (double *)gaussData->mDx[2]->data_d);

  cublas_gauss_opf(handle, nsData->numCells, (double *)gaussData->OPf[1]->data_d,
                   (double *)gaussData->pDy[1]->data_d, (double *)gaussData->pD[1]->data_d, (double *)gaussData->mDy[0]->data_d,
                   (double *)gaussData->mDy[1]->data_d, (double *)gaussData->mDy[2]->data_d);

  cublas_gauss_opf(handle, nsData->numCells, (double *)gaussData->OPf[2]->data_d,
                   (double *)gaussData->pDy[2]->data_d, (double *)gaussData->pD[2]->data_d, (double *)gaussData->pDx[0]->data_d,
                   (double *)gaussData->pDx[1]->data_d, (double *)gaussData->pDx[2]->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(18, gauss_args);
  // Free resources used by cuBLAS
  cublasDestroy(handle);
}