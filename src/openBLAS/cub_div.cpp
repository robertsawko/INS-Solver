#include "cblas.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void openblas_cub_div(const int numCells, const double *in0, const double *in1,
                             double *out0, double *out1, double *out2, double *out3) {
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 46, numCells, 15, 1.0, constants->cubDr, 15, in0, 15, 0.0, out0, 46);
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 46, numCells, 15, 1.0, constants->cubDs, 15, in0, 15, 0.0, out1, 46);
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 46, numCells, 15, 1.0, constants->cubDr, 15, in1, 15, 0.0, out2, 46);
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 46, numCells, 15, 1.0, constants->cubDs, 15, in1, 15, 0.0, out3, 46);
}

void cub_div_blas(INSData *data, CubatureData *cubatureData, op_dat u, op_dat v) {
  // Make sure OP2 data is in the right place
  op_arg cub_div_args[] = {
    op_arg_dat(u, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(v, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(cubatureData->op_temps[0], -1, OP_ID, 46, "double", OP_WRITE),
    op_arg_dat(cubatureData->op_temps[1], -1, OP_ID, 46, "double", OP_WRITE),
    op_arg_dat(cubatureData->op_temps[2], -1, OP_ID, 46, "double", OP_WRITE),
    op_arg_dat(cubatureData->op_temps[3], -1, OP_ID, 46, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges(data->cells, 6, cub_div_args);

  openblas_cub_div(data->numCells, (double *)u->data, (double *)v->data,
                   (double *)cubatureData->op_temps[0]->data, (double *)cubatureData->op_temps[1]->data,
                   (double *)cubatureData->op_temps[2]->data, (double *)cubatureData->op_temps[3]->data);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit(6, cub_div_args);
}
