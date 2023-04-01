#ifndef __INS_FACTOR_POISSON_MATRIX_FREE_MULT_3D_H
#define __INS_FACTOR_POISSON_MATRIX_FREE_MULT_3D_H

#include "op_seq.h"
#include "dg_mesh/dg_mesh_3d.h"
#include "poisson_matrix_free_mult_3d.h"

class FactorPoissonMatrixFreeMult3D : public PoissonMatrixFreeMult3D {
public:
  FactorPoissonMatrixFreeMult3D(DGMesh3D *m, bool alloc_tmp_dats = true);

  // op_dat bc_types - 0 for Dirichlet, 1 for Neumann
  virtual void mat_free_apply_bc(op_dat rhs, op_dat bc);
  virtual void mat_free_mult(op_dat in, op_dat out);
  virtual void mat_free_set_factor(op_dat f);

protected:
  op_dat mat_free_factor, mat_free_gtau;
};

#endif