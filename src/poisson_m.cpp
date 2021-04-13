#include "poisson.h"

#include <iostream>
#include <unistd.h>

#include "op_seq.h"

using namespace std;

Poisson_M::~Poisson_M() {
  if(pMatInit)
    MatDestroy(&pMat);
  if(pMMatInit)
    MatDestroy(&pMMat);
  if(pBCMatInit)
    MatDestroy(&pBCMat);
}

bool Poisson_M::solve(op_dat b_dat, op_dat x_dat, bool addMass, double factor) {
  massMat = addMass;
  massFactor = factor;
  Vec b;
  create_vec(&b);
  load_vec(&b, b_dat);

  Vec bc;
  create_vec(&bc, 21);
  load_vec(&bc, bc_dat, 21);

  // Calculate RHS for linear solve by applying the BCs
  Vec rhs;
  create_vec(&rhs);
  MatMultAdd(pBCMat, bc, b, rhs);

  Vec x;
  create_vec(&x);

  // Create PETSc Preconditioned Conjugate Gradient linear solver
  KSP ksp;
  KSPCreate(PETSC_COMM_SELF, &ksp);
  KSPSetType(ksp, KSPCG);
  // Set preconditioner to Incomplete Cholesky
  PC pc;
  KSPGetPC(ksp, &pc);
  PCSetType(pc, PCICC);

  // Create matrix for linear solve, adding mass matrix scaled by a factor if required
  Mat op;
  create_mat(&op, 15 * data->numCells, 15 * data->numCells, 15 * 4);
  MatAssemblyBegin(op, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(op, MAT_FINAL_ASSEMBLY);
  if(addMass) {
    MatCopy(pMat, op, DIFFERENT_NONZERO_PATTERN);
    MatAXPY(op, factor, pMMat, DIFFERENT_NONZERO_PATTERN);
  } else {
    MatCopy(pMat, op, DIFFERENT_NONZERO_PATTERN);
  }

  KSPSetOperators(ksp, op, op);
  KSPSetTolerances(ksp, 1e-10, 1e-50, 1e5, 1e4);
  // Solve
  KSPSolve(ksp, rhs, x);
  int numIt;
  KSPGetIterationNumber(ksp, &numIt);
  KSPConvergedReason reason;
  KSPGetConvergedReason(ksp, &reason);
  double residual;
  KSPGetResidualNorm(ksp, &residual);
  // Check that the solver converged
  bool converged = true;
  if(reason < 0) {
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
  KSPDestroy(&ksp);
  destroy_vec(&b);
  destroy_vec(&x);
  MatDestroy(&op);

  return converged;
}

void Poisson_M::createMatrix() {
  create_mat(&pMat, 15 * data->numCells, 15 * data->numCells, 15 * 4);
  pMatInit = true;
  double tol = 1e-15;

  // Add cubature OP to Poisson matrix
  double *cub_OP = (double *)malloc(15 * 15 * op_get_size(data->cells) * sizeof(double));
  op_fetch_data(cData->OP, cub_OP);
  for(int i = 0; i < data->numCells; i++) {
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = i * 15 + m;
        int col = i * 15 + n;
        int colInd = n * 15 + m;
        double val = cub_OP[i * 15 * 15 + colInd];
        if(abs(val) > tol)
          MatSetValues(pMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    }
  }
  free(cub_OP);

  double *gauss_OP[3];
  double *gauss_OPf[3];
  for(int i = 0; i < 3; i++) {
    gauss_OP[i] = (double *)malloc(15 * 15 * op_get_size(data->cells) * sizeof(double));
    gauss_OPf[i] = (double *)malloc(15 * 15 * op_get_size(data->cells) * sizeof(double));
    op_fetch_data(gData->OP[i], gauss_OP[i]);
    op_fetch_data(gData->OPf[i], gauss_OPf[i]);
  }

  // Add Gauss OP and OPf to Poisson matrix
  for(int i = 0; i < data->numEdges; i++) {
    int leftElement = data->edge2cell_data[i * 2];
    int rightElement = data->edge2cell_data[i * 2 + 1];
    int leftEdge = data->edgeNum_data[i * 2];
    int rightEdge = data->edgeNum_data[i * 2 + 1];
    // Gauss OP
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = leftElement * 15 + m;
        int col = leftElement * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gauss_OP[leftEdge][leftElement * 15 * 15 + colInd];
        if(abs(val) > tol)
          MatSetValues(pMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    }
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = rightElement * 15 + m;
        int col = rightElement * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gauss_OP[rightEdge][rightElement * 15 * 15 + colInd];
        if(abs(val) > tol)
          MatSetValues(pMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    }

    // Gauss OPf
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = leftElement * 15 + m;
        int col = rightElement * 15 + n;
        int colInd = n * 15 + m;
        double val = -0.5 * gauss_OPf[leftEdge][leftElement * 15 * 15 + colInd];
        if(abs(val) > tol)
          MatSetValues(pMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    }
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = rightElement * 15 + m;
        int col = leftElement * 15 + n;
        int colInd = n * 15 + m;
        double val = -0.5 * gauss_OPf[rightEdge][rightElement * 15 * 15 + colInd];
        if(abs(val) > tol)
          MatSetValues(pMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    }
  }

  // Add Gauss OP for boundary edges
  for(int i = 0; i < data->numBoundaryEdges; i++) {
    int element = data->bedge2cell_data[i];
    int bedgeType = data->bedge_type_data[i];
    int edge = data->bedgeNum_data[i];
    if(dirichlet[0] == bedgeType || dirichlet[1] == bedgeType || dirichlet[2] == bedgeType) {
      // Convert data to row major format
      for(int m = 0; m < 15; m++) {
        for(int n = 0; n < 15; n++) {
          int row = element * 15 + m;
          int col = element * 15 + n;
          int colInd = n * 15 + m;
          double val = gauss_OP[edge][element * 15 * 15 + colInd];
          if(abs(val) > tol)
            MatSetValues(pMat, 1, &row, 1, &col, &val, ADD_VALUES);
        }
      }
    }
  }

  for(int i = 0; i < 3; i++) {
    free(gauss_OP[i]);
    free(gauss_OPf[i]);
  }

  MatAssemblyBegin(pMat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(pMat, MAT_FINAL_ASSEMBLY);
  // PetscViewer pv;
  // PetscViewerDrawOpen(PETSC_COMM_SELF, NULL, NULL, PETSC_DECIDE, PETSC_DECIDE, 500, 500, &pv);
  // MatView(pMat, pv);
}

void Poisson_M::createMassMatrix() {
  create_mat(&pMMat, 15 * data->numCells, 15 * data->numCells, 15);
  pMMatInit = true;
  // Add Cubature OP to mass matrix
  double *cub_MM = (double *)malloc(15 * 15 * op_get_size(data->cells) * sizeof(double));
  op_fetch_data(cData->mm, cub_MM);
  for(int i = 0; i < data->numCells; i++) {
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = i * 15 + m;
        int col = i * 15 + n;
        int colInd = n * 15 + m;
        double val = cub_MM[i * 15 * 15 + colInd];
        MatSetValues(pMMat, 1, &row, 1, &col, &val, ADD_VALUES);
      }
    }
  }
  free(cub_MM);

  MatAssemblyBegin(pMMat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(pMMat, MAT_FINAL_ASSEMBLY);
}

void Poisson_M::createBCMatrix() {
  create_mat(&pBCMat, 15 * data->numCells, 21 * data->numCells, 15);
  pBCMatInit = true;
  double tol = 1e-15;

  double *gauss_sJ  = (double *)malloc(21 * op_get_size(data->cells) * sizeof(double));
  double *gauss_tau = (double *)malloc(3 * op_get_size(data->cells) * sizeof(double));
  double *gauss_mD[3];
  for(int i = 0; i < 3; i++) {
    gauss_mD[i]  = (double *)malloc(7 * 15 * op_get_size(data->cells) * sizeof(double));
    op_fetch_data(gData->mD[i], gauss_mD[i]);
  }
  op_fetch_data(gData->sJ, gauss_sJ);
  op_fetch_data(gData->tau, gauss_tau);

  // Create BCs matrix using Gauss data on boundary edges
  for(int i = 0; i < data->numBoundaryEdges; i++) {
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
          val = gFInterp0[indT] * gaussW[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)] * gauss_tau[element * 3 + edge];
        } else if(edge == 1) {
          val = gFInterp1[indT] * gaussW[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)] * gauss_tau[element * 3 + edge];
        } else {
          val = gFInterp2[indT] * gaussW[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)] * gauss_tau[element * 3 + edge];
        }
        val -= gauss_mD[edge][element * 7 * 15 + indT] * gaussW[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
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
          val = gFInterp0[indT] * gaussW[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
        } else if(edge == 1) {
          val = gFInterp1[indT] * gaussW[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
        } else {
          val = gFInterp2[indT] * gaussW[j % 7] * gauss_sJ[element * 21 + edge * 7 + (j % 7)];
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
