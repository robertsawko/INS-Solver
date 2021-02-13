//
// auto-generated by op2.py
//

#include "poisson_rhs.h"

#include  "op_lib_cpp.h"

//
// op_par_loop declarations
//
#ifdef OPENACC
#ifdef __cplusplus
extern "C" {
#endif
#endif

void op_par_loop_pRHS_faces(char const *, op_set,
  op_arg,
  op_arg,
  op_arg,
  op_arg,
  op_arg );

void op_par_loop_pRHS_bc(char const *, op_set,
  op_arg,
  op_arg,
  op_arg,
  op_arg );

void op_par_loop_pRHS_du(char const *, op_set,
  op_arg,
  op_arg,
  op_arg );
#ifdef OPENACC
#ifdef __cplusplus
}
#endif
#endif


#include "kernels/pRHS_faces.h"
#include "kernels/pRHS_bc.h"
#include "kernels/pRHS_du.h"

void poisson_rhs(const double *u, double *rhs) {
  op_arg u_copy_args[] = {
    op_arg_dat(data->pU, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(data->cells, 1, u_copy_args);
  cudaMemcpy(data->pU->data_d, u, data->numCells * 15 * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, u_copy_args);

  op_par_loop_pRHS_faces("pRHS_faces",data->edges,
              op_arg_dat(data->edgeNum,-1,OP_ID,2,"int",OP_READ),
              op_arg_dat(data->nodeX,-2,data->edge2cells,3,"double",OP_READ),
              op_arg_dat(data->nodeY,-2,data->edge2cells,3,"double",OP_READ),
              op_arg_dat(data->pU,-2,data->edge2cells,15,"double",OP_READ),
              op_arg_dat(data->pExU,-2,data->edge2cells,15,"double",OP_INC));

  op_par_loop_pRHS_bc("pRHS_bc",data->bedges,
              op_arg_dat(data->bedge_type,-1,OP_ID,1,"int",OP_READ),
              op_arg_dat(data->bedgeNum,-1,OP_ID,1,"int",OP_READ),
              op_arg_dat(data->pU,0,data->bedge2cells,15,"double",OP_READ),
              op_arg_dat(data->pExU,0,data->bedge2cells,15,"double",OP_INC));

  op_par_loop_pRHS_du("pRHS_du",data->cells,
              op_arg_dat(data->pU,-1,OP_ID,15,"double",OP_READ),
              op_arg_dat(data->pExU,-1,OP_ID,15,"double",OP_READ),
              op_arg_dat(data->pDu,-1,OP_ID,15,"double",OP_WRITE));
}
