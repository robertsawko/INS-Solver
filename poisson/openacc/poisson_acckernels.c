//
// auto-generated by op2.py
//

// global constants
extern int FMASK[15];

// header
#include "op_lib_c.h"

void op_decl_const_char(int dim, char const *type,
int size, char *dat, char const *name){}
// user kernel files
#include "init_grid_acckernel.c"
#include "set_ic1_acckernel.c"
#include "set_ic2_acckernel.c"
#include "div_acckernel.c"
#include "curl_acckernel.c"
#include "grad_acckernel.c"
#include "pRHS_faces_acckernel.c"
#include "pRHS_bc_acckernel.c"
#include "pRHS_du_acckernel.c"