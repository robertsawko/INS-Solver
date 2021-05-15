#include "poisson.h"

#include <iostream>
#include <unistd.h>

#include "op_seq.h"

#include "kernels/poisson_mf2_op.h"
#include "kernels/poisson_mf2_opf.h"
#include "kernels/poisson_mf2_opbf.h"
#include "kernels/glb_ind_kernel.h"
#include "kernels/glb_ind_kernelBC.h"

using namespace std;

Poisson_M::Poisson_M(INSData *data, CubatureData *cubData, GaussData *gaussData) : Poisson(data, cubData, gaussData) {
  glb_ind_data   = (int *)calloc(data->numCells, sizeof(int));
  glb_indL_data  = (int *)calloc(data->numEdges, sizeof(int));
  glb_indR_data  = (int *)calloc(data->numEdges, sizeof(int));
  glb_indBC_data = (int *)calloc(data->numBoundaryEdges, sizeof(int));
  op1_data       = (double *)calloc(15 * 15 * data->numCells, sizeof(double));
  op2_data[0]    = (double *)calloc(15 * 15 * data->numEdges, sizeof(double));
  op2_data[1]    = (double *)calloc(15 * 15 * data->numEdges, sizeof(double));
  op_bc_data     = (double *)calloc(7 * 15 * data->numBoundaryEdges, sizeof(double));

  glb_ind   = op_decl_dat(data->cells, 1, "int", glb_ind_data, "poisson_glb_ind");
  glb_indL  = op_decl_dat(data->edges, 1, "int", glb_indL_data, "poisson_glb_indL");
  glb_indR  = op_decl_dat(data->edges, 1, "int", glb_indR_data, "poisson_glb_indR");
  glb_indBC = op_decl_dat(data->bedges, 1, "int", glb_indBC_data, "poisson_glb_indBC");
  op1       = op_decl_dat(data->cells, 15 * 15, "double", op1_data, "poisson_op1");
  op2[0]    = op_decl_dat(data->edges, 15 * 15, "double", op2_data[0], "poisson_op20");
  op2[1]    = op_decl_dat(data->edges, 15 * 15, "double", op2_data[1], "poisson_op21");
  op_bc     = op_decl_dat(data->bedges, 7 * 15, "double", op_bc_data, "poisson_op_bc");
}

Poisson_M::~Poisson_M() {
  if(pMatInit)
    MatDestroy(&pMat);
  if(pMMatInit)
    MatDestroy(&pMMat);
  if(pBCMatInit)
    MatDestroy(&pBCMat);

  destroy_vec(&b);
  destroy_vec(&bc);
  destroy_vec(&rhs);
  destroy_vec(&x);

  MatDestroy(&op);
  KSPDestroy(&ksp);

  free(glb_ind_data);
  free(glb_indL_data);
  free(glb_indR_data);
  free(glb_indBC_data);
  free(op1_data);
  free(op2_data[0]);
  free(op2_data[1]);
}

