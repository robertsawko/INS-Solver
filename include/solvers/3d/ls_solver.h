#ifndef __INS_LS_3D_H
#define __INS_LS_3D_H

#include "dg_compiler_defs.h"

#include "op_seq.h"

#include "solvers/3d/advection_solver.h"

#include "dg_mesh/dg_mesh_3d.h"

#include "ls_utils/3d/kd_tree.h"

class LevelSetAdvectionSolver3D : public AdvectionSolver3D {
public:
  LevelSetAdvectionSolver3D(DGMesh3D *m);
  void set_bc_types(op_dat bc);

protected:
  virtual void bc_kernel(op_dat val, op_dat u, op_dat v, op_dat w, op_dat out) override;
  virtual void bc_kernel_oi(op_dat val, op_dat u, op_dat v, op_dat w, op_dat uM,
                            op_dat vM, op_dat wM, op_dat valM, op_dat valP) override;

  op_dat bc_types;
};

class LevelSetSolver3D {
public:
  LevelSetSolver3D(DGMesh3D *m);
  LevelSetSolver3D(DGMesh3D *m, const std::string &filename);
  ~LevelSetSolver3D();

  void init();

  void set_bc_types(op_dat bc);
  void step(op_dat u, op_dat v, op_dat w, const DG_FP dt, const int num_steps);
  void getRhoMu(op_dat rho, op_dat mu);
  void getNormalsCurvature(op_dat nx, op_dat ny, op_dat nz, op_dat curv);

  DGMesh3D *mesh;

  op_dat s, bc_types;

  DG_FP alpha, order_width;
private:
  void sampleInterface(op_dat sampleX, op_dat sampleY, op_dat sampleZ,
                       std::vector<PolyApprox3D> &polys, std::map<int,int> &cell2polyMap,
                       std::set<int> &cellInds);
  void reinitLS();
  // bool reinitNeeded();

  DG_FP h, epsilon, reinit_dt, reinit_width;
  int reinit_count;
  bool resuming;

  LevelSetAdvectionSolver3D *advectionSolver;
  KDTree3D *kdtree;
};

#endif
