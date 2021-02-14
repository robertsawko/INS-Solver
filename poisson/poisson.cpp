// Include OP2 stuff
#include "op_seq.h"
// Include C++ stuff
#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <getopt.h>
#include <limits>

#include "constants/all_constants.h"
#include "load_mesh.h"
#include "ins_data.h"
#include "blas_calls.h"
#include "save_solution.h"
#include "poisson_rhs.h"
#include "operators.h"

// Kernels
#include "kernels/init_grid.h"
#include "kernels/set_ic1.h"
#include "kernels/set_ic2.h"
#include "kernels/set_tau.h"
#include "kernels/set_tau_bc.h"
#include "kernels/div.h"
#include "kernels/curl.h"
#include "kernels/grad.h"

using namespace std;

// Stuff for parsing command line arguments
extern char *optarg;
extern int  optind, opterr, optopt;
static struct option options[] = {
  {"iter", required_argument, 0, 0},
  {"alpha", required_argument, 0, 0},
  {0,    0,                  0,  0}
};

int main(int argc, char **argv) {
  // Object that holds all sets, maps and dats
  // (along with memory associated with them)
  data = new INSData();

  load_mesh("./grid.cgns", data);

  // Initialise OP2
  op_init(argc, argv, 2);

  // Initialise all sets, maps and dats
  data->initOP2();

  // Get input from args
  int iter = 1;
  bc_alpha = 0.0;

  int opt_index = 0;
  while(getopt_long_only(argc, argv, "", options, &opt_index) != -1) {
    if(strcmp((char*)options[opt_index].name,"iter") == 0) iter = atoi(optarg);
    if(strcmp((char*)options[opt_index].name,"alpha") == 0) bc_alpha = stod(optarg);
  }

  // Declare OP2 constants
  op_decl_const(3 * 5, "int", FMASK);

  // Calculate geometric factors
  init_grid_blas(data);

  op_par_loop(init_grid, "init_grid", data->cells,
              op_arg_dat(data->node_coords, -3, data->cell2nodes, 2, "double", OP_READ),
              op_arg_dat(data->nodeX, -1, OP_ID, 3, "double", OP_WRITE),
              op_arg_dat(data->nodeY, -1, OP_ID, 3, "double", OP_WRITE),
              op_arg_dat(data->xr, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->yr, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->xs, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->ys, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->rx, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->ry, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->sx, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->sy, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->nx, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->ny, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->J,  -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->sJ, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->fscale, -1, OP_ID, 15, "double", OP_WRITE));

  // Set initial conditions
  op_par_loop(set_ic1, "set_ic1", data->cells,
              op_arg_dat(data->uD,   -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->qN,   -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->rhs,  -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->pTau, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->pExRHS[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->pExRHS[1], -1, OP_ID, 15, "double", OP_WRITE));

  // Compute tau
  op_par_loop(set_tau, "set_tau", data->edges,
              op_arg_dat(data->edgeNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(data->nodeX, -2, data->edge2cells, 3, "double", OP_READ),
              op_arg_dat(data->nodeY, -2, data->edge2cells, 3, "double", OP_READ),
              op_arg_dat(data->J,  -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(data->sJ, -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(data->pTau, -2, data->edge2cells, 15, "double", OP_INC));

  op_par_loop(set_tau_bc, "set_tau_bc", data->bedges,
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->J,  0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->sJ, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->pTau, 0, data->bedge2cells, 15, "double", OP_INC));

  op_par_loop(set_ic2, "set_ic2", data->bedges,
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->uD,   0, data->bedge2cells, 15, "double", OP_INC),
              op_arg_dat(data->qN,   0, data->bedge2cells, 15, "double", OP_INC));

  // TODO


  // Save solution to CGNS file
  // double *sol_q0 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  // double *sol_q1 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  // double *sol_q2 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  // op_fetch_data(data->Q[currentIter][0], sol_q0);
  // op_fetch_data(data->Q[currentIter][1], sol_q1);
  // op_fetch_data(data->Q[currentIter][2], sol_q2);
  // save_solution("./naca0012.cgns", op_get_size(data->nodes), op_get_size(data->cells),
  //               sol_q0, sol_q1, sol_q2, data->cgnsCells);

  // free(sol_q0);
  // free(sol_q1);
  // free(sol_q2);

  // Clean up OP2
  op_exit();

  delete data;
}
