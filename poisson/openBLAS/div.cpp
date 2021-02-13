#include "cblas.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void openblas_div(const int numCells, const double *u, const double *v,
                         double *div0, double *div1, double *div2, double *div3) {
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 15, numCells, 15, 1.0, Dr, 15, u, 15, 0.0, div0, 15);
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 15, numCells, 15, 1.0, Ds, 15, u, 15, 0.0, div1, 15);
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 15, numCells, 15, 1.0, Dr, 15, v, 15, 0.0, div2, 15);
  cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, 15, numCells, 15, 1.0, Ds, 15, v, 15, 0.0, div3, 15);
}

void div_blas(INSData *nsData, op_dat u, op_dat v) {
  // Make sure OP2 data is in the right place
  op_arg div_args[] = {
    op_arg_dat(u, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(v, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(nsData->div[0], -1, OP_ID, 15, "double", OP_WRITE),
    op_arg_dat(nsData->div[1], -1, OP_ID, 15, "double", OP_WRITE),
    op_arg_dat(nsData->div[2], -1, OP_ID, 15, "double", OP_WRITE),
    op_arg_dat(nsData->div[3], -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges(nsData->cells, 6, div_args);

  openblas_div(nsData->numCells, (double *)u->data, (double *)v->data,
               (double *)nsData->div[0]->data, (double *)nsData->div[1]->data,
               (double *)nsData->div[2]->data, (double *)nsData->div[3]->data);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit(6, div_args);
}