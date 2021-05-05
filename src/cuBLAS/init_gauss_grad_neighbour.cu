#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_init_gauss_grad_neighbour(cublasHandle_t handle, const int numCells, const int *reverse,
                        const double *x_d, const double *y_d, double *gxr_d,
                        double *gxs_d, double *gyr_d, double *gys_d) {
  // Calc Grad Matrices
  double alpha = 1.0;
  double beta = 0.0;
  for(int c = 0; c < numCells; c++) {
    const double *x = x_d + c * 15;
    const double *y = y_d + c * 15;
    double *gxr = gxr_d + c * 21;
    double *gxs = gxs_d + c * 21;
    double *gyr = gyr_d + c * 21;
    double *gys = gys_d + c * 21;

    // Face 0
    if(reverse[3 * c]) {
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0DrR_d, 15, x, 1, &beta, gxr, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0DsR_d, 15, x, 1, &beta, gxs, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0DrR_d, 15, y, 1, &beta, gyr, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0DsR_d, 15, y, 1, &beta, gys, 1);
    } else {
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0Dr_d, 15, x, 1, &beta, gxr, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0Ds_d, 15, x, 1, &beta, gxs, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0Dr_d, 15, y, 1, &beta, gyr, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF0Ds_d, 15, y, 1, &beta, gys, 1);
    }

    // Face 1
    if(reverse[3 * c + 1]) {
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1DrR_d, 15, x, 1, &beta, gxr + 7, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1DsR_d, 15, x, 1, &beta, gxs + 7, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1DrR_d, 15, y, 1, &beta, gyr + 7, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1DsR_d, 15, y, 1, &beta, gys + 7, 1);
    } else {
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1Dr_d, 15, x, 1, &beta, gxr + 7, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1Ds_d, 15, x, 1, &beta, gxs + 7, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1Dr_d, 15, y, 1, &beta, gyr + 7, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF1Ds_d, 15, y, 1, &beta, gys + 7, 1);
    }

    // Face 2
    if(reverse[3 * c + 2]) {
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2DrR_d, 15, x, 1, &beta, gxr + 14, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2DsR_d, 15, x, 1, &beta, gxs + 14, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2DrR_d, 15, y, 1, &beta, gyr + 14, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2DsR_d, 15, y, 1, &beta, gys + 14, 1);
    } else {
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2Dr_d, 15, x, 1, &beta, gxr + 14, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2Ds_d, 15, x, 1, &beta, gxs + 14, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2Dr_d, 15, y, 1, &beta, gyr + 14, 1);
      cublasDgemv(handle, CUBLAS_OP_T, 15, 7, &alpha, constants->gF2Ds_d, 15, y, 1, &beta, gys + 14, 1);
    }
  }
}

void init_gauss_grad_neighbour_blas(INSData *nsData, GaussData *gaussData) {
  // Make sure OP2 data is in the right place
  op_arg init_grad_args[] = {
    op_arg_dat(nsData->x, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(nsData->y, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(gaussData->reverse, -1, OP_ID, 3, "double", OP_READ),
    op_arg_dat(gaussData->rx, -1, OP_ID, 21, "double", OP_WRITE),
    op_arg_dat(gaussData->sx, -1, OP_ID, 21, "double", OP_WRITE),
    op_arg_dat(gaussData->ry, -1, OP_ID, 21, "double", OP_WRITE),
    op_arg_dat(gaussData->sy, -1, OP_ID, 21, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 7, init_grad_args);

  int setSize = nsData->x->set->size;

  int *reverse = (int *)malloc(3 * setSize * sizeof(int));
  cudaMemcpy(reverse, gaussData->reverse->data_d, 3 * setSize * sizeof(int), cudaMemcpyDeviceToHost);

  cublas_init_gauss_grad_neighbour(constants->handle, setSize, reverse, (double *)nsData->x->data_d,
                   (double *)nsData->y->data_d, (double *)gaussData->rx->data_d,
                   (double *)gaussData->sx->data_d, (double *)gaussData->ry->data_d,
                   (double *)gaussData->sy->data_d);

  free(reverse);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(7, init_grad_args);
}
