#include "solvers/3d/ls_solver.h"

#include "op_seq.h"

#include "ls_utils/3d/kd_tree.h"
#ifdef INS_MPI
#include "ls_utils/3d/kd_tree_mpi.h"
#endif
#include "utils.h"
#include "dg_op2_blas.h"
#include "dg_utils.h"

#include <iostream>
#include <fstream>

LevelSetSolver3D::LevelSetSolver3D(DGMesh3D *m) {
  mesh = m;
  advectionSolver = new AdvectionSolver3D(m);

  double * dg_np_data = (double *)calloc(DG_NP * mesh->cells->size, sizeof(double));
  s = op_decl_dat(mesh->cells, DG_NP, "double", dg_np_data, "ls_solver_s");
  s_modal = op_decl_dat(mesh->cells, DG_NP, "double", dg_np_data, "ls_solver_s_modal");
  free(dg_np_data);

  double *ls_sample_np_data = (double *)calloc(LS_SAMPLE_NP * mesh->cells->size, sizeof(double));
  sampleX = op_decl_dat(mesh->cells, LS_SAMPLE_NP, "double", ls_sample_np_data, "ls_solver_sampleX");
  sampleY = op_decl_dat(mesh->cells, LS_SAMPLE_NP, "double", ls_sample_np_data, "ls_solver_sampleY");
  sampleZ = op_decl_dat(mesh->cells, LS_SAMPLE_NP, "double", ls_sample_np_data, "ls_solver_sampleZ");
  free(ls_sample_np_data);

  h = 0;
  reinit_count = 0;
}

LevelSetSolver3D::~LevelSetSolver3D() {
  delete advectionSolver;
}

void LevelSetSolver3D::setBCTypes(op_dat bc) {
  advectionSolver->set_bc_types(bc);
}

void LevelSetSolver3D::step(op_dat u, op_dat v, op_dat w, const double dt) {
  advectionSolver->set_dt(dt);
  advectionSolver->step(s, u, v, w);

  if(reinit_count > 49) {
    reinitLS();
    reinit_count = 0;
  }
  reinit_count++;
}

bool newtoncp_gepp(arma::mat &A, arma::vec &b) {
  for (int i = 0; i < 4; ++i) {
    int j = i;
    for (int k = i + 1; k < 4; ++k)
      if (std::abs(A(k,i)) > std::abs(A(j,i)))
        j = k;
    if (j != i) {
      for (int k = 0; k < 4; ++k)
        std::swap(A(i,k), A(j,k));
      std::swap(b(i), b(j));
    }

    if (std::abs(A(i,i)) < 1.0e4*std::numeric_limits<double>::epsilon())
      return false;

    double fac = 1.0 / A(i,i);
    for (int j = i + 1; j < 4; ++j)
      A(j,i) *= fac;

    for (int j = i + 1; j < 4; ++j) {
      for (int k = i + 1; k < 4; ++k)
        A(j,k) -= A(j,i)*A(i,k);
      b(j) -= A(j,i)*b(i);
    }
  }

  for (int i = 4 - 1; i >= 0; --i) {
    double sum = 0.0;
    for (int j = i + 1; j < 4; ++j)
      sum += A(i,j)*b(j);
    b(i) = (b(i) - sum) / A(i,i);
  }

  return true;
}

