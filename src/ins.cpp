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
#include "operators.h"
#include "save_solution.h"
#include "poisson.h"

// Kernels
#include "kernels/init_grid.h"
#include "kernels/set_ic.h"
#include "kernels/calc_dt.h"

#include "kernels/advection_flux.h"
#include "kernels/advection_faces.h"
#include "kernels/advection_bc.h"
#include "kernels/advection_numerical_flux.h"
#include "kernels/advection_intermediate_vel.h"

#include "kernels/pressure_bc.h"
#include "kernels/pressure_bc2.h"
#include "kernels/pressure_bc3.h"
#include "kernels/pressure_rhs.h"
#include "kernels/pressure_update_vel.h"

#include "kernels/viscosity_faces.h"
#include "kernels/viscosity_rhs.h"
#include "kernels/viscosity_bc.h"
#include "kernels/viscosity_set_bc.h"

using namespace std;

// Stuff for parsing command line arguments
extern char *optarg;
extern int  optind, opterr, optopt;
static struct option options[] = {
  {"iter", required_argument, 0, 0},
  {"alpha", required_argument, 0, 0},
  {0,    0,                  0,  0}
};

void advection(INSData *data, int currentInd, double a0, double a1, double b0,
               double b1, double g0, double dt, double t);

void pressure(INSData *data, Poisson *poisson, int currentInd, double a0, double a1, double b0,
              double b1, double g0, double dt, double t);

void viscosity(INSData *data, Poisson *poisson, int currentInd, double a0, double a1, double b0,
               double b1, double g0, double dt, double t);

