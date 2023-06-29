#include "solvers/2d/mp_ins_solver_over_int.h"

// Include OP2 stuff
#include "op_seq.h"

#include <iostream>
#include <limits>
#include <stdexcept>

#include "dg_op2_blas.h"

#include "timing.h"
#include "config.h"
#include "linear_solvers/petsc_amg.h"
#include "linear_solvers/petsc_block_jacobi.h"

extern Timing *timer;
extern Config *config;

using namespace std;

MPINSSolverOverInt2D::MPINSSolverOverInt2D(DGMesh2D *m) {
  mesh = m;
  resuming = false;
  ls = new LevelSetSolver2D(mesh);

  setup_common();

  std::string name;
  for(int i = 0; i < 2; i++) {
    name = "mp_ins_solver_vel0" + std::to_string(i);
    vel[0][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "mp_ins_solver_vel1" + std::to_string(i);
    vel[1][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "mp_ins_solver_n0" + std::to_string(i);
    n[0][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "mp_ins_solver_n1" + std::to_string(i);
    n[1][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
  }
  pr  = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, "mp_ins_solver_pr");

  dPdN[0] = op_decl_dat(mesh->cells, DG_G_NP, DG_FP_STR, (DG_FP *)NULL, "mp_ins_solver_dPdN0");
  dPdN[1] = op_decl_dat(mesh->cells, DG_G_NP, DG_FP_STR, (DG_FP *)NULL, "mp_ins_solver_dPdN1");

  currentInd = 0;
  time = 0.0;

  a0 = 1.0;
  a1 = 0.0;
  b0 = 1.0;
  b1 = 0.0;
  g0 = 1.0;
}

MPINSSolverOverInt2D::MPINSSolverOverInt2D(DGMesh2D *m, const std::string &filename, const int iter) {
  mesh = m;
  resuming = true;
  ls = new LevelSetSolver2D(mesh, filename);

  setup_common();

  vel[0][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_vel00");
  vel[1][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_vel10");
  vel[0][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_vel01");
  vel[1][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_vel11");
  n[0][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_n00");
  n[1][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_n10");
  n[0][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_n01");
  n[1][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_n11");
  pr      = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_pr");
  dPdN[0] = op_decl_dat_hdf5(mesh->cells, DG_G_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_dPdN0");
  dPdN[1] = op_decl_dat_hdf5(mesh->cells, DG_G_NP, DG_FP_STR, filename.c_str(), "mp_ins_solver_dPdN1");

  currentInd = iter;

  if(iter > 0) {
    g0 = 1.5;
    a0 = 2.0;
    a1 = -0.5;
    b0 = 2.0;
    b1 = -1.0;
  } else {
    a0 = 1.0;
    a1 = 0.0;
    b0 = 1.0;
    b1 = 0.0;
    g0 = 1.0;
  }
}

void MPINSSolverOverInt2D::setup_common() {
  #ifdef DG_OP2_SOA
  throw std::runtime_error("2D over integrate not implemented for SoA");
  #endif

  // pressureMatrix = new CubFactorPoissonMatrix2D(mesh);
  // viscosityMatrix = new CubFactorMMPoissonMatrix2D(mesh);
  coarsePressureMatrix = new FactorPoissonCoarseMatrixOverInt2D(mesh);
  pressureMatrix = new FactorPoissonSemiMatrixFreeOverInt2D(mesh);
  viscosityMatrix = new FactorMMPoissonMatrixOverInt2D(mesh);
  // pressureSolver = new PETScAMGSolver(mesh);
  pressureSolver = new PETScPMultigrid(mesh);
  viscositySolver = new PETScBlockJacobiSolver(mesh);

  int pr_tmp = 0;
  int vis_tmp = 0;
  config->getInt("solver-options", "pr_nullspace", pr_tmp);
  config->getInt("solver-options", "vis_nullspace", vis_tmp);

  pressureSolver->set_coarse_matrix(coarsePressureMatrix);
  pressureSolver->set_matrix(pressureMatrix);
  pressureSolver->set_nullspace(pr_tmp == 1);
  viscositySolver->set_matrix(viscosityMatrix);
  viscositySolver->set_nullspace(vis_tmp == 1);

  std::string name;
  for(int i = 0; i < 2; i++) {
    name = "mp_ins_solver_velT" + std::to_string(i);
    velT[i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "mp_ins_solver_velTT" + std::to_string(i);
    velTT[i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
  }
  for(int i = 0; i < 4; i++) {
    name = "mp_ins_solver_tmp_np" + std::to_string(i);
    tmp_np[i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
  }
  rho = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, "mp_ins_solver_rho");
  mu  = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, "mp_ins_solver_mu");

  for(int i = 0; i < 5; i++) {
    string name    = "tmp_g_np" + to_string(i);
    tmp_g_np[i] = op_decl_dat(mesh->cells, DG_G_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
  }

  bc_types     = op_decl_dat(mesh->bfaces, 1, "int", (int *)NULL, "ins_solver_bc_types");
  pr_bc_types  = op_decl_dat(mesh->bfaces, 1, "int", (int *)NULL, "ins_solver_pr_bc_types");
  vis_bc_types = op_decl_dat(mesh->bfaces, 1, "int", (int *)NULL, "ins_solver_vis_bc_types");

  f[0] = tmp_np[0];
  f[1] = tmp_np[1];
  f[2] = tmp_np[2];
  f[3] = tmp_np[3];
  divVelT = tmp_np[0];
  curlVel = tmp_np[1];
  gradCurlVel[0] = tmp_np[2];
  gradCurlVel[1] = tmp_np[3];
  pRHS = tmp_np[1];
  pr_mat_fact = tmp_np[2];
  dpdx = tmp_np[1];
  dpdy = tmp_np[2];
  visRHS[0] = tmp_np[0];
  visRHS[1] = tmp_np[1];
  vis_mat_mm_fact = tmp_np[2];

  gVel[0] = tmp_g_np[0];
  gVel[1] = tmp_g_np[1];
  gAdvecFlux[0] = tmp_g_np[2];
  gAdvecFlux[1] = tmp_g_np[3];
  gN[0] = tmp_g_np[0];
  gN[1] = tmp_g_np[1];
  gGradCurl[0] = tmp_g_np[2];
  gGradCurl[1] = tmp_g_np[3];
  gRho = tmp_g_np[4];
  prBC = tmp_g_np[0];
  visBC[0] = tmp_g_np[0];
  visBC[1] = tmp_g_np[1];
}

MPINSSolverOverInt2D::~MPINSSolverOverInt2D() {
  delete pressureMatrix;
  delete viscosityMatrix;
  delete pressureSolver;
  delete viscositySolver;
  delete ls;
}

void MPINSSolverOverInt2D::init(const DG_FP re, const DG_FP refVel) {
  timer->startTimer("MPINSSolverOverInt2D - Init");
  reynolds = re;

  ls->init();

  // Set initial conditions
  if(!resuming) {
    op_par_loop(ins_2d_set_ic, "ins_2d_set_ic", mesh->cells,
                op_arg_dat(mesh->x, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->y, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[0][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(vel[0][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(vel[1][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(vel[1][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
  }

  dt = numeric_limits<DG_FP>::max();
  op_par_loop(calc_dt, "calc_dt", mesh->cells,
              op_arg_dat(mesh->nodeX, -1, OP_ID, 3, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->nodeY, -1, OP_ID, 3, DG_FP_STR, OP_READ),
              op_arg_gbl(&dt, 1, DG_FP_STR, OP_MIN));
  dt = dt / (DG_ORDER * DG_ORDER * refVel);
  op_printf("dt: %g\n", dt);

  time = dt * currentInd;
  currentInd = currentInd % 2;

  if(mesh->bface2nodes) {
    op_par_loop(ins_bc_types, "ins_bc_types", mesh->bfaces,
                op_arg_dat(mesh->node_coords, -3, mesh->bface2nodes, 3, DG_FP_STR, OP_READ),
                op_arg_dat(bc_types,     -1, OP_ID, 1, "int", OP_WRITE),
                op_arg_dat(pr_bc_types,  -1, OP_ID, 1, "int", OP_WRITE),
                op_arg_dat(vis_bc_types, -1, OP_ID, 1, "int", OP_WRITE));
  }

  ls->getRhoMu(rho, mu);

  pressureSolver->init();
  viscositySolver->init();

  timer->endTimer("MPINSSolverOverInt2D - Init");
}

void MPINSSolverOverInt2D::step() {
  timer->startTimer("MPINSSolverOverInt2D - Advection");
  advection();
  timer->endTimer("MPINSSolverOverInt2D - Advection");

  timer->startTimer("MPINSSolverOverInt2D - Pressure");
  pressure();
  timer->endTimer("MPINSSolverOverInt2D - Pressure");

  // timer->startTimer("Shock Capturing");
  // shock_capturing();
  // timer->endTimer("Shock Capturing");

  timer->startTimer("MPINSSolverOverInt2D - Viscosity");
  viscosity();
  timer->endTimer("MPINSSolverOverInt2D - Viscosity");

  timer->startTimer("MPINSSolverOverInt2D - Surface");
  surface();
  timer->endTimer("MPINSSolverOverInt2D - Surface");

  currentInd = (currentInd + 1) % 2;
  time += dt;
  g0 = 1.5;
  a0 = 2.0;
  a1 = -0.5;
  b0 = 2.0;
  b1 = -1.0;
}

// Calculate Nonlinear Terms
void MPINSSolverOverInt2D::advection() {
  // Calculate flux values
  op_par_loop(ins_advec_flux_2d, "ins_advec_flux_2d", mesh->cells,
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(f[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[3], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  mesh->div(f[0], f[1], n[currentInd][0]);
  mesh->div(f[2], f[3], n[currentInd][1]);

  op2_gemv(mesh, false, 1.0, DGConstants::GAUSS_INTERP, vel[currentInd][0], 0.0, gVel[0]);
  op2_gemv(mesh, false, 1.0, DGConstants::GAUSS_INTERP, vel[currentInd][1], 0.0, gVel[1]);

  op_par_loop(zero_g_np2, "zero_g_np", mesh->cells,
              op_arg_dat(gAdvecFlux[0], -1, OP_ID, DG_G_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(gAdvecFlux[1], -1, OP_ID, DG_G_NP, DG_FP_STR, OP_WRITE));

  // Exchange values on edges between elements
  op_par_loop(ins_advec_faces_over_int_2d, "ins_advec_faces_over_int_2d", mesh->faces,
              op_arg_dat(mesh->order,     -2, mesh->face2cells, 1, "int", OP_READ),
              op_arg_dat(mesh->edgeNum,   -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(mesh->reverse,   -1, OP_ID, 1, "bool", OP_READ),
              op_arg_dat(mesh->gauss->nx, -2, mesh->face2cells, DG_G_NP, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->gauss->ny, -2, mesh->face2cells, DG_G_NP, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->gauss->sJ, -2, mesh->face2cells, DG_G_NP, DG_FP_STR, OP_READ),
              op_arg_dat(gVel[0],         -2, mesh->face2cells, DG_G_NP, DG_FP_STR, OP_READ),
              op_arg_dat(gVel[1],         -2, mesh->face2cells, DG_G_NP, DG_FP_STR, OP_READ),
              op_arg_dat(gAdvecFlux[0],   -2, mesh->face2cells, DG_G_NP, DG_FP_STR, OP_INC),
              op_arg_dat(gAdvecFlux[1],   -2, mesh->face2cells, DG_G_NP, DG_FP_STR, OP_INC));

  // Enforce BCs
  if(mesh->bface2cells) {
    op_par_loop(ins_advec_bc_over_int_2d, "ins_advec_bc_over_int_2d", mesh->bfaces,
                op_arg_gbl(&time, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->order,     0, mesh->bface2cells, 1, "int", OP_READ),
                op_arg_dat(bc_types,       -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bedgeNum, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->gauss->x,  0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->y,  0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->nx, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->ny, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->sJ, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gVel[0],         0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gVel[1],         0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gAdvecFlux[0],   0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_INC),
                op_arg_dat(gAdvecFlux[1],   0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_INC));
  }
  op2_gemv(mesh, false, 1.0, DGConstants::INV_MASS_GAUSS_INTERP_T, gAdvecFlux[0], 1.0, n[currentInd][0]);
  op2_gemv(mesh, false, 1.0, DGConstants::INV_MASS_GAUSS_INTERP_T, gAdvecFlux[1], 1.0, n[currentInd][1]);

  // Calculate the intermediate velocity values
  op_par_loop(ins_advec_intermediate_vel_2d, "ins_advec_intermediate_vel_2d", mesh->cells,
              op_arg_gbl(&a0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&a1, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&b0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&b1, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&dt, 1, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
}

bool MPINSSolverOverInt2D::pressure() {
  timer->startTimer("MPINSSolverOverInt2D - Pressure RHS");

  mesh->div(velT[0], velT[1], divVelT);
  mesh->curl(vel[currentInd][0], vel[currentInd][1], curlVel);
  mesh->grad(curlVel, gradCurlVel[0], gradCurlVel[1]);

  op2_gemv(mesh, false, 1.0, DGConstants::GAUSS_INTERP, n[currentInd][0], 0.0, gN[0]);
  op2_gemv(mesh, false, 1.0, DGConstants::GAUSS_INTERP, n[currentInd][1], 0.0, gN[1]);
  op2_gemv(mesh, false, 1.0, DGConstants::GAUSS_INTERP, gradCurlVel[0], 0.0, gGradCurl[0]);
  op2_gemv(mesh, false, 1.0, DGConstants::GAUSS_INTERP, gradCurlVel[1], 0.0, gGradCurl[1]);
  op2_gemv(mesh, false, 1.0, DGConstants::GAUSS_INTERP, rho, 0.0, gRho);

  // Apply Neumann pressure boundary conditions
  if(mesh->bface2cells) {
    op_par_loop(mp_ins_pressure_bc_2d, "mp_ins_pressure_bc_2d", mesh->bfaces,
                op_arg_gbl(&time, 1, DG_FP_STR, OP_READ),
                op_arg_dat(bc_types,       -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bedgeNum, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->gauss->x,  0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->y,  0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->nx, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->ny, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gN[0], 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gN[1], 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gGradCurl[0], 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gGradCurl[1], 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(gRho, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(dPdN[currentInd], 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_INC));
  }
  // Apply Dirichlet BCs
  op_par_loop(zero_g_np1, "zero_g_np1", mesh->cells,
              op_arg_dat(prBC, -1, OP_ID, DG_G_NP, DG_FP_STR, OP_WRITE));

  // Calculate RHS of pressure solve
  // This assumes that the boundaries will always be order DG_ORDER
  op_par_loop(ins_pressure_rhs_over_int_2d, "ins_pressure_rhs_over_int_2d", mesh->cells,
              op_arg_gbl(&b0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&b1, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&dt, 1, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->order, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(mesh->J, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->gauss->sJ, -1, OP_ID, DG_G_NP, DG_FP_STR, OP_READ),
              op_arg_dat(dPdN[currentInd], -1, OP_ID, DG_G_NP, DG_FP_STR, OP_READ),
              op_arg_dat(dPdN[(currentInd + 1) % 2], -1, OP_ID, DG_G_NP, DG_FP_STR, OP_RW),
              op_arg_dat(divVelT, -1, OP_ID, DG_NP, DG_FP_STR, OP_RW));

  op2_gemv(mesh, false, 1.0, DGConstants::MASS, divVelT, 0.0, pRHS);
  op2_gemv(mesh, true, 1.0, DGConstants::GAUSS_INTERP, dPdN[(currentInd + 1) % 2], 1.0, pRHS);
  timer->endTimer("MPINSSolverOverInt2D - Pressure RHS");

  // Call PETSc linear solver
  timer->startTimer("MPINSSolverOverInt2D - Pressure Linear Solve");

  op_par_loop(mp_ins_pressure_fact_2d, "mp_ins_pressure_fact_2d", mesh->cells,
              op_arg_dat(rho, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(pr_mat_fact, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  bool converged;
  pressureMatrix->set_factor(pr_mat_fact);
  coarsePressureMatrix->set_factor(pr_mat_fact);
  pressureMatrix->set_bc_types(pr_bc_types);
  pressureSolver->set_coarse_matrix(coarsePressureMatrix);
  pressureSolver->set_bcs(prBC);
  converged = pressureSolver->solve(pRHS, pr);
  timer->endTimer("MPINSSolverOverInt2D - Pressure Linear Solve");

  timer->startTimer("MPINSSolverOverInt2D - Pressure Projection");
  // Calculate gradient of pressure
  mesh->grad_with_central_flux_over_int(pr, dpdx, dpdy);

  // Calculate new velocity intermediate values
  op_par_loop(mp_ins_pressure_update_2d, "mp_ins_pressure_update_2d", mesh->cells,
              op_arg_gbl(&dt, 1, DG_FP_STR, OP_READ),
              op_arg_dat(rho, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(dpdx, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(dpdy, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velTT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velTT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(dPdN[(currentInd + 1) % 2], -1, OP_ID, DG_G_NP, DG_FP_STR, OP_WRITE));

  timer->endTimer("MPINSSolverOverInt2D - Pressure Projection");
  return converged;
}

bool MPINSSolverOverInt2D::viscosity() {
  timer->startTimer("MPINSSolverOverInt2D - Viscosity RHS");
  DG_FP time_n1 = time + dt;

  op_par_loop(zero_g_np2, "zero_g_np", mesh->cells,
              op_arg_dat(visBC[0], -1, OP_ID, DG_G_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(visBC[1], -1, OP_ID, DG_G_NP, DG_FP_STR, OP_WRITE));

  // Get BCs for viscosity solve
  if(mesh->bface2cells) {
    op_par_loop(ins_vis_bc_over_int_2d, "ins_vis_bc_over_int_2d", mesh->bfaces,
                op_arg_gbl(&time_n1, 1, DG_FP_STR, OP_READ),
                op_arg_dat(bc_types,       -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bedgeNum, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->gauss->x,  0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->y,  0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->nx, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->gauss->ny, 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_READ),
                op_arg_dat(visBC[0], 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_INC),
                op_arg_dat(visBC[1], 0, mesh->bface2cells, DG_G_NP, DG_FP_STR, OP_INC));
  }
  // Set up RHS for viscosity solve
  op_par_loop(ins_vis_copy_2d, "ins_vis_copy_2d", mesh->cells,
              op_arg_dat(velTT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velTT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(visRHS[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(visRHS[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

   mesh->mass(visRHS[0]);
   mesh->mass(visRHS[1]);

  DG_FP factor = reynolds / dt;
  op_par_loop(mp_ins_vis_rhs_2d, "mp_ins_vis_rhs_2d", mesh->cells,
              op_arg_gbl(&factor, 1, DG_FP_STR, OP_READ),
              op_arg_dat(rho,       -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(visRHS[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
              op_arg_dat(visRHS[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW));

  timer->endTimer("MPINSSolverOverInt2D - Viscosity RHS");

  // Call PETSc linear solver
  timer->startTimer("MPINSSolverOverInt2D - Viscosity Linear Solve");
  factor = g0 * reynolds / dt;
  op_par_loop(mp_ins_vis_mm_fact_2d, "mp_ins_vis_mm_fact_2d", mesh->cells,
              op_arg_gbl(&factor, 1, DG_FP_STR, OP_READ),
              op_arg_dat(rho, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vis_mat_mm_fact, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  viscosityMatrix->set_factor(mu);
  viscosityMatrix->set_mm_factor(vis_mat_mm_fact);
  viscosityMatrix->set_bc_types(vis_bc_types);
  viscosityMatrix->calc_mat();
  viscositySolver->set_bcs(visBC[0]);
  bool convergedX = viscositySolver->solve(visRHS[0], vel[(currentInd + 1) % 2][0]);

  viscositySolver->set_bcs(visBC[1]);
  bool convergedY = viscositySolver->solve(visRHS[1], vel[(currentInd + 1) % 2][1]);
  timer->endTimer("MPINSSolverOverInt2D - Viscosity Linear Solve");

  // timer->startTimer("Filtering");
  // filter(mesh, data->Q[(currentInd + 1) % 2][0]);
  // filter(mesh, data->Q[(currentInd + 1) % 2][1]);
  // timer->endTimer("Filtering");

  return convergedX && convergedY;
}

void MPINSSolverOverInt2D::surface() {
  ls->setVelField(vel[(currentInd + 1) % 2][0], vel[(currentInd + 1) % 2][1]);
  ls->step(dt);
  ls->getRhoMu(rho, mu);
}

// DG_FP MPINSSolverOverInt2D::getAvgPressureConvergance() {
//   return pressurePoisson->getAverageConvergeIter();
// }

// DG_FP MPINSSolverOverInt2D::getAvgViscosityConvergance() {
//   return viscosityPoisson->getAverageConvergeIter();
// }

DG_FP MPINSSolverOverInt2D::get_time() {
  return time;
}

DG_FP MPINSSolverOverInt2D::get_dt() {
  return dt;
}

void MPINSSolverOverInt2D::dump_data(const std::string &filename) {
  timer->startTimer("MPINSSolverOverInt2D - Dump Data");
  op_fetch_data_hdf5_file(mesh->x, filename.c_str());
  op_fetch_data_hdf5_file(mesh->y, filename.c_str());
  op_fetch_data_hdf5_file(vel[0][0], filename.c_str());
  op_fetch_data_hdf5_file(vel[0][1], filename.c_str());
  op_fetch_data_hdf5_file(vel[1][0], filename.c_str());
  op_fetch_data_hdf5_file(vel[1][1], filename.c_str());
  op_fetch_data_hdf5_file(velT[0], filename.c_str());
  op_fetch_data_hdf5_file(velT[1], filename.c_str());
  op_fetch_data_hdf5_file(velTT[0], filename.c_str());
  op_fetch_data_hdf5_file(velTT[1], filename.c_str());
  op_fetch_data_hdf5_file(pr, filename.c_str());
  op_fetch_data_hdf5_file(rho, filename.c_str());
  op_fetch_data_hdf5_file(mu, filename.c_str());
  op_fetch_data_hdf5_file(ls->s, filename.c_str());
  op_fetch_data_hdf5_file(mesh->order, filename.c_str());
  timer->endTimer("MPINSSolverOverInt2D - Dump Data");
}
