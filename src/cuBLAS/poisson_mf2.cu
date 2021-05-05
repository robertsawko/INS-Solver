#include "cublas_v2.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void cublas_poisson_mf2_op2(cublasHandle_t handle, const int numEdges, const int *edges,
                                     const int *edgeNum, const double *u,
                                     const double *op0, const double *op1,
                                     const double *op2, double *rhs) {
  double alpha = 1.0;
  double beta = 1.0;
  for(int e = 0; e < numEdges; e++) {
    const int leftElement  = edges[2 * e];
    const int rightElement = edges[2 * e + 1];
    const double *u_l = u + leftElement * 15;
    const double *u_r = u + rightElement * 15;
    const double *op2_l; const double *op2_r;
    switch(edgeNum[2 * e]) {
      case 0:
        op2_l = op0 + leftElement * 15 * 15;
        break;
      case 1:
        op2_l = op1 + leftElement * 15 * 15;
        break;
      case 2:
        op2_l = op2 + leftElement * 15 * 15;
        break;
    }
    switch(edgeNum[2 * e + 1]) {
      case 0:
        op2_r = op0 + rightElement * 15 * 15;
        break;
      case 1:
        op2_r = op1 + rightElement * 15 * 15;
        break;
      case 2:
        op2_r = op2 + rightElement * 15 * 15;
        break;
    }
    double *rhs_l = rhs + leftElement * 15;
    double *rhs_r = rhs + rightElement * 15;

    cublasDgemv(handle, CUBLAS_OP_T, 15, 15, &alpha, op2_l, 15, u_r, 1, &beta, rhs_l, 1);
    cublasDgemv(handle, CUBLAS_OP_T, 15, 15, &alpha, op2_r, 15, u_l, 1, &beta, rhs_r, 1);
  }
}

void poisson_mf2_blas(INSData *data, Poisson_MF2 *poisson, CubatureData *cubatureData, bool massMat, double massFactor) {
  // Make sure OP2 data is in the right place
  op_arg poisson_args[] = {
    op_arg_dat(poisson->u, -1, OP_ID, 15, "double", OP_READ),
    op_arg_dat(poisson->op2[0], -1, OP_ID, 15 * 15, "double", OP_READ),
    op_arg_dat(poisson->op2[1], -1, OP_ID, 15 * 15, "double", OP_READ),
    op_arg_dat(poisson->op2[2], -1, OP_ID, 15 * 15, "double", OP_READ),
    op_arg_dat(poisson->rhs, -1, OP_ID, 15, "double", OP_RW)
  };
  op_mpi_halo_exchanges_cuda(data->cells, 5, poisson_args);

  cublas_poisson_mf2_op2(constants->handle, data->numEdges, (int *)data->edge2cell_data,
                         (int *)data->edgeNum_data, (double *)poisson->u->data_d,
                         (double *)poisson->op2[0]->data_d, (double *)poisson->op2[1]->data_d,
                         (double *)poisson->op2[2]->data_d, (double *)poisson->rhs->data_d);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit_cuda(5, poisson_args);
}