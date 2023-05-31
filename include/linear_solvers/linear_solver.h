#ifndef __LINEAR_SOLVER_H
#define __LINEAR_SOLVER_H

#include "dg_compiler_defs.h"

#include "op_seq.h"
#include "matrices/poisson_matrix.h"

class LinearSolver {
public:
  virtual void set_matrix(PoissonMatrix *mat);
  void set_bcs(op_dat bcs);
  void set_nullspace(bool ns);
  virtual bool solve(op_dat rhs, op_dat ans) = 0;
  virtual void init();
  virtual void set_tol(const DG_FP r_tol, const DG_FP a_tol);

protected:
  PoissonMatrix *matrix;
  bool nullspace;
  op_dat bc;
};

#endif
