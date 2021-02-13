//
// auto-generated by op2.py
//

// global constants
int FMASK_ompkernel[15];

// header
#include "op_lib_cpp.h"

void op_decl_const_char(int dim, char const *type,
  int size, char *dat, char const *name){
  if(!strcmp(name, "FMASK")) {
    memcpy(FMASK_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:FMASK_ompkernel[:15])
  }
}
// user kernel files
#include "init_grid_omp4kernel_func.cpp"
#include "set_ic1_omp4kernel_func.cpp"
#include "set_ic2_omp4kernel_func.cpp"
#include "div_omp4kernel_func.cpp"
#include "curl_omp4kernel_func.cpp"
#include "grad_omp4kernel_func.cpp"
#include "pRHS_faces_omp4kernel_func.cpp"
#include "pRHS_bc_omp4kernel_func.cpp"
#include "pRHS_du_omp4kernel_func.cpp"