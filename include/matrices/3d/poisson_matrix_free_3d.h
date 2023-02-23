#ifndef __INS_POISSON_MATRIX_FREE_3D_H
#define __INS_POISSON_MATRIX_FREE_3D_H

#include "op_seq.h"
#include "dg_mesh/dg_mesh_3d.h"
#include "poisson_semi_matrix_free_3d.h"

class PoissonMatrixFree3D : public PoissonSemiMatrixFree3D {
public:
  PoissonMatrixFree3D(DGMesh3D *m);

  // op_dat bc_types - 0 for Dirichlet, 1 for Neumann
  virtual void calc_mat() override;
  virtual void apply_bc(op_dat rhs, op_dat bc) override;
  virtual void multJacobi(op_dat in, op_dat out) override;
  virtual bool getPETScMat(Mat** mat) override;

protected:
  virtual void calc_op1() override;
  virtual void calc_op2() override;
  virtual void calc_opbc() override;
  virtual void calc_glb_ind() override;
};

#endif
