//
// auto-generated by op2.py
//

// global constants
double gam_ompkernel;
double mu_ompkernel;
double bc_mach_ompkernel;
double bc_alpha_ompkernel;
double bc_p_ompkernel;
double bc_u_ompkernel;
double bc_v_ompkernel;
double ones_ompkernel[15];
int FMASK_ompkernel[15];

// header
#include "op_lib_cpp.h"

void op_decl_const_char(int dim, char const *type,
  int size, char *dat, char const *name){
  if(!strcmp(name, "gam")) {
    memcpy(&gam_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:gam_ompkernel)
  } else if(!strcmp(name, "mu")) {
    memcpy(&mu_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:mu_ompkernel)
  } else if(!strcmp(name, "bc_mach")) {
    memcpy(&bc_mach_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:bc_mach_ompkernel)
  } else if(!strcmp(name, "bc_alpha")) {
    memcpy(&bc_alpha_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:bc_alpha_ompkernel)
  } else if(!strcmp(name, "bc_p")) {
    memcpy(&bc_p_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:bc_p_ompkernel)
  } else if(!strcmp(name, "bc_u")) {
    memcpy(&bc_u_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:bc_u_ompkernel)
  } else if(!strcmp(name, "bc_v")) {
    memcpy(&bc_v_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:bc_v_ompkernel)
  } else if(!strcmp(name, "ones")) {
    memcpy(ones_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:ones_ompkernel[:15])
  } else if(!strcmp(name, "FMASK")) {
    memcpy(FMASK_ompkernel, dat, dim*size);
  #pragma omp target enter data map(to:FMASK_ompkernel[:15])
  }
}
// user kernel files
#include "init_grid_omp4kernel_func.cpp"
#include "set_ic_omp4kernel_func.cpp"
#include "div_omp4kernel_func.cpp"
#include "advection_flux_omp4kernel_func.cpp"
#include "advection_faces_omp4kernel_func.cpp"
#include "advection_bc_omp4kernel_func.cpp"
