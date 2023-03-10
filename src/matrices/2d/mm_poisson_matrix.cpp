#include "matrices/2d/mm_poisson_matrix_2d.h"

#include "op_seq.h"

#include "dg_constants/dg_constants.h"
#include "timing.h"

extern DGConstants *constants;
extern Timing *timer;

MMPoissonMatrix2D::MMPoissonMatrix2D(DGMesh2D *m) : PoissonMatrix2D(m) {
  factor = 0.0;
}

void MMPoissonMatrix2D::calc_mat() {
  timer->startTimer("MMPoissonMatrix2D - calc_mat");
  calc_glb_ind();
  calc_op1();
  calc_op2();
  calc_opbc();
  calc_mm();
  petscMatResetRequired = true;
  timer->endTimer("MMPoissonMatrix2D - calc_mat");
}

void MMPoissonMatrix2D::set_factor(DG_FP f) {
  factor = f;
}

DG_FP MMPoissonMatrix2D::get_factor() {
  return factor;
}

void MMPoissonMatrix2D::calc_mm() {
  timer->startTimer("MMPoissonMatrix2D - calc_mm");
  op_par_loop(poisson_matrix_2d_mm, "poisson_matrix_2d_mm", mesh->cells,
              op_arg_gbl(&factor, 1, DG_FP_STR, OP_READ),
              op_arg_gbl(constants->get_mat_ptr(DGConstants::MASS), DG_ORDER * DG_NP * DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(mesh->order, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(mesh->J, -1, OP_ID, DG_NP, DG_FP_STR, OP_READ),
              op_arg_dat(op1, -1, OP_ID, DG_NP * DG_NP, DG_FP_STR, OP_INC));
  timer->endTimer("MMPoissonMatrix2D - calc_mm");
}
