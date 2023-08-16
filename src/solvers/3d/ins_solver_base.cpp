#include "solvers/3d/ins_solver.h"

#include "op_seq.h"

#include "dg_op2_blas.h"
#include "dg_constants/dg_constants.h"

extern DGConstants *constants;

#include "timing.h"
#include "config.h"
#include "dg_linear_solvers/petsc_amg.h"
#include "dg_linear_solvers/petsc_block_jacobi.h"
#include "dg_linear_solvers/petsc_pmultigrid.h"
#include "dg_linear_solvers/initial_guess_extrapolation.h"
#include "dg_dat_pool.h"
#include "dg_utils.h"

#include <string>
#include <iostream>
#include <stdexcept>

extern Timing *timer;
extern Config *config;
extern DGDatPool *dg_dat_pool;

INSSolverBase3D::INSSolverBase3D(DGMesh3D *m) {
  mesh = m;

  read_options();
  init_dats();

  std::string name;
  for(int i = 0; i < 3; i++) {
    name = "ins_solver_vel0" + std::to_string(i);
    vel[0][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "ins_solver_vel1" + std::to_string(i);
    vel[1][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "ins_solver_n0" + std::to_string(i);
    n[0][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "ins_solver_n1" + std::to_string(i);
    n[1][i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
  }
  pr = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, "ins_solver_pr");

  dPdN[0] = op_decl_dat(mesh->cells, 4 * DG_NPF, DG_FP_STR, (DG_FP *)NULL, "ins_solver_dPdN0");
  dPdN[1] = op_decl_dat(mesh->cells, 4 * DG_NPF, DG_FP_STR, (DG_FP *)NULL, "ins_solver_dPdN1");
}

INSSolverBase3D::INSSolverBase3D(DGMesh3D *m, const std::string &filename) {
  mesh = m;

  read_options();
  init_dats();

  vel[0][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_vel00");
  vel[1][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_vel10");
  vel[0][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_vel01");
  vel[1][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_vel11");
  vel[0][2] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_vel02");
  vel[1][2] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_vel12");
  n[0][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_n00");
  n[1][0] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_n10");
  n[0][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_n01");
  n[1][1] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_n11");
  n[0][2] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_n02");
  n[1][2] = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_n12");
  pr = op_decl_dat_hdf5(mesh->cells, DG_NP, DG_FP_STR, filename.c_str(), "ins_solver_pr");
  dPdN[0] = op_decl_dat_hdf5(mesh->cells, 4 * DG_NPF, DG_FP_STR, filename.c_str(), "ins_solver_dPdN0");
  dPdN[1] = op_decl_dat_hdf5(mesh->cells, 4 * DG_NPF, DG_FP_STR, filename.c_str(), "ins_solver_dPdN1");
}

void INSSolverBase3D::read_options() {
  int tmp_div = 1;
  config->getInt("solver-options", "div_div", tmp_div);
  div_div_proj = tmp_div != 0;
  config->getInt("solver-options", "sub_cycle", sub_cycles);
  config->getInt("solver-options", "num_iter_before_sub_cycle", it_pre_sub_cycle);
  it_pre_sub_cycle = it_pre_sub_cycle > 1 ? it_pre_sub_cycle : 1;
  int tmp_eig = 1;
  config->getInt("solver-options", "extrapolate_initial_guess", tmp_eig);
  extrapolate_initial_guess = tmp_eig == 1;
  int tmp_shock = 1;
  config->getInt("solver-options", "shock_capturing", tmp_shock);
  shock_cap = tmp_shock == 1;
  int tmp_oia = 0;
  config->getInt("solver-options", "over_int_advec", tmp_oia);
  over_int_advec = tmp_oia == 1;

  filter_max_alpha = 18.0;
  config->getDouble("filter", "max_alpha", filter_max_alpha);
  filter_s0 = -2.0;
  config->getDouble("filter", "s0", filter_s0);
  filter_k = 1.0;
  config->getDouble("filter", "k", filter_k);
  filter_c = 1.0;
  config->getDouble("filter", "c", filter_c);
  int tmp_cutoff = 1;
  config->getInt("filter", "use_cutoff", tmp_cutoff);
  shock_cutoff_filter = tmp_cutoff == 1;
}

void INSSolverBase3D::init_dats() {
  std::string name;
  for(int i = 0; i < 3; i++) {
    name = "ins_solver_velT" + std::to_string(i);
    velT[i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
    name = "ins_solver_velTT" + std::to_string(i);
    velTT[i] = op_decl_dat(mesh->cells, DG_NP, DG_FP_STR, (DG_FP *)NULL, name.c_str());
  }

  bc_types = op_decl_dat(mesh->bfaces, 1, "int", (int *)NULL, "ins_solver_bc_types");

  if(div_div_proj) {
    proj_h = op_decl_dat(mesh->cells, 1, DG_FP_STR, (DG_FP *)NULL, "ins_solver_proj_h");
  }
}

INSSolverBase3D::~INSSolverBase3D() {

}

void INSSolverBase3D::init(const DG_FP re, const DG_FP refVel) {
  // Characteristic length of mesh approximation
  h = 0.0;
  op_par_loop(calc_h_3d, "calc_h_3d", mesh->faces,
              op_arg_dat(mesh->fscale, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_gbl(&h, 1, DG_FP_STR, OP_MAX));
  h = 1.0 / h;
  op_printf("h: %g\n", h);

  // Set up pressure projection
  if(div_div_proj) {
    DGTempDat tmp_npf = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
    op_par_loop(zero_npf_1, "zero_npf_1", mesh->cells,
                op_arg_dat(tmp_npf.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE));

    op_par_loop(ins_proj_setup_0, "ins_3d_proj_setup_0", mesh->faces,
                op_arg_dat(mesh->faceNum, -1, OP_ID, 2, "int", OP_READ),
                op_arg_dat(mesh->fscale, -1, OP_ID, 2, DG_FP_STR, OP_READ),
                op_arg_dat(tmp_npf.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_INC));

    op_par_loop(ins_proj_setup_1, "ins_3d_proj_setup_1", mesh->cells,
                op_arg_dat(tmp_npf.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_READ),
                op_arg_dat(proj_h, -1, OP_ID, 1, DG_FP_STR, OP_WRITE));
    dg_dat_pool->releaseTempDatCells(tmp_npf);
  }

  op_par_loop(zero_np_3, "zero_np_3", mesh->cells,
              op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  op_par_loop(zero_np_3, "zero_np_3", mesh->cells,
              op_arg_dat(velTT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velTT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velTT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
}

void INSSolverBase3D::advec_current_non_linear() {
  timer->startTimer("INSSolverBase3D - advec_current_non_linear");
  DGTempDat f[3][3];
  f[0][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[0][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[0][2] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[1][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[1][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[1][2] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[2][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[2][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[2][2] = dg_dat_pool->requestTempDatCells(DG_NP);
  timer->startTimer("INSSolverBase3D - advec_current_non_linear - 0");
  op_par_loop(ins_3d_advec_0, "ins_3d_advec_0", mesh->cells,
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(f[0][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[0][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[0][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - advec_current_non_linear - 0");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear - div");
  mesh->div(f[0][0].dat, f[0][1].dat, f[0][2].dat, n[currentInd][0]);
  mesh->div(f[1][0].dat, f[1][1].dat, f[1][2].dat, n[currentInd][1]);
  mesh->div(f[2][0].dat, f[2][1].dat, f[2][2].dat, n[currentInd][2]);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear - div");

  dg_dat_pool->releaseTempDatCells(f[0][0]);
  dg_dat_pool->releaseTempDatCells(f[0][1]);
  dg_dat_pool->releaseTempDatCells(f[0][2]);
  dg_dat_pool->releaseTempDatCells(f[1][0]);
  dg_dat_pool->releaseTempDatCells(f[1][1]);
  dg_dat_pool->releaseTempDatCells(f[1][2]);
  dg_dat_pool->releaseTempDatCells(f[2][0]);
  dg_dat_pool->releaseTempDatCells(f[2][1]);
  dg_dat_pool->releaseTempDatCells(f[2][2]);

  DGTempDat tmp_advec_flux0 = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_advec_flux1 = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_advec_flux2 = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);

  timer->startTimer("INSSolverBase3D - advec_current_non_linear - zero");
  op_par_loop(zero_npf_3, "zero_npf_3", mesh->cells,
              op_arg_dat(tmp_advec_flux0.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux1.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux2.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - advec_current_non_linear - zero");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear - 1");
  // Flux across faces
  op_par_loop(ins_3d_advec_1, "ins_3d_advec_1", mesh->faces,
              op_arg_dat(mesh->faceNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(mesh->fmaskL,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(mesh->fmaskR,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(mesh->nx, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->ny, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->nz, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->fscale, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][0], -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_advec_flux0.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux1.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux2.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - advec_current_non_linear - 1");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear - 2");
  // Boundary flux
  if(mesh->bface2cells) {
    op_par_loop(ins_3d_advec_2, "ins_3d_advec_2", mesh->bfaces,
                op_arg_gbl(&time, 1, DG_FP_STR, OP_READ),
                op_arg_dat(bc_types, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bfaceNum, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bnx, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bny, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bnz, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bfscale, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->x, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->y, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->z, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][0], 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][1], 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][2], 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(tmp_advec_flux0.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_INC),
                op_arg_dat(tmp_advec_flux1.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_INC),
                op_arg_dat(tmp_advec_flux2.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_INC));
  }
  timer->endTimer("INSSolverBase3D - advec_current_non_linear - 2");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear - LIFT");
  op2_gemv(mesh, false, 1.0, DGConstants::LIFT, tmp_advec_flux0.dat, 1.0, n[currentInd][0]);
  op2_gemv(mesh, false, 1.0, DGConstants::LIFT, tmp_advec_flux1.dat, 1.0, n[currentInd][1]);
  op2_gemv(mesh, false, 1.0, DGConstants::LIFT, tmp_advec_flux2.dat, 1.0, n[currentInd][2]);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear - LIFT");

  dg_dat_pool->releaseTempDatCells(tmp_advec_flux0);
  dg_dat_pool->releaseTempDatCells(tmp_advec_flux1);
  dg_dat_pool->releaseTempDatCells(tmp_advec_flux2);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear");
}

void INSSolverBase3D::advec_current_non_linear_over_int() {
  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int");
  DGTempDat f[3][3];
  f[0][0] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[0][1] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[0][2] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[1][0] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[1][1] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[1][2] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[2][0] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[2][1] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[2][2] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - Interp");
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, vel[currentInd][0], 0.0, f[0][0].dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, vel[currentInd][1], 0.0, f[0][1].dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, vel[currentInd][2], 0.0, f[0][2].dat);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - Interp");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - 0");
  op_par_loop(ins_3d_advec_oi_0, "ins_3d_advec_oi_0", mesh->cells,
              op_arg_dat(mesh->geof, -1, OP_ID, 10, DG_FP_STR, OP_READ),
              op_arg_dat(f[0][0].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[0][1].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[0][2].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[1][0].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][1].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][2].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][0].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][1].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][2].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - 0");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - div");
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDR, f[0][0].dat, 0.0, n[currentInd][0]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDS, f[0][1].dat, 1.0, n[currentInd][0]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDT, f[0][2].dat, 1.0, n[currentInd][0]);

  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDR, f[1][0].dat, 0.0, n[currentInd][1]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDS, f[1][1].dat, 1.0, n[currentInd][1]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDT, f[1][2].dat, 1.0, n[currentInd][1]);

  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDR, f[2][0].dat, 0.0, n[currentInd][2]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDS, f[2][1].dat, 1.0, n[currentInd][2]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDT, f[2][2].dat, 1.0, n[currentInd][2]);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - div");

  dg_dat_pool->releaseTempDatCells(f[0][0]);
  dg_dat_pool->releaseTempDatCells(f[0][1]);
  dg_dat_pool->releaseTempDatCells(f[0][2]);
  dg_dat_pool->releaseTempDatCells(f[1][0]);
  dg_dat_pool->releaseTempDatCells(f[1][1]);
  dg_dat_pool->releaseTempDatCells(f[1][2]);
  dg_dat_pool->releaseTempDatCells(f[2][0]);
  dg_dat_pool->releaseTempDatCells(f[2][1]);
  dg_dat_pool->releaseTempDatCells(f[2][2]);

  DGTempDat tmp_mU = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_mV = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_mW = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pU = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pV = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pW = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);

/*
  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - zero");
  op_par_loop(zero_npf_3, "zero_npf_3", mesh->cells,
              op_arg_dat(tmp_mU.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mV.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mW.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  op_par_loop(zero_npf_3, "zero_npf_3", mesh->cells,
              op_arg_dat(tmp_pU.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pV.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pW.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - zero");
*/

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - 1");
  // Flux across faces
  op_par_loop(ins_3d_advec_oi_1, "ins_3d_advec_oi_1", mesh->faces,
              op_arg_dat(mesh->faceNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(mesh->fmaskL,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(mesh->fmaskR,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(vel[currentInd][0], -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mU.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mV.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mW.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pU.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pV.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pW.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - 1");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - 2");
  // Boundary flux
  if(mesh->bface2cells) {
    // Needs more testing
    op_par_loop(ins_3d_advec_oi_2, "ins_3d_advec_oi_2", mesh->bfaces,
                op_arg_gbl(&time, 1, DG_FP_STR, OP_READ),
                op_arg_dat(bc_types, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bfaceNum, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bnx, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bny, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bnz, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bfscale, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->x, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->y, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->z, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][0], 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][1], 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][2], 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(tmp_mU.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_mV.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_mW.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pU.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pV.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pW.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  }
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - 2");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - Interp Surf");
  DGTempDat tmp_mU_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_mV_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_mW_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pU_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pV_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pW_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);

  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mU.dat, 0.0, tmp_mU_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mV.dat, 0.0, tmp_mV_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mW.dat, 0.0, tmp_mW_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pU.dat, 0.0, tmp_pU_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pV.dat, 0.0, tmp_pV_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pW.dat, 0.0, tmp_pW_cub.dat);

  dg_dat_pool->releaseTempDatCells(tmp_mU);
  dg_dat_pool->releaseTempDatCells(tmp_mV);
  dg_dat_pool->releaseTempDatCells(tmp_mW);
  dg_dat_pool->releaseTempDatCells(tmp_pU);
  dg_dat_pool->releaseTempDatCells(tmp_pV);
  dg_dat_pool->releaseTempDatCells(tmp_pW);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - Interp Surf");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - 3");
  op_par_loop(ins_3d_advec_oi_3, "ins_3d_advec_oi_3", mesh->cells,
              op_arg_dat(mesh->nx_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->ny_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->nz_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->sJ_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->geof, -1, OP_ID, 10, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mU_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mV_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mW_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_pU_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(tmp_pV_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(tmp_pW_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_RW));
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - 3");

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - LIFT");
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_LIFT, tmp_pU_cub.dat, -1.0, n[currentInd][0]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_LIFT, tmp_pV_cub.dat, -1.0, n[currentInd][1]);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_LIFT, tmp_pW_cub.dat, -1.0, n[currentInd][2]);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - LIFT");

  dg_dat_pool->releaseTempDatCells(tmp_mU_cub);
  dg_dat_pool->releaseTempDatCells(tmp_mV_cub);
  dg_dat_pool->releaseTempDatCells(tmp_mW_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pU_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pV_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pW_cub);

  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int");
}

void INSSolverBase3D::advec_standard() {
  if(over_int_advec) {
    advec_current_non_linear_over_int();
  } else {
    advec_current_non_linear();
  }

  // Calculate the intermediate velocity values
  op_par_loop(ins_3d_advec_3, "ins_3d_advec_3", mesh->cells,
              op_arg_gbl(&a0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&a1, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&b0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&b1, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&dt, 1, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(n[(currentInd + 1) % 2][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
}

void INSSolverBase3D::advec_sub_cycle_rk_step(const DG_FP time_sc, op_dat u, op_dat v, op_dat w) {
  timer->startTimer("INSSolverBase3D - RK");
  // Request temporary dats
  DGTempDat advec_sc_rk[3][3];
  advec_sc_rk[0][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[0][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[0][2] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[1][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[1][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[1][2] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[2][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[2][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_rk[2][2] = dg_dat_pool->requestTempDatCells(DG_NP);

  op_par_loop(ins_3d_advec_sc_copy, "ins_3d_advec_sc_copy", mesh->cells,
              op_arg_dat(u, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(v, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(w, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_rk[0][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc_rk[0][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc_rk[0][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  for(int rk_step = 0; rk_step < 3; rk_step++) {
    double rk_time = time_sc;
    if(rk_step == 1) rk_time += sub_cycle_dt;
    if(rk_step == 2) rk_time += 0.5 * sub_cycle_dt;
    const int rk_ind = rk_step == 2 ? 2 : rk_step + 1;
    timer->startTimer("INSSolverBase3D - RK RHS");
    if(over_int_advec) {
      advec_sub_cycle_rhs_over_int(advec_sc_rk[0][0].dat, advec_sc_rk[0][1].dat, advec_sc_rk[0][2].dat,
                        advec_sc_rk[rk_ind][0].dat, advec_sc_rk[rk_ind][1].dat,
                        advec_sc_rk[rk_ind][2].dat, rk_time);
    } else {
      advec_sub_cycle_rhs(advec_sc_rk[0][0].dat, advec_sc_rk[0][1].dat, advec_sc_rk[0][2].dat,
                        advec_sc_rk[rk_ind][0].dat, advec_sc_rk[rk_ind][1].dat,
                        advec_sc_rk[rk_ind][2].dat, rk_time);
    }
    timer->endTimer("INSSolverBase3D - RK RHS");
    // Set up next step
    if(rk_step == 0) {
      op_par_loop(ins_3d_advec_sc_rk_0, "ins_3d_advec_sc_rk_0", mesh->cells,
                  op_arg_gbl(&sub_cycle_dt, 1, DG_FP_STR, OP_READ),
                  op_arg_dat(u, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(v, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(w, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[1][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[1][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[1][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[0][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                  op_arg_dat(advec_sc_rk[0][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                  op_arg_dat(advec_sc_rk[0][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
    } else if(rk_step == 1) {
      op_par_loop(ins_3d_advec_sc_rk_1, "ins_3d_advec_sc_rk_1", mesh->cells,
                  op_arg_gbl(&sub_cycle_dt, 1, DG_FP_STR, OP_READ),
                  op_arg_dat(u, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(v, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(w, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[1][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
                  op_arg_dat(advec_sc_rk[1][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
                  op_arg_dat(advec_sc_rk[1][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
                  op_arg_dat(advec_sc_rk[2][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[2][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[2][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                  op_arg_dat(advec_sc_rk[0][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                  op_arg_dat(advec_sc_rk[0][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                  op_arg_dat(advec_sc_rk[0][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
    }
  }
  // Update velT
  op_par_loop(ins_3d_advec_sc_rk_2, "ins_3d_advec_sc_rk_2", mesh->cells,
              op_arg_gbl(&sub_cycle_dt, 1, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_rk[1][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_rk[1][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_rk[1][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_rk[2][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_rk[2][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_rk[2][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(u, -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
              op_arg_dat(v, -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
              op_arg_dat(w, -1, OP_ID, DG_NP, DG_FP_STR, OP_RW));

  dg_dat_pool->releaseTempDatCells(advec_sc_rk[0][0]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[0][1]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[0][2]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[1][0]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[1][1]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[1][2]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[2][0]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[2][1]);
  dg_dat_pool->releaseTempDatCells(advec_sc_rk[2][2]);
  timer->endTimer("INSSolverBase3D - RK");
}

void INSSolverBase3D::advec_sub_cycle() {
  op_par_loop(ins_3d_advec_sc_copy, "ins_3d_advec_sc_copy", mesh->cells,
              op_arg_dat(vel[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(velT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  timer->startTimer("INSSolverBase3D - Subcycle 0");
  // Advance 2 * number of subcycles
  for(int i = 0; i < 2 * sub_cycles; i++) {
    advec_sub_cycle_rk_step(time - dt + i * sub_cycle_dt, velT[0], velT[1], velT[2]);
  }
  timer->endTimer("INSSolverBase3D - Subcycle 0");

  DGTempDat advec_sc_tmp[3];
  advec_sc_tmp[0] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_tmp[1] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc_tmp[2] = dg_dat_pool->requestTempDatCells(DG_NP);

  // Other velocity
  op_par_loop(ins_3d_advec_sc_copy, "ins_3d_advec_sc_copy", mesh->cells,
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_tmp[0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc_tmp[1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc_tmp[2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  timer->startTimer("INSSolverBase3D - Subcycle 1");
  // Advance number of subcycles
  for(int i = 0; i < sub_cycles; i++) {
    advec_sub_cycle_rk_step(time + i * sub_cycle_dt, advec_sc_tmp[0].dat, advec_sc_tmp[1].dat, advec_sc_tmp[2].dat);
  }
  timer->endTimer("INSSolverBase3D - Subcycle 1");

  // Get final velT
  op_par_loop(ins_3d_advec_sc_update, "ins_3d_advec_sc_update", mesh->cells,
              op_arg_gbl(&a0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&a1, 1, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_tmp[0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_tmp[1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc_tmp[2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
              op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
              op_arg_dat(velT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW));

  dg_dat_pool->releaseTempDatCells(advec_sc_tmp[0]);
  dg_dat_pool->releaseTempDatCells(advec_sc_tmp[1]);
  dg_dat_pool->releaseTempDatCells(advec_sc_tmp[2]);

  // Needed for Pressure boundary conditions
  if(over_int_advec) {
    advec_current_non_linear_over_int();
  } else {
    advec_current_non_linear();
  }
}

void INSSolverBase3D::advec_sub_cycle_rhs(op_dat u_in, op_dat v_in, op_dat w_in,
                                      op_dat u_out, op_dat v_out, op_dat w_out,
                                      const double t) {
  double t0 = time - dt;
  double t1 = time;
  double tI = t;
  DGTempDat advec_sc[3];
  advec_sc[0] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc[1] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc[2] = dg_dat_pool->requestTempDatCells(DG_NP);

  timer->startTimer("INSSolverBase3D - RK RHS Interp");
  op_par_loop(ins_3d_advec_sc_interp, "ins_3d_advec_sc_interp", mesh->cells,
              op_arg_gbl(&t0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&t1, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&tI, 1, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc[1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc[2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS Interp");
  DGTempDat f[3][3];
  f[0][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[0][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[0][2] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[1][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[1][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[1][2] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[2][0] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[2][1] = dg_dat_pool->requestTempDatCells(DG_NP);
  f[2][2] = dg_dat_pool->requestTempDatCells(DG_NP);

  timer->startTimer("INSSolverBase3D - RK RHS 0");
  op_par_loop(ins_3d_advec_sc_rhs_0, "ins_3d_advec_sc_rhs_0", mesh->cells,
              op_arg_dat(u_in, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(v_in, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(w_in, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(f[0][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[0][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[0][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[1][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS 0");

  timer->startTimer("INSSolverBase3D - RK RHS div");
  mesh->div(f[0][0].dat, f[0][1].dat, f[0][2].dat, u_out);
  mesh->div(f[1][0].dat, f[1][1].dat, f[1][2].dat, v_out);
  mesh->div(f[2][0].dat, f[2][1].dat, f[2][2].dat, w_out);
  timer->endTimer("INSSolverBase3D - RK RHS div");

  dg_dat_pool->releaseTempDatCells(f[0][0]);
  dg_dat_pool->releaseTempDatCells(f[0][1]);
  dg_dat_pool->releaseTempDatCells(f[0][2]);
  dg_dat_pool->releaseTempDatCells(f[1][0]);
  dg_dat_pool->releaseTempDatCells(f[1][1]);
  dg_dat_pool->releaseTempDatCells(f[1][2]);
  dg_dat_pool->releaseTempDatCells(f[2][0]);
  dg_dat_pool->releaseTempDatCells(f[2][1]);
  dg_dat_pool->releaseTempDatCells(f[2][2]);

  DGTempDat tmp_advec_flux0 = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_advec_flux1 = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_advec_flux2 = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);

  timer->startTimer("INSSolverBase3D - RK RHS zero");
  op_par_loop(zero_npf_3, "zero_npf_3", mesh->cells,
              op_arg_dat(tmp_advec_flux0.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux1.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux2.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS zero");

  timer->startTimer("INSSolverBase3D - RK RHS 1");
  op_par_loop(ins_3d_advec_sc_rhs_1, "ins_3d_advec_sc_rhs_1", mesh->faces,
              op_arg_dat(mesh->faceNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(mesh->fmaskL,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(mesh->fmaskR,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(mesh->nx, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->ny, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->nz, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->fscale, -1, OP_ID, 2, DG_FP_STR, OP_READ),
              op_arg_dat(u_in, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(v_in, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(w_in, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[0].dat, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[1].dat, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[2].dat, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_advec_flux0.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux1.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux2.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS 1");

  timer->startTimer("INSSolverBase3D - RK RHS 2");
  if(mesh->bface2cells) {
    op_par_loop(ins_3d_advec_sc_rhs_2, "ins_3d_advec_sc_rhs_2", mesh->bfaces,
                op_arg_dat(bc_types, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bfaceNum, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bnx, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bny, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bnz, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bfscale, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->x, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->y, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->z, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(u_in, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(v_in, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(w_in, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(advec_sc[0].dat, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(advec_sc[1].dat, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(advec_sc[2].dat, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(tmp_advec_flux0.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_INC),
                op_arg_dat(tmp_advec_flux1.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_INC),
                op_arg_dat(tmp_advec_flux2.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_INC));
  }
  timer->endTimer("INSSolverBase3D - RK RHS 2");

  dg_dat_pool->releaseTempDatCells(advec_sc[0]);
  dg_dat_pool->releaseTempDatCells(advec_sc[1]);
  dg_dat_pool->releaseTempDatCells(advec_sc[2]);

  timer->startTimer("INSSolverBase3D - RK RHS LIFT");
  op2_gemv(mesh, false, 1.0, DGConstants::LIFT, tmp_advec_flux0.dat, 1.0, u_out);
  op2_gemv(mesh, false, 1.0, DGConstants::LIFT, tmp_advec_flux1.dat, 1.0, v_out);
  op2_gemv(mesh, false, 1.0, DGConstants::LIFT, tmp_advec_flux2.dat, 1.0, w_out);
  timer->endTimer("INSSolverBase3D - RK RHS LIFT");

  dg_dat_pool->releaseTempDatCells(tmp_advec_flux0);
  dg_dat_pool->releaseTempDatCells(tmp_advec_flux1);
  dg_dat_pool->releaseTempDatCells(tmp_advec_flux2);
}

// TODO redo - uses too much memory
void INSSolverBase3D::advec_sub_cycle_rhs_over_int(op_dat u_in, op_dat v_in, op_dat w_in,
                                      op_dat u_out, op_dat v_out, op_dat w_out,
                                      const double t) {
  double t0 = time - dt;
  double t1 = time;
  double tI = t;
  DGTempDat advec_sc[3];
  advec_sc[0] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc[1] = dg_dat_pool->requestTempDatCells(DG_NP);
  advec_sc[2] = dg_dat_pool->requestTempDatCells(DG_NP);

  timer->startTimer("INSSolverBase3D - RK RHS over_int Interp");
  op_par_loop(ins_3d_advec_sc_interp, "ins_3d_advec_sc_interp", mesh->cells,
              op_arg_gbl(&t0, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&t1, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(&tI, 1, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[(currentInd + 1) % 2][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc[1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(advec_sc[2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS over_int Interp");
  DGTempDat f[3][3];
  f[0][0] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[0][1] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[0][2] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[1][0] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[1][1] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[1][2] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[2][0] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[2][1] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);
  f[2][2] = dg_dat_pool->requestTempDatCells(DG_CUB_3D_NP);

  timer->startTimer("INSSolverBase3D - RK RHS over_int - Cub Interp");
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, u_in, 0.0, f[0][0].dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, v_in, 0.0, f[0][1].dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, w_in, 0.0, f[0][2].dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, advec_sc[0].dat, 0.0, f[1][0].dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, advec_sc[1].dat, 0.0, f[1][1].dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_INTERP, advec_sc[2].dat, 0.0, f[1][2].dat);
  timer->endTimer("INSSolverBase3D - RK RHS over_int - Cub Interp");

  timer->startTimer("INSSolverBase3D - RK RHS over_int 0");
  op_par_loop(ins_3d_advec_sc_rhs_oi_0, "ins_3d_advec_sc_rhs_oi_0", mesh->cells,
              op_arg_dat(mesh->geof, -1, OP_ID, 10, DG_FP_STR, OP_READ),
              op_arg_dat(f[0][0].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[0][1].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[0][2].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[1][0].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[1][1].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[1][2].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(f[2][0].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][1].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE),
              op_arg_dat(f[2][2].dat, -1, OP_ID, DG_CUB_3D_NP, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS over_int 0");

  timer->startTimer("INSSolverBase3D - RK RHS over_int div");
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDR, f[0][0].dat, 0.0, u_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDS, f[0][1].dat, 1.0, u_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDT, f[0][2].dat, 1.0, u_out);

  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDR, f[1][0].dat, 0.0, v_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDS, f[1][1].dat, 1.0, v_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDT, f[1][2].dat, 1.0, v_out);

  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDR, f[2][0].dat, 0.0, w_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDS, f[2][1].dat, 1.0, w_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUB3D_PDT, f[2][2].dat, 1.0, w_out);
  timer->endTimer("INSSolverBase3D - RK RHS over_int div");

  dg_dat_pool->releaseTempDatCells(f[0][0]);
  dg_dat_pool->releaseTempDatCells(f[0][1]);
  dg_dat_pool->releaseTempDatCells(f[0][2]);
  dg_dat_pool->releaseTempDatCells(f[1][0]);
  dg_dat_pool->releaseTempDatCells(f[1][1]);
  dg_dat_pool->releaseTempDatCells(f[1][2]);
  dg_dat_pool->releaseTempDatCells(f[2][0]);
  dg_dat_pool->releaseTempDatCells(f[2][1]);
  dg_dat_pool->releaseTempDatCells(f[2][2]);

  DGTempDat tmp_mUs = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_mVs = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_mWs = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pUs = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pVs = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pWs = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_mUb = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_mVb = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_mWb = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pUb = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pVb = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);
  DGTempDat tmp_pWb = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_NPF);

/*
  timer->startTimer("INSSolverBase3D - RK RHS zero");
  op_par_loop(zero_npf_3, "zero_npf_3", mesh->cells,
              op_arg_dat(tmp_advec_flux0.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux1.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_advec_flux2.dat, -1, OP_ID, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS zero");
*/

  timer->startTimer("INSSolverBase3D - RK RHS over_int 1");
  op_par_loop(ins_3d_advec_sc_rhs_oi_1, "ins_3d_advec_sc_rhs_oi_1", mesh->faces,
              op_arg_dat(mesh->faceNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(mesh->fmaskL,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(mesh->fmaskR,  -1, OP_ID, DG_NPF, "int", OP_READ),
              op_arg_dat(u_in, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(v_in, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(w_in, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[0].dat, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[1].dat, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(advec_sc[2].dat, -2, mesh->face2cells, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mUs.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mVs.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mWs.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pUs.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pVs.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pWs.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mUb.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mVb.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_mWb.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pUb.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pVb.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
              op_arg_dat(tmp_pWb.dat, -2, mesh->face2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  timer->endTimer("INSSolverBase3D - RK RHS over_int 1");

  timer->startTimer("INSSolverBase3D - RK RHS over_int 2");
  if(mesh->bface2cells) {
    op_par_loop(ins_3d_advec_sc_rhs_oi_2, "ins_3d_advec_sc_rhs_oi_2", mesh->bfaces,
                op_arg_dat(bc_types, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bfaceNum, -1, OP_ID, 1, "int", OP_READ),
                op_arg_dat(mesh->bnx, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bny, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->bnz, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->x, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->y, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->z, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(u_in, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(v_in, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(w_in, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(advec_sc[0].dat, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(advec_sc[1].dat, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(advec_sc[2].dat, 0, mesh->bface2cells, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(tmp_mUs.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_mVs.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_mWs.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pUs.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pVs.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pWs.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_mUb.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_mVb.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_mWb.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pUb.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pVb.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE),
                op_arg_dat(tmp_pWb.dat, 0, mesh->bface2cells, 4 * DG_NPF, DG_FP_STR, OP_WRITE));
  }
  timer->endTimer("INSSolverBase3D - RK RHS over_int 2");

  dg_dat_pool->releaseTempDatCells(advec_sc[0]);
  dg_dat_pool->releaseTempDatCells(advec_sc[1]);
  dg_dat_pool->releaseTempDatCells(advec_sc[2]);

  timer->startTimer("INSSolverBase3D - advec_current_non_linear_over_int - Interp Surf");
  DGTempDat tmp_mUs_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_mVs_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_mWs_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pUs_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pVs_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pWs_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_mUb_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_mVb_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_mWb_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pUb_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pVb_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);
  DGTempDat tmp_pWb_cub = dg_dat_pool->requestTempDatCells(DG_NUM_FACES * DG_CUB_SURF_3D_NP);

  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mUs.dat, 0.0, tmp_mUs_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mVs.dat, 0.0, tmp_mVs_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mWs.dat, 0.0, tmp_mWs_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pUs.dat, 0.0, tmp_pUs_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pVs.dat, 0.0, tmp_pVs_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pWs.dat, 0.0, tmp_pWs_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mUb.dat, 0.0, tmp_mUb_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mVb.dat, 0.0, tmp_mVb_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_mWb.dat, 0.0, tmp_mWb_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pUb.dat, 0.0, tmp_pUb_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pVb.dat, 0.0, tmp_pVb_cub.dat);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_INTERP, tmp_pWb.dat, 0.0, tmp_pWb_cub.dat);

  dg_dat_pool->releaseTempDatCells(tmp_mUs);
  dg_dat_pool->releaseTempDatCells(tmp_mVs);
  dg_dat_pool->releaseTempDatCells(tmp_mWs);
  dg_dat_pool->releaseTempDatCells(tmp_pUs);
  dg_dat_pool->releaseTempDatCells(tmp_pVs);
  dg_dat_pool->releaseTempDatCells(tmp_pWs);
  dg_dat_pool->releaseTempDatCells(tmp_mUb);
  dg_dat_pool->releaseTempDatCells(tmp_mVb);
  dg_dat_pool->releaseTempDatCells(tmp_mWb);
  dg_dat_pool->releaseTempDatCells(tmp_pUb);
  dg_dat_pool->releaseTempDatCells(tmp_pVb);
  dg_dat_pool->releaseTempDatCells(tmp_pWb);
  timer->endTimer("INSSolverBase3D - advec_current_non_linear_over_int - Interp Surf");

  timer->startTimer("INSSolverBase3D - RK RHS over_int 3");
  op_par_loop(ins_3d_advec_sc_rhs_oi_3, "ins_3d_advec_sc_rhs_oi_3", mesh->cells,
              op_arg_dat(mesh->nx_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->ny_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->nz_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->sJ_c, -1, OP_ID, 4, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->geof, -1, OP_ID, 10, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mUs_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mVs_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mWs_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_pUs_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_pVs_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_pWs_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mUb_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mVb_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_mWb_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_READ),
              op_arg_dat(tmp_pUb_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(tmp_pVb_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_RW),
              op_arg_dat(tmp_pWb_cub.dat, -1, OP_ID, DG_NUM_FACES * DG_CUB_SURF_3D_NP, DG_FP_STR, OP_RW));
  timer->endTimer("NSSolverBase3D - RK RHS over_int 3");

  timer->startTimer("INSSolverBase3D - RK RHS over_int LIFT");
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_LIFT, tmp_pUb_cub.dat, -1.0, u_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_LIFT, tmp_pVb_cub.dat, -1.0, v_out);
  op2_gemv(mesh, false, 1.0, DGConstants::CUBSURF3D_LIFT, tmp_pWb_cub.dat, -1.0, w_out);
  timer->endTimer("INSSolverBase3D - RK RHSover_int LIFT");

  dg_dat_pool->releaseTempDatCells(tmp_mUs_cub);
  dg_dat_pool->releaseTempDatCells(tmp_mVs_cub);
  dg_dat_pool->releaseTempDatCells(tmp_mWs_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pUs_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pVs_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pWs_cub);
  dg_dat_pool->releaseTempDatCells(tmp_mUb_cub);
  dg_dat_pool->releaseTempDatCells(tmp_mVb_cub);
  dg_dat_pool->releaseTempDatCells(tmp_mWb_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pUb_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pVb_cub);
  dg_dat_pool->releaseTempDatCells(tmp_pWb_cub);
}

void INSSolverBase3D::project_velocity(op_dat dpdx, op_dat dpdy, op_dat dpdz) {
  if(div_div_proj) {
    DGTempDat projRHS[3];
    projRHS[0] = dg_dat_pool->requestTempDatCells(DG_NP);
    projRHS[1] = dg_dat_pool->requestTempDatCells(DG_NP);
    projRHS[2] = dg_dat_pool->requestTempDatCells(DG_NP);

    op_par_loop(ins_3d_proj_rhs, "ins_3d_proj_rhs", mesh->cells,
                op_arg_gbl(&dt, 1, DG_FP_STR, OP_READ),
                op_arg_dat(dpdx, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(dpdy, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(dpdz, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velTT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(velTT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(velTT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(projRHS[0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(projRHS[1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(projRHS[2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

    mesh->mass(projRHS[0].dat);
    mesh->mass(projRHS[1].dat);
    mesh->mass(projRHS[2].dat);

    DGTempDat proj_pen = dg_dat_pool->requestTempDatCells(1);
    DG_FP factor = dt * 1.0;
    // DG_FP factor = dt / Cr;
    // op_printf("Cr: %g\n", Cr);
    op_par_loop(ins_3d_proj_pen, "ins_3d_proj_pen", mesh->cells,
                op_arg_gbl(&factor, 1, DG_FP_STR, OP_READ),
                op_arg_gbl(&a0, 1, DG_FP_STR, OP_READ),
                op_arg_gbl(&a1, 1, DG_FP_STR, OP_READ),
                op_arg_dat(mesh->geof, -1, OP_ID, 10, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[(currentInd + 1) % 2][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[(currentInd + 1) % 2][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(vel[(currentInd + 1) % 2][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(proj_h, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(proj_pen.dat, -1, OP_ID, 1, DG_FP_STR, OP_WRITE));

    int num_cells = 0;
    int num_converge = 0;
    DG_FP num_iter = 0.0;
    op_par_loop(ins_3d_proj_cg_precon, "ins_3d_proj_cg_precon", mesh->cells,
                op_arg_dat(mesh->geof, -1, OP_ID, 10, DG_FP_STR, OP_READ),
                op_arg_dat(proj_pen.dat, -1, OP_ID, 1, DG_FP_STR, OP_READ),
                op_arg_dat(projRHS[0].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(projRHS[1].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(projRHS[2].dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velTT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
                op_arg_dat(velTT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
                op_arg_dat(velTT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_RW),
                op_arg_gbl(&num_cells, 1, "int", OP_INC),
                op_arg_gbl(&num_converge, 1, "int", OP_INC),
                op_arg_gbl(&num_iter, 1, DG_FP_STR, OP_INC));

    dg_dat_pool->releaseTempDatCells(proj_pen);
    dg_dat_pool->releaseTempDatCells(projRHS[0]);
    dg_dat_pool->releaseTempDatCells(projRHS[1]);
    dg_dat_pool->releaseTempDatCells(projRHS[2]);
    if(num_cells != num_converge) {
      op_printf("%d out of %d cells converged on projection step\n", num_converge, num_cells);
      exit(-1);
    }
  } else {
    op_par_loop(ins_3d_pr_3, "ins_3d_pr_3", mesh->cells,
                op_arg_gbl(&dt, 1, DG_FP_STR, OP_READ),
                op_arg_dat(dpdx, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(dpdy, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(dpdz, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
                op_arg_dat(velTT[0], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(velTT[1], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE),
                op_arg_dat(velTT[2], -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));
  }
}

void INSSolverBase3D::shock_capture_filter_dat(op_dat in) {
  DGTempDat u_hat = dg_dat_pool->requestTempDatCells(DG_NP);
  DGTempDat u_modal = dg_dat_pool->requestTempDatCells(DG_NP);
  op2_gemv(mesh, false, 1.0, DGConstants::INV_V, in, 0.0, u_modal.dat);

  const double *r_ptr = constants->get_mat_ptr(DGConstants::R) + (DG_ORDER - 1) * DG_NP;
  const double *s_ptr = constants->get_mat_ptr(DGConstants::S) + (DG_ORDER - 1) * DG_NP;
  const double *t_ptr = constants->get_mat_ptr(DGConstants::T) + (DG_ORDER - 1) * DG_NP;

  std::vector<DG_FP> r_vec, s_vec, t_vec;
  for(int i = 0; i < DG_NP; i++) {
    r_vec.push_back(r_ptr[i]);
    s_vec.push_back(s_ptr[i]);
    t_vec.push_back(t_ptr[i]);
  }

  std::vector<DG_FP> simplex_vals = DGUtils::val_at_pt_N_1_3d_get_simplexes(r_vec, s_vec, t_vec, DG_ORDER);

  op_par_loop(discont_sensor_0, "discont_sensor_0", mesh->cells,
              op_arg_gbl(simplex_vals.data(), DG_NP * DG_NP, "double", OP_READ),
              op_arg_dat(in, -1, OP_ID, DG_NP, "double", OP_READ),
              op_arg_dat(u_modal.dat, -1, OP_ID, DG_NP, "double", OP_READ),
              op_arg_dat(u_hat.dat, -1, OP_ID, DG_NP, "double", OP_WRITE));

  double max_alpha = filter_max_alpha;
  // double e0 = h;
  // double s0 = 1.0 / (double)(DG_ORDER * DG_ORDER * DG_ORDER * DG_ORDER);
  double s0 = filter_s0;
  // double k  = 5.0;
  double k = filter_k;
  double c = filter_c;
  if(shock_cutoff_filter) {
    op_par_loop(discont_sensor_cutoff_filter, "discont_sensor_cutoff_filter", mesh->cells,
                op_arg_gbl(&max_alpha, 1, "double", OP_READ),
                op_arg_gbl(&s0, 1, "double", OP_READ),
                op_arg_gbl(&k,  1, "double", OP_READ),
                op_arg_gbl(&c,  1, "double", OP_READ),
                op_arg_dat(mesh->geof, -1, OP_ID, 10, "double", OP_READ),
                op_arg_dat(in, -1, OP_ID, DG_NP, "double", OP_READ),
                op_arg_dat(u_hat.dat, -1, OP_ID, DG_NP, "double", OP_READ),
                op_arg_dat(u_modal.dat, -1, OP_ID, DG_NP, "double", OP_RW));
  } else {
    op_par_loop(discont_sensor_filter, "discont_sensor_filter", mesh->cells,
                op_arg_gbl(&max_alpha, 1, "double", OP_READ),
                op_arg_gbl(&s0, 1, "double", OP_READ),
                op_arg_gbl(&k,  1, "double", OP_READ),
                op_arg_gbl(&c,  1, "double", OP_READ),
                op_arg_gbl(&dt,  1, "double", OP_READ),
                op_arg_dat(mesh->geof, -1, OP_ID, 10, "double", OP_READ),
                op_arg_dat(in, -1, OP_ID, DG_NP, "double", OP_READ),
                op_arg_dat(u_hat.dat, -1, OP_ID, DG_NP, "double", OP_READ),
                op_arg_dat(u_modal.dat, -1, OP_ID, DG_NP, "double", OP_RW));
  }

  op2_gemv(mesh, false, 1.0, DGConstants::V, u_modal.dat, 0.0, in);

  dg_dat_pool->releaseTempDatCells(u_hat);
  dg_dat_pool->releaseTempDatCells(u_modal);
}

void INSSolverBase3D::add_to_pr_history() {
  const DG_FP t_n1 = time + dt;
  DGTempDat pr_copy = dg_dat_pool->requestTempDatCells(DG_NP);

  op_par_loop(copy_dg_np, "copy_dg_np", mesh->cells,
              op_arg_dat(pr, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(pr_copy.dat, -1, OP_ID, DG_NP, DG_FP_STR, OP_WRITE));

  pr_history.push_back({t_n1, pr_copy});

  while(pr_history.size() > EXTRAPOLATE_HISTORY_SIZE) {
    dg_dat_pool->releaseTempDatCells(pr_history[0].second);
    pr_history.erase(pr_history.begin());
  }
}

DG_FP INSSolverBase3D::get_time() {
  return time;
}

DG_FP INSSolverBase3D::get_dt() {
  return dt;
}

DG_FP INSSolverBase3D::max_vel() {
  DG_FP max_vel_tmp = 0.0;

  op_par_loop(ins_3d_max_vel, "ins_3d_max_vel", mesh->cells,
              op_arg_gbl(&max_vel_tmp, 1, DG_FP_STR, OP_MAX)
              op_arg_dat(vel[currentInd][0], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][1], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(vel[currentInd][2], -1, OP_ID, DG_NP, DG_FP_STR, OP_READ));

  return std::max(1.0, sqrt(max_vel_tmp));
}
