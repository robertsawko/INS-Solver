//
// auto-generated by op2.py
//

#ifdef _OPENMP
  #include <omp.h>
#endif

// global constants
extern int FMASK[15];

// header
#include "op_lib_cpp.h"

// user kernel files
#include "init_grid_kernel.cpp"
#include "set_ic1_kernel.cpp"
#include "set_ic2_kernel.cpp"
#include "div_kernel.cpp"
#include "curl_kernel.cpp"
#include "grad_kernel.cpp"
#include "pRHS_faces_kernel.cpp"
#include "pRHS_bc_kernel.cpp"
#include "pRHS_du_kernel.cpp"
