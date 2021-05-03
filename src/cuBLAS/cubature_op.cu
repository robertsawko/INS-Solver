#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_cubature_op(cublasHandle_t handle, const int numCells,
                               const double *Dx_d, const double *Dy_d,
                               const double *temp_d, const double *temp2_d, double *OP_d) {
  for(int i = 0; i < numCells; i++) {
    const double *Dx = Dx_d + i * 46 * 15;
    const double *Dy = Dy_d + i * 46 * 15;
    const double *temp = temp_d + i * 46 * 15;
    const double *temp2 = temp2_d + i * 46 * 15;
    double *OP = OP_d + i * 15 * 15;

    double alpha = 1.0;
    double beta = 0.0;
    cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_T, 15, 15, 46, &alpha, Dx, 15, temp, 15, &beta, OP, 15);
    double beta2 = 1.0;
    cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_T, 15, 15, 46, &alpha, Dy, 15, temp2, 15, &beta2, OP, 15);
  }
}

void cubature_op_blas(INSData *nsData, CubatureData *cubData) {
  // Make sure OP2 data is in the right place
  op_arg op_cubature_args[] = {
    op_arg_dat(cubData->Dx, -1, OP_ID, 46 * 15, "double", OP_READ),
    op_arg_dat(cubData->Dy, -1, OP_ID, 46 * 15, "double", OP_READ),
    op_arg_dat(cubData->temp, -1, OP_ID, 46 * 15, "double", OP_READ),
    op_arg_dat(cubData->temp2, -1, OP_ID, 46 * 15, "double", OP_READ),
    op_arg_dat(cubData->OP, -1, OP_ID, 15 * 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(nsData->cells, 5, op_cubature_args);

  cublas_cubature_op(constants->handle, nsData->numCells, (double *)cubData->Dx->data_d,
                     (double *)cubData->Dy->data_d, (double *)cubData->temp->data_d,
                     (double *)cubData->temp2->data_d, (double *)cubData->OP->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(5, op_cubature_args);
}
