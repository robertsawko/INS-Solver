#include "poisson.h"

void Poisson::copy_u(const double *u) {
  op_arg u_copy_args[] = {
    op_arg_dat(pU, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges(data->cells, 1, u_copy_args);
  memcpy(pU->data, u, data->numCells * 15 * sizeof(double));
  op_mpi_set_dirtybit(1, u_copy_args);
}

void Poisson::copy_rhs(double *rhs) {
  op_arg rhs_copy_args[] = {
    op_arg_dat(data->pRHS, -1, OP_ID, 15, "double", OP_READ)
  };
  op_mpi_halo_exchanges(data->cells, 1, rhs_copy_args);
  memcpy(rhs, pRHS->data, data->numCells * 15 * sizeof(double));
  op_mpi_set_dirtybit(1, rhs_copy_args);
}