bool newton_kernel(double &closest_pt_x, double &closest_pt_y, double &closest_pt_z,
                   const double node_x, const double node_y, const double node_z,
                   PolyApprox3D &p, const double h) {
  double lambda = 0.0;
  bool converged = false;
  double pt_x = closest_pt_x;
  double pt_y = closest_pt_y;
  double pt_z = closest_pt_z;
  double init_x = closest_pt_x;
  double init_y = closest_pt_y;
  double init_z = closest_pt_z;

  for(int step = 0; step < 100; step++) {
    double pt_x_old = pt_x;
    double pt_y_old = pt_y;
    double pt_z_old = pt_z;
    // Evaluate surface and gradient at current guess
    double surface = p.val_at(pt_x, pt_y, pt_z);
    double surface_dx, surface_dy, surface_dz;
    p.grad_at(pt_x, pt_y, pt_z, surface_dx, surface_dy, surface_dz);
    // Evaluate Hessian
    double hessian[6];
    p.hessian_at(pt_x, pt_y, pt_z, hessian[0], hessian[1], hessian[2],
                 hessian[3], hessian[4], hessian[5]);

    // Check if |nabla(surface)| = 0, if so then return
    double gradsqrnorm = surface_dx * surface_dx + surface_dy * surface_dy + surface_dz * surface_dz;
    if(gradsqrnorm < 1e-14)
      break;

    // Init lambda at first step
    if(step == 0)
      lambda = ((node_x - pt_x) * surface_dx + (node_y - pt_y) * surface_dy + (node_z - pt_z) * surface_dz) / gradsqrnorm;

    // Gradient of functional
    arma::vec gradf(4);
    gradf(0) = pt_x - node_x + lambda * surface_dx;
    gradf(1) = pt_y - node_y + lambda * surface_dy;
    gradf(2) = pt_z - node_z + lambda * surface_dz;
    gradf(3) = surface;

    // Calculate Hessian of functional
    arma::mat hessianf(4, 4);
    hessianf(0, 0) = 1.0 + lambda * hessian[0];
    hessianf(0, 1) = lambda * hessian[3]; hessianf(1, 0) = hessianf(0, 1);
    hessianf(0, 2) = lambda * hessian[4]; hessianf(2, 0) = hessianf(0, 2);
    hessianf(0, 3) = surface_dx; hessianf(3, 0) = hessianf(0, 3);

    hessianf(1, 1) = 1.0 + lambda * hessian[1];
    hessianf(1, 2) = lambda * hessian[5]; hessianf(2, 1) = hessianf(1, 2);
    hessianf(1, 3) = surface_dy; hessianf(3, 1) = hessianf(1, 3);

    hessianf(2, 2) = 1.0 + lambda * hessian[2];
    hessianf(2, 3) = surface_dz; hessianf(3, 2) = hessianf(2, 3);

    hessianf(3, 3) = 0.0;

    if(!newtoncp_gepp(hessianf, gradf)) {
      double delta1_x = (surface / gradsqrnorm) * surface_dx;
      double delta1_y = (surface / gradsqrnorm) * surface_dy;
      double delta1_z = (surface / gradsqrnorm) * surface_dz;
      lambda = ((node_x - pt_x) * surface_dx + (node_y - pt_y) * surface_dy + (node_z - pt_z) * surface_dz) / gradsqrnorm;
      double delta2_x = pt_x - node_x + lambda * surface_dx;
      double delta2_y = pt_y - node_y + lambda * surface_dy;
      double delta2_z = pt_z - node_z + lambda * surface_dz;
      double msqr = delta2_x * delta2_x + delta2_y * delta2_y + delta2_z * delta2_z;
      if(msqr > 0.1 * h * 0.1 * h) {
        delta2_x *= 0.1 * h / sqrt(msqr);
        delta2_y *= 0.1 * h / sqrt(msqr);
        delta2_z *= 0.1 * h / sqrt(msqr);
      }
      pt_x -= delta1_x + delta2_x;
      pt_y -= delta1_y + delta2_y;
      pt_z -= delta1_z + delta2_z;
    } else {
      arma::vec ans = gradf;

      // Clamp update
      double msqr = ans(0) * ans(0) + ans(1) * ans(1) + ans(2) * ans(2);
      if(msqr > h * 0.5 * h * 0.5)
        ans = ans * 0.5 * h / sqrt(msqr);

      // Update guess
      pt_x -= ans(0);
      pt_y -= ans(1);
      pt_z -= ans(2);
      lambda -= ans(3);
    }

    // Gone outside the element, return
    // if((init_x - pt_x) * (init_x - pt_x) + (init_y - pt_y) * (init_y - pt_y) > h * h) {
    //   pt_x = pt_x_old;
    //   pt_y = pt_y_old;
    //   pt_z = pt_z_old;
    //   break;
    // }

    // Converged, no more steps required
    if(sqrt((pt_x_old - pt_x) * (pt_x_old - pt_x) + (pt_y_old - pt_y) * (pt_y_old - pt_y) + (pt_z_old - pt_z) * (pt_z_old - pt_z)) < 1e-12) {
      converged = true;
      break;
    }
  }

  closest_pt_x = pt_x;
  closest_pt_y = pt_y;
  closest_pt_z = pt_z;

  return converged;
}