void Poisson_M::init() {
  setGlbInd();
  op_par_loop(glb_ind_kernel, "glb_ind_kernel", data->edges,
              op_arg_dat(glb_ind, -2, data->edge2cells, 1, "int", OP_READ),
              op_arg_dat(glb_indL, -1, OP_ID, 1, "int", OP_WRITE),
              op_arg_dat(glb_indR, -1, OP_ID, 1, "int", OP_WRITE));
  op_par_loop(glb_ind_kernelBC, "glb_ind_kernelBC", data->bedges,
              op_arg_dat(glb_ind, 0, data->bedge2cells, 1, "int", OP_READ),
              op_arg_dat(glb_indBC, -1, OP_ID, 1, "int", OP_WRITE));
  setOp();
  setBCOP();
  createMatrix();
  createMassMatrix();
  createBCMatrix();

  create_vec(&b);
  create_vec(&bc, 21);
  create_vec(&rhs);
  create_vec(&x);

  create_mat(&op, 15 * data->cells->size, 15 * data->cells->size, 15 * 4);
  MatAssemblyBegin(op, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(op, MAT_FINAL_ASSEMBLY);

  KSPCreate(PETSC_COMM_WORLD, &ksp);
  KSPSetType(ksp, KSPCG);
  // KSPSetType(ksp, KSPGMRES);
  PC pc;
  KSPGetPC(ksp, &pc);
  // PCSetType(pc, PCICC);
  // PCSetType(pc, PCJACOBI);
  PCSetType(pc, PCNONE);
  KSPSetTolerances(ksp, 1e-10, 1e-50, 1e5, 1e4);
}

bool Poisson_M::solve(op_dat b_dat, op_dat x_dat, bool addMass, double factor) {
  massMat = addMass;
  massFactor = factor;

  // Calculate RHS for linear solve by applying the BCs
  load_vec(&b, b_dat);
  load_vec(&bc, bc_dat, 21);
  MatMultAdd(pBCMat, bc, b, rhs);

  // Create matrix for linear solve, adding mass matrix scaled by a factor if required
  if(addMass) {
    MatCopy(pMat, op, DIFFERENT_NONZERO_PATTERN);
    MatAXPY(op, factor, pMMat, DIFFERENT_NONZERO_PATTERN);
  } else {
    MatCopy(pMat, op, DIFFERENT_NONZERO_PATTERN);
  }

  KSPSetOperators(ksp, op, op);

  // Solve
  KSPSolve(ksp, rhs, x);

  int numIt;
  KSPGetIterationNumber(ksp, &numIt);
  KSPConvergedReason reason;
  KSPGetConvergedReason(ksp, &reason);
  // Check that the solver converged
  bool converged = true;
  if(reason < 0) {
    double residual;
    KSPGetResidualNorm(ksp, &residual);
    converged = false;
    cout << "Number of iterations for linear solver: " << numIt << endl;
    cout << "Converged reason: " << reason << " Residual: " << residual << endl;
  }
  numberIter += numIt;
  solveCount++;

  // Get solution and free PETSc vectors and matrix
  Vec solution;
  KSPGetSolution(ksp, &solution);
  store_vec(&solution, x_dat);

  return converged;
}

/*
void Poisson_M::createBCMatrix() {
  create_mat(&pBCMat, 15 * data->cells->size, 21 * data->cells->size, 15);
  pBCMatInit = true;
  double tol = 1e-15;

  double *gauss_sJ  = (double *)malloc(21 * data->cells->size * sizeof(double));
  double *gauss_tau = (double *)malloc(3 * data->cells->size * sizeof(double));
  double *gauss_mD[3];
  for(int i = 0; i < 3; i++) {
    gauss_mD[i]  = (double *)malloc(7 * 15 * data->cells->size * sizeof(double));
    op_fetch_data(gData->mD[i], gauss_mD[i]);
  }
  op_fetch_data(gData->sJ, gauss_sJ);
  op_fetch_data(gData->tau, gauss_tau);

  // Create BCs matrix using Gauss data on boundary edges
  for(int i = 0; i < data->bedges->size; i++) {
    int element = data->bedge2cell_data[i];
    int bedgeType = data->bedge_type_data[i];
    int edge = data->bedgeNum_data[i];
    if(dirichlet[0] == bedgeType || dirichlet[1] == bedgeType || dirichlet[2] == bedgeType) {
      // Get data
      for(int j = 0; j < 7 * 15; j++) {
        int indT = (j % 7) * 15 + (j / 7);
        int col = element * 21 + edge * 7 + (j % 7);
        int row = element * 15 + (j / 7);
        double val;
        if(edge == 0) {
          val = gFInterp0_g[indT] * gaussW_g[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)] * gauss_tau[element * 3 + edge];
        } else if(edge == 1) {
          val = gFInterp1_g[indT] * gaussW_g[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)] * gauss_tau[element * 3 + edge];
        } else {
          val = gFInterp2_g[indT] * gaussW_g[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)] * gauss_tau[element * 3 + edge];
        }
        val -= gauss_mD[edge][element * 7 * 15 + indT] * gaussW_g[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
        if(abs(val) > tol)
          MatSetValues(pBCMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    } else if(neumann[0] == bedgeType || neumann[1] == bedgeType || neumann[2] == bedgeType) {
      // Get data
      for(int j = 0; j < 7 * 15; j++) {
        int indT = (j % 7) * 15 + (j / 7);
        int col = element * 21 + edge * 7 + (j % 7);
        int row = element * 15 + (j / 7);
        double val;
        if(edge == 0) {
          val = gFInterp0_g[indT] * gaussW_g[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
        } else if(edge == 1) {
          val = gFInterp1_g[indT] * gaussW_g[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
        } else {
          val = gFInterp2_g[indT] * gaussW_g[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
        }
        if(abs(val) > tol)
          MatSetValues(pBCMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    } else {
      cout << "UNDEFINED BOUNDARY EDGE" << endl;
      cout << "Element " << element << " Edge " << edge << " Type " << bedgeType << endl;
      cout << "D: " << dirichlet[0] << " " << dirichlet[1] << endl;
      cout << "N: " << neumann[0] << " " << neumann[1] << endl;
    }
  }

  free(gauss_sJ);
  free(gauss_tau);
  for(int i = 0; i < 3; i++) {
    free(gauss_mD[i]);
  }

  MatAssemblyBegin(pBCMat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(pBCMat, MAT_FINAL_ASSEMBLY);
}
*/

void Poisson_M::setOp() {
  double tol = 1e-15;

  op_par_loop(poisson_mf2_op, "poisson_mf2_op", data->cells,
              op_arg_dat(cData->OP, -1, OP_ID, 15 * 15, "double", OP_READ),
              op_arg_gbl(&tol, 1, "double", OP_READ),
              op_arg_dat(op1, -1, OP_ID, 15 * 15, "double", OP_WRITE));

  op_par_loop(poisson_mf2_opf, "poisson_mf2_opf", data->edges,
              op_arg_gbl(&tol, 1, "double", OP_READ),
              op_arg_dat(data->edgeNum, -1, OP_ID, 2, "int", OP_READ),
              op_arg_dat(gData->OP[0], 0, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OP[1], 0, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OP[2], 0, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OPf[0], 0, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OPf[1], 0, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OPf[2], 0, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(op2[0], -1, OP_ID, 15 * 15, "double", OP_INC),
              op_arg_dat(op1, 0, data->edge2cells, 15 * 15, "double", OP_INC),
              op_arg_dat(gData->OP[0], 1, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OP[1], 1, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OP[2], 1, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OPf[0], 1, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OPf[1], 1, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OPf[2], 1, data->edge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(op2[1], -1, OP_ID, 15 * 15, "double", OP_INC),
              op_arg_dat(op1, 1, data->edge2cells, 15 * 15, "double", OP_INC));

  op_par_loop(poisson_mf2_opbf, "poisson_mf2_opbf", data->bedges,
              op_arg_gbl(&tol, 1, "double", OP_READ),
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum, -1, OP_ID, 1, "int", OP_READ),
              op_arg_gbl(&dirichlet[0], 1, "int", OP_READ),
              op_arg_gbl(&dirichlet[1], 1, "int", OP_READ),
              op_arg_gbl(&dirichlet[2], 1, "int", OP_READ),
              op_arg_dat(gData->OP[0], 0, data->bedge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OP[1], 0, data->bedge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(gData->OP[2], 0, data->bedge2cells, 15 * 15, "double", OP_READ),
              op_arg_dat(op1, 0, data->bedge2cells, 15 * 15, "double", OP_INC));
}

void Poisson_M::setBCOP() {
  double tol = 1e-15;
  // If not dirichlet BC, kernel will assume it is a neumann bc
  op_par_loop(poisson_mf2_bc, "poisson_mf2_bc", data->bedges,
              op_arg_gbl(&tol, 1, "double", OP_READ),
              op_arg_dat(data->bedge_type, -1, OP_ID, 1, "int", OP_READ),
              op_arg_dat(data->bedgeNum,   -1, OP_ID, 1, "int", OP_READ),
              op_arg_gbl(&dirichlet[0], 1, "int", OP_READ),
              op_arg_gbl(&dirichlet[1], 1, "int", OP_READ),
              op_arg_gbl(&dirichlet[2], 1, "int", OP_READ),
              op_arg_dat(gData->mD[0], 0, data->bedge2cells, 7 * 15, "double", OP_READ),
              op_arg_dat(gData->mD[1], 0, data->bedge2cells, 7 * 15, "double", OP_READ),
              op_arg_dat(gData->mD[2], 0, data->bedge2cells, 7 * 15, "double", OP_READ),
              op_arg_dat(gData->sJ, 0, data->bedge2cells, 21, "double", OP_READ),
              op_arg_dat(gData->tau, 0, data->bedge2cells, 3, "double", OP_READ),
              op_arg_dat(op_bc, -1, OP_ID, 7 * 15, "double", OP_INC));
}
