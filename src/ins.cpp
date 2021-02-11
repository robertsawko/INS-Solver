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

// Kernels
#include "kernels/init_grid.h"
#include "kernels/set_ic.h"

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
  INSData *data = new INSData();

  load_mesh("./naca0012.cgns", data);

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


  gam = 1.4;
  mu = 1e-2;
  bc_mach = 0.4;
  bc_p = 1.0;
  bc_r = 1.0;
  if(bc_alpha != 0.0) {
    bc_u = sin((M_PI/2.0) - (bc_alpha * M_PI / 180.0)) * sqrt(gam * bc_p / bc_r) * bc_mach;
    bc_v = cos((M_PI/2.0) - (bc_alpha * M_PI / 180.0)) * sqrt(gam * bc_p / bc_r) * bc_mach;
  } else {
    bc_u = sqrt(gam * bc_p / bc_r) * bc_mach;
    bc_v = 0.0;
  }

  cout << "gam: " << gam << endl;
  cout << "mu: " << mu << endl;
  cout << "bc_mach: " << bc_mach << endl;
  cout << "bc_alpha: " << bc_alpha << endl;
  cout << "bc_p: " << bc_p << endl;
  cout << "bc_u: " << bc_u << endl;
  cout << "bc_v: " << bc_v << endl;

  // Declare OP2 constants
  op_decl_const(1, "double", &gam);
  op_decl_const(1, "double", &mu);
  op_decl_const(1, "double", &bc_mach);
  op_decl_const(1, "double", &bc_alpha);
  op_decl_const(1, "double", &bc_p);
  op_decl_const(1, "double", &bc_u);
  op_decl_const(1, "double", &bc_v);
  op_decl_const(15, "double", ones);
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
              op_arg_dat(data->nx, -1, OP_ID, 3 * 5, "double", OP_WRITE),
              op_arg_dat(data->ny, -1, OP_ID, 3 * 5, "double", OP_WRITE),
              op_arg_dat(data->fscale, -1, OP_ID, 3 * 5, "double", OP_WRITE));

  // Set initial conditions
  op_par_loop(set_ic, "set_ic", data->cells,
              op_arg_dat(data->Q[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->Q[1], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->Q[2], -1, OP_ID, 15, "double", OP_WRITE));

  // TODO

  // Save solution to CGNS file
  double *sol_q0 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  double *sol_q1 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  double *sol_q2 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  op_fetch_data(data->Q[0], sol_q0);
  op_fetch_data(data->Q[1], sol_q1);
  op_fetch_data(data->Q[2], sol_q2);
  save_solution("./naca0012.cgns", op_get_size(data->nodes), op_get_size(data->cells),
                sol_q0, sol_q1, sol_q2, data->cgnsCells);

  free(sol_q0);
  free(sol_q1);
  free(sol_q2);

  // Clean up OP2
  op_exit();

  delete data;
}