int main(int argc, char **argv) {
  string filename = "./cylinder.cgns";
  char help[] = "TODO";
  int ierr = PetscInitialize(&argc, &argv, (char *)0, help);
  if(ierr) {
    cout << "Error initialising PETSc" << endl;
    return ierr;
  }

  gam = 1.4;
  mu = 1e-2;
  nu = 1e-3;
  bc_u = 1e-6;
  bc_v = 0.0;
  ic_u = 0.0;
  ic_v = 0.0;

  cout << "gam: " << gam << endl;
  cout << "mu: " << mu << endl;
  cout << "nu: " << nu << endl;

  // Object that holds all sets, maps and dats
  // (along with memory associated with them)
  INSData *data = new INSData();

  auto bcNum = [](double x1, double x2, double y1, double y2) -> int {
    if(x1 == 0.0 && x2 == 0.0) {
      // Inflow
      return 0;
    } else if(x1 == 2.2 && x2 == 2.2) {
      // Outflow
      return 1;
    } else {
      // Wall
      return 2;
    }
  };

  load_mesh(filename.c_str(), data, bcNum);

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
  op_par_loop(set_ic, "set_ic", data->cells,
              op_arg_dat(data->Q[0][0],   -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->Q[0][1],   -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->exQ[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->exQ[1], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->dPdN[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->dPdN[1], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->pRHSex, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->dirichletBC, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->neumannBCx, -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->neumannBCy, -1, OP_ID, 15, "double", OP_WRITE));

  Poisson *poisson = new Poisson(data);

  double dt = numeric_limits<double>::max();
  op_par_loop(calc_dt, "calc_dt", data->cells,
              op_arg_dat(data->nodeX, -1, OP_ID, 3, "double", OP_READ),
              op_arg_dat(data->nodeY, -1, OP_ID, 3, "double", OP_READ),
              op_arg_gbl(&dt, 1, "double", OP_MIN));
  dt = dt * 1e-2;
  cout << "dt: " << dt << endl;

  double a0 = 1.0;
  double a1 = 0.0;
  double b0 = 1.0;
  double b1 = 0.0;
  double g0 = 1.0;
  int currentIter = 0;
  double time = 0.0;

  double cpu_1, wall_1, cpu_2, wall_2, cpu_loop_1, wall_loop_1, cpu_loop_2, wall_loop_2;
  double a_time = 0.0;
  double p_time = 0.0;
  double v_time = 0.0;
  op_timers(&cpu_1, &wall_1);

  for(int i = 0; i < iter; i++) {
    if(i == 1) {
      g0 = 1.5;
      a0 = 2.0;
      a1 = -0.5;
      b0 = 2.0;
      b1 = -1.0;
    }
    op_timers(&cpu_loop_1, &wall_loop_1);
    advection(data, currentIter % 2, a0, a1, b0, b1, g0, dt, time);
    op_timers(&cpu_loop_2, &wall_loop_2);
    a_time += wall_loop_2 - wall_loop_1;

    op_timers(&cpu_loop_1, &wall_loop_1);
    pressure(data, poisson, currentIter % 2, a0, a1, b0, b1, g0, dt, time);
    op_timers(&cpu_loop_2, &wall_loop_2);
    p_time += wall_loop_2 - wall_loop_1;

    op_timers(&cpu_loop_1, &wall_loop_1);
    viscosity(data, poisson, currentIter % 2, a0, a1, b0, b1, g0, dt, time);
    op_timers(&cpu_loop_2, &wall_loop_2);
    v_time += wall_loop_2 - wall_loop_1;

    currentIter++;
    time += dt;
  }
  op_timers(&cpu_2, &wall_2);

  cout << "Final time: " << time << endl;

  cout << "Wall time: " << wall_2 - wall_1 << endl;
  cout << "Time to simulate 1 second: " << (wall_2 - wall_1) / time << endl << endl;

  cout << "Time in advection solve: " << a_time << endl;
  cout << "Time in pressure solve: " << p_time << endl;
  cout << "Time in viscosity solve: " << v_time << endl;


  // Save solution to CGNS file
  double *sol_q0 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  double *sol_q1 = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  op_fetch_data(data->Q[currentIter % 2][0], sol_q0);
  op_fetch_data(data->Q[currentIter % 2][1], sol_q1);
  // op_fetch_data(data->p, sol_q0);
  // op_fetch_data(data->pRHS, sol_q1);
  // save_solution_cell("cylinder.cgns", op_get_size(data->nodes), op_get_size(data->cells),
  //               sol_q0, sol_q1, data->cgnsCells);

  // op_fetch_data(data->p, sol_q0);
  // op_fetch_data(data->Q[currentIter % 2][1], sol_q1);
  save_solution("cylinder.cgns", op_get_size(data->nodes), op_get_size(data->cells),
                sol_q0, sol_q1, data->cgnsCells);
/*
  op_fetch_data(data->QT[0], sol_q0);
  op_fetch_data(data->QT[1], sol_q1);
  save_solution("cylinder1.cgns", op_get_size(data->nodes), op_get_size(data->cells),
                sol_q0, sol_q1, data->cgnsCells);

  op_fetch_data(data->QTT[0], sol_q0);
  op_fetch_data(data->QTT[1], sol_q1);
  save_solution("cylinder2.cgns", op_get_size(data->nodes), op_get_size(data->cells),
                sol_q0, sol_q1, data->cgnsCells);

  op_fetch_data(data->dpdx, sol_q0);
  op_fetch_data(data->dpdy, sol_q1);
  save_solution("cylinder3.cgns", op_get_size(data->nodes), op_get_size(data->cells),
                sol_q0, sol_q1, data->cgnsCells);

  op_fetch_data(data->visRHS[0], sol_q0);
  op_fetch_data(data->visRHS[1], sol_q1);
  save_solution("cylinder4.cgns", op_get_size(data->nodes), op_get_size(data->cells),
                sol_q0, sol_q1, data->cgnsCells);
  */

  free(sol_q0);
  free(sol_q1);

/*
  double *u_ptr = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  double *v_ptr = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  double *x_ptr = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));
  double *y_ptr = (double *)malloc(15 * op_get_size(data->cells) * sizeof(double));

  // op_fetch_data(data->Q[currentIter % 2][0], u_ptr);
  // op_fetch_data(data->Q[currentIter % 2][1], v_ptr);
  op_fetch_data(data->dpdx, u_ptr);
  op_fetch_data(data->dpdy, v_ptr);
  op_fetch_data(data->x, x_ptr);
  op_fetch_data(data->y, y_ptr);

  save_solution_all("solution.cgns", op_get_size(data->cells), u_ptr, v_ptr, x_ptr, y_ptr);

  free(u_ptr);
  free(v_ptr);
  free(x_ptr);
  free(y_ptr);
*/
  // Clean up OP2
  op_exit();

  delete poisson;
  delete data;

  ierr = PetscFinalize();
  return ierr;
}

void advection(INSData *data, int currentInd, double a0, double a1, double b0,
               double b1, double g0, double dt, double t) {
  op_par_loop(advection_flux, "advection_flux", data->cells,
              op_arg_dat(data->Q[currentInd][0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->F[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->F[1], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->F[2], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->F[3], -1, OP_ID, 15, "double", OP_WRITE));

  div(data, data->F[0], data->F[1], data->N[currentInd][0]);
  div(data, data->F[2], data->F[3], data->N[currentInd][1]);

  op_par_loop(advection_faces, "advection_faces", data->edges,
              op_arg_dat(data->edgeNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(data->nodeX, -2, data->edge2cells, 3, "double", OP_READ),
              op_arg_dat(data->nodeY, -2, data->edge2cells, 3, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][0], -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][1], -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(data->exQ[0], -2, data->edge2cells, 15, "double", OP_INC),
              op_arg_dat(data->exQ[1], -2, data->edge2cells, 15, "double", OP_INC));

  op_par_loop(advection_bc, "advection_bc", data->bedges,
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_gbl(&t, 1, "double", OP_READ),
              op_arg_dat(data->x, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->y, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][0], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][1], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->exQ[0], 0, data->bedge2cells, 15, "double", OP_INC),
              op_arg_dat(data->exQ[1], 0, data->bedge2cells, 15, "double", OP_INC));

  op_par_loop(advection_numerical_flux, "advection_numerical_flux", data->cells,
              op_arg_dat(data->fscale,  -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->nx,      -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->ny,      -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->exQ[0],  -1, OP_ID, 15, "double", OP_RW),
              op_arg_dat(data->exQ[1],  -1, OP_ID, 15, "double", OP_RW),
              op_arg_dat(data->flux[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->flux[1], -1, OP_ID, 15, "double", OP_WRITE));

  advection_lift_blas(data, currentInd);

  op_par_loop(advection_intermediate_vel, "advection_intermediate_vel", data->cells,
              op_arg_gbl(&a0, 1, "double", OP_READ),
              op_arg_gbl(&a1, 1, "double", OP_READ),
              op_arg_gbl(&b0, 1, "double", OP_READ),
              op_arg_gbl(&b1, 1, "double", OP_READ),
              op_arg_gbl(&g0, 1, "double", OP_READ),
              op_arg_gbl(&dt, 1, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->Q[currentInd][1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->Q[(currentInd + 1) % 2][0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->Q[(currentInd + 1) % 2][1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->N[currentInd][0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->N[currentInd][1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->N[(currentInd + 1) % 2][0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->N[(currentInd + 1) % 2][1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->QT[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->QT[1], -1, OP_ID, 15, "double", OP_WRITE));
}

void pressure(INSData *data, Poisson *poisson, int currentInd, double a0, double a1, double b0,
              double b1, double g0, double dt, double t) {
  int pressure_dirichlet[] = {1, -1};
  int pressure_neumann[] = {0, 2};
  // int pressure_neumann[] = {-1, -1};
  poisson->setDirichletBCs(pressure_dirichlet);
  poisson->setNeumannBCs(pressure_neumann);
  div(data, data->QT[0], data->QT[1], data->divVelT);
  curl(data, data->Q[currentInd][0], data->Q[currentInd][1], data->curlVel);
  grad(data, data->curlVel, data->gradCurlVel[0], data->gradCurlVel[1]);

  // Apply boundary conditions
  // May need to change in future if non-constant boundary conditions used
  op_par_loop(pressure_bc, "pressure_bc", data->bedges,
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_gbl(&t, 1, "double", OP_READ),
              op_arg_dat(data->x, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->y, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->nx, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->ny, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->N[currentInd][0], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->N[currentInd][1], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->gradCurlVel[0], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->gradCurlVel[1], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->dPdN[currentInd], 0, data->bedge2cells, 15, "double", OP_INC));

  op_par_loop(pressure_rhs, "pressure_rhs", data->cells,
              op_arg_gbl(&b0, 1, "double", OP_READ),
              op_arg_gbl(&b1, 1, "double", OP_READ),
              op_arg_gbl(&g0, 1, "double", OP_READ),
              op_arg_gbl(&dt, 1, "double", OP_READ),
              op_arg_dat(data->J, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->sJ, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->dPdN[currentInd], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->dPdN[(currentInd + 1) % 2], -1, OP_ID, 15, "double", OP_RW),
              op_arg_dat(data->divVelT, -1, OP_ID, 15, "double", OP_RW));

  pressure_rhs_blas(data, currentInd);

  // Dirichlet BCs, p = 0 on outflow
  op_par_loop(pressure_bc2, "pressure_bc2", data->bedges,
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->pRHS, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->pRHSex, 0, data->bedge2cells, 15, "double", OP_INC));

  op_par_loop(pressure_bc3, "pressure_bc3", data->cells,
              op_arg_dat(data->pRHSex, -1, OP_ID, 15, "double", OP_RW),
              op_arg_dat(data->pRHS, -1, OP_ID, 15, "double", OP_RW));

  poisson->solve(data->pRHS, data->p);

  grad(data, data->p, data->dpdx, data->dpdy);

  double factor = dt / g0;
  op_par_loop(pressure_update_vel, "pressure_update_vel", data->cells,
              op_arg_gbl(&factor, 1, "double", OP_READ),
              op_arg_dat(data->dpdx, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->dpdy, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->QT[0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->QT[1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->QTT[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->QTT[1], -1, OP_ID, 15, "double", OP_WRITE));
}

void viscosity(INSData *data, Poisson *poisson, int currentInd, double a0, double a1, double b0,
               double b1, double g0, double dt, double t) {
  int viscosity_dirichlet[] = {0, 2};
  int viscosity_neumann[] = {1, -1};
  poisson->setDirichletBCs(viscosity_dirichlet);
  poisson->setNeumannBCs(viscosity_neumann);

  double time = t + dt;

  op_par_loop(viscosity_faces, "viscosity_faces", data->edges,
              op_arg_dat(data->edgeNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(data->QTT[0], -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(data->QTT[1], -2, data->edge2cells, 15, "double", OP_READ),
              op_arg_dat(data->exQ[0], -2, data->edge2cells, 15, "double", OP_INC),
              op_arg_dat(data->exQ[1], -2, data->edge2cells, 15, "double", OP_INC));

  op_par_loop(advection_bc, "advection_bc", data->bedges,
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_gbl(&time, 1, "double", OP_READ),
              op_arg_dat(data->x, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->y, 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->QTT[0], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->QTT[1], 0, data->bedge2cells, 15, "double", OP_READ),
              op_arg_dat(data->exQ[0], 0, data->bedge2cells, 15, "double", OP_INC),
              op_arg_dat(data->exQ[1], 0, data->bedge2cells, 15, "double", OP_INC));

  op_par_loop(viscosity_set_bc, "viscosity_set_bc", data->cells,
              op_arg_dat(data->QTT[0], -1, OP_ID, 15, "double", OP_RW),
              op_arg_dat(data->QTT[1], -1, OP_ID, 15, "double", OP_RW),
              op_arg_dat(data->exQ[0], -1, OP_ID, 15, "double", OP_RW),
              op_arg_dat(data->exQ[1], -1, OP_ID, 15, "double", OP_RW));

  viscosity_rhs_blas(data);

  double factor = g0 / (nu * dt);
  op_par_loop(viscosity_rhs, "viscosity_rhs", data->cells,
              op_arg_gbl(&factor, 1, "double", OP_READ),
              op_arg_dat(data->J, -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->QTT[0], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->QTT[1], -1, OP_ID, 15, "double", OP_READ),
              op_arg_dat(data->visRHS[0], -1, OP_ID, 15, "double", OP_WRITE),
              op_arg_dat(data->visRHS[1], -1, OP_ID, 15, "double", OP_WRITE));
/*
  op_par_loop(viscosity_bc, "viscosity_bc", data->bedges,
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->visRHS[0], 0, data->bedge2cells, 15, "double", OP_INC),
              op_arg_dat(data->visRHS[1], 0, data->bedge2cells, 15, "double", OP_INC));
*/
  poisson->solve(data->visRHS[0], data->Q[(currentInd + 1) % 2][0], true, factor);
  poisson->solve(data->visRHS[1], data->Q[(currentInd + 1) % 2][1], true, factor);
}
