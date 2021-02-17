#include "poisson.h"

#include "kernels/setup_poisson.h"
#include "kernels/set_tau.h"
#include "kernels/set_tau_bc.h"

Poisson::Poisson(INSData *data) {
  // Allocate memory
  pTau_data      = (double *)malloc(15 * data->numCells * sizeof(double));
  pExRHS_data[0] = (double *)malloc(15 * data->numCells * sizeof(double));
  pExRHS_data[1] = (double *)malloc(15 * data->numCells * sizeof(double));
  // Declare OP2 dats
  pTau      = op_decl_dat(data->cells, 15, "double", pTau_data, "pTau");
  pExRHS[0] = op_decl_dat(data->cells, 15, "double", pExRHS_data[0], "pExRHS0");
  pExRHS[1] = op_decl_dat(data->cells, 15, "double", pExRHS_data[1], "pExRHS1");
  // Initialisation kernels
  
  op_par_loop(setup_poisson, "setup_poisson", data->cells,
              op_arg_dat(pTau, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(pExRHS[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(pExRHS[1], -1, OP_ID, 15, "double", OP_WRITE));

  op_par_loop(set_tau, "set_tau", data->edges,
              op_arg_dat(data->edgeNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(data->nodeX, -2, data->edge2cells, 3, "double", OP_READ),
              op_arg_dat(data->nodeY, -2, data->edge2cells, 3, "double", OP_READ),
              op_arg_dat(data->J,  -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(data->sJ, -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(pTau, -2, data->edge2cells, 15, "double", OP_INC));

  op_par_loop(set_tau_bc, "set_tau_bc", data->bedges,
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->J,  0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->sJ, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(pTau, 0, data->bedge2cells, 15, "double", OP_INC));
}

Poisson::~Poisson() {
  // Free memory
  free(pTau_data);
  free(pExRHS_data[0]);
  free(pExRHS_data[1]);
}
