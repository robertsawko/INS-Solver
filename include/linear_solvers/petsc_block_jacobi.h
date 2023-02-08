#ifndef __PETSC_BLOCK_JACOBI_H
#define __PETSC_BLOCK_JACOBI_H

#include "op_seq.h"
#include "linear_solver.h"
#include "petscvec.h"
#include "petscksp.h"
#include "dg_mesh/dg_mesh_2d.h"

class PETScBlockJacobiSolver : public LinearSolver {
public:
  PETScBlockJacobiSolver(DGMesh2D *m);
  ~PETScBlockJacobiSolver();

  bool solve(op_dat rhs, op_dat ans) override;

  void calc_rhs(const double *in_d, double *out_d);
  void precond(const double *in_d, double *out_d);

private:
  void calc_precond_mat();
  void create_shell_mat();
  void set_shell_pc(PC pc);

  DGMesh2D *mesh;
  KSP ksp;
  op_dat in, out, pre;
  bool pMatInit;
  Mat pMat;
};

#endif