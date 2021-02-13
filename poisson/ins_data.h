#ifndef __NS_DATA_H
#define __NS_DATA_H

#include "op_seq.h"

class INSData {
public:
  INSData();
  ~INSData();
  void initOP2();
  // Pointers used when loading data
  double *coords;
  int *cgnsCells;
  int *edge2node_data;
  int *edge2cell_data;
  int *bedge2node_data;
  int *bedge2cell_data;
  int *bedge_type_data;
  int *edgeNum_data;
  int *bedgeNum_data;
  int numNodes, numCells, numEdges, numBoundaryEdges;
  // OP2 stuff
  op_set nodes, cells, edges, bedges;
  op_map cell2nodes, edge2nodes, edge2cells, bedge2nodes, bedge2cells;
  op_dat node_coords, nodeX, nodeY, x, y, xr, yr, xs, ys, rx, ry, sx, sy, nx,
         ny, J, sJ, fscale, bedge_type, edgeNum, bedgeNum;
  op_dat uD, qN, Aqbc, rhs, poissonBC[4], pRHS;
  op_dat div[4];
  op_dat pU, pExU, pDu;
private:
  // Pointers to private memory
  double *nodeX_data;
  double *nodeY_data;
  double *x_data;
  double *y_data;
  double *xr_data;
  double *yr_data;
  double *xs_data;
  double *ys_data;
  double *rx_data;
  double *ry_data;
  double *sx_data;
  double *sy_data;
  double *nx_data;
  double *ny_data;
  double *J_data;
  double *sJ_data;
  double *fscale_data;
  double *uD_data;
  double *qN_data;
  double *Aqbc_data;
  double *rhs_data;
  double *poissonBC_data[4];
  double *pRHS_data;
  double *div_data[4];
  double *pU_data;
  double *pExU_data;
  double *pDu_data;
};

#endif