void newton_method(const int numPts, double *closest_x, double *closest_y, double *closest_z,
                   const double *x, const double *y, const double *z, int *poly_ind,
                   std::vector<PolyApprox3D> &polys, double *s, const double h) {
  int numNonConv = 0;
  int numReinit = 0;

  #pragma omp parallel for
  for(int i = 0; i < numPts; i++) {
    int start_ind = (i / DG_NP) * DG_NP;
    bool reinit = false;
    for(int j = 0; j < DG_NP; j++) {
      if(fabs(s[start_ind + j]) < 0.02) {
        reinit = true;
      }
    }

    if(reinit) {
      bool tmp = newton_kernel(closest_x[i], closest_y[i], closest_z[i], x[i], y[i], z[i], polys[poly_ind[i]], h);
      if(tmp) {
        bool negative = s[i] < 0.0;
        s[i] = (closest_x[i] - x[i]) * (closest_x[i] - x[i]) + (closest_y[i] - y[i]) * (closest_y[i] - y[i]) + (closest_z[i] - z[i]) * (closest_z[i] - z[i]);
        s[i] = sqrt(s[i]);
        if(negative) s[i] *= -1.0;
      }
      if(!tmp) {
        #pragma omp atomic
        numNonConv++;
      }
      #pragma omp atomic
      numReinit++;
    }
  }

  if(numNonConv != 0)
    std::cout << numNonConv << " non-converged points out of " << numReinit << " points reinitialised" << std::endl;
}

void LevelSetSolver3D::reinitLS() {
  sampleInterface();

  const double *sample_pts_x = getOP2PtrHost(sampleX, OP_READ);
  const double *sample_pts_y = getOP2PtrHost(sampleY, OP_READ);
  const double *sample_pts_z = getOP2PtrHost(sampleZ, OP_READ);

  #ifndef INS_MPI
  /*
  std::ofstream pts_file("pts.csv");
  pts_file << "x,y,z" << std::endl;
  for(int i = 0; i < mesh->cells->size * LS_SAMPLE_NP; i++) {
    if(!isnan(sample_pts_x[i])) {
      pts_file << sample_pts_x[i] << "," << sample_pts_y[i] << "," << sample_pts_z[i] << std::endl;
    }
  }
  pts_file.close();
  */
  #endif

  #ifdef INS_MPI
  KDTree3DMPI kdtree(sample_pts_x, sample_pts_y, sample_pts_z, LS_SAMPLE_NP * mesh->cells->size, mesh, s);
  kdtree.build_tree();
  #else
  KDTree3D kdtree(sample_pts_x, sample_pts_y, sample_pts_z, LS_SAMPLE_NP * mesh->cells->size, mesh, s);
  kdtree.build_tree();
  #endif

  releaseOP2PtrHost(sampleX, OP_READ, sample_pts_x);
  releaseOP2PtrHost(sampleY, OP_READ, sample_pts_y);
  releaseOP2PtrHost(sampleZ, OP_READ, sample_pts_z);

  const double *x_ptr = getOP2PtrHost(mesh->x, OP_READ);
  const double *y_ptr = getOP2PtrHost(mesh->y, OP_READ);
  const double *z_ptr = getOP2PtrHost(mesh->z, OP_READ);

  double *closest_x = (double *)calloc(DG_NP * mesh->cells->size, sizeof(double));
  double *closest_y = (double *)calloc(DG_NP * mesh->cells->size, sizeof(double));
  double *closest_z = (double *)calloc(DG_NP * mesh->cells->size, sizeof(double));
  int *poly_ind     = (int *)calloc(DG_NP * mesh->cells->size, sizeof(int));
  std::vector<PolyApprox3D> polys;

  if(!kdtree.empty) {
    #pragma omp parallel for
    for(int i = 0; i < DG_NP * mesh->cells->size; i++) {
      // Get closest sample point
      KDCoord tmp = kdtree.closest_point(x_ptr[i], y_ptr[i], z_ptr[i]);
      closest_x[i] = tmp.x;
      closest_y[i] = tmp.y;
      closest_z[i] = tmp.z;
      poly_ind[i]  = tmp.poly;
    }

    polys = kdtree.get_polys();
  }

  double *surface_ptr = getOP2PtrHost(s, OP_RW);

  if(h == 0.0) {
    op_par_loop(calc_h_3d, "calc_h_3d", mesh->faces,
                op_arg_dat(mesh->fscale, -1, OP_ID, 2, "double", OP_READ),
                op_arg_gbl(&h, 1, "double", OP_MAX));
    h = 1.0 / h;
  }

  // Newton method
  if(!kdtree.empty) {
    newton_method(DG_NP * mesh->cells->size, closest_x, closest_y, closest_z,
                  x_ptr, y_ptr, z_ptr, poly_ind, polys, surface_ptr, h);
  }
  releaseOP2PtrHost(s, OP_RW, surface_ptr);

  free(closest_x);
  free(closest_y);
  free(closest_z);
  free(poly_ind);

  releaseOP2PtrHost(mesh->x, OP_READ, x_ptr);
  releaseOP2PtrHost(mesh->y, OP_READ, y_ptr);
  releaseOP2PtrHost(mesh->z, OP_READ, z_ptr);
}