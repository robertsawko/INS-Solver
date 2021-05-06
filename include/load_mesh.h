#ifndef __INS_LOADMESH_H
#define __INS_LOADMESH_H
// Include CGNS stuff
#include "cgnslib.h"
// Include C++ stuff
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "ins_data.h"

// Template functions for loading cells into an int array
// (cgsize_t can be long or int depending on how the library was compiled)
template<typename T>
void cgns_load_cells(int file, int baseIndex, int zoneIndex, int *cgnsCells, int numCells);

template<>
void cgns_load_cells<int>(int file, int baseIndex, int zoneIndex, int *cgnsCells, int numCells) {
  std::vector<cgsize_t> cells(numCells * 3);
  cgsize_t parentData;
  cg_elements_read(file, baseIndex, zoneIndex, 1, cells.data(), &parentData);
  std::transform(cells.begin(), cells.end(), cgnsCells, [](int x) { return x - 1;});
  // cgsize_t parentData;
  // cg_elements_read(file, baseIndex, zoneIndex, 1, (cgsize_t *)cgnsCells, &parentData);
  // // CGNS starts numbering from 1 but OP2 starts from 0
  // std::transform(cgnsCells, cgnsCells + 3 * numCells, cgnsCells, [](int x) { return x - 1;})
  ;
}

template<>
void cgns_load_cells<long>(int file, int baseIndex, int zoneIndex, int *cgnsCells, int numCells) {
  std::vector<cgsize_t> cells(numCells * 3);
  cgsize_t parentData;
  cg_elements_read(file, baseIndex, zoneIndex, 1, cells.data(), &parentData);
  // CGNS starts numbering from 1 but OP2 starts from 0
  std::transform(cells.begin(), cells.end(), cgnsCells, [](long x) { return (int)x - 1;});
}

#ifdef INS_MPI

#include "mpi.h"

int compute_local_size(int global_size, int mpi_comm_size, int mpi_rank) {
  int local_size = global_size / mpi_comm_size;
  int remainder = global_size % mpi_comm_size;

  if (mpi_rank < remainder) {
    local_size = local_size + 1;
  }
  return local_size;
}

void scatter_double_array(double *g_array, double *l_array, int comm_size,
                          int g_size, int l_size, int elem_size) {
  int *sendcnts = (int *)malloc(comm_size * sizeof(int));
  int *displs = (int *)malloc(comm_size * sizeof(int));
  int disp = 0;

  for (int i = 0; i < comm_size; i++) {
    sendcnts[i] = elem_size * compute_local_size(g_size, comm_size, i);
  }
  for (int i = 0; i < comm_size; i++) {
    displs[i] = disp;
    disp = disp + sendcnts[i];
  }

  MPI_Scatterv(g_array, sendcnts, displs, MPI_DOUBLE, l_array,
               l_size * elem_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  free(sendcnts);
  free(displs);
}

void scatter_int_array(int *g_array, int *l_array, int comm_size, int g_size,
                       int l_size, int elem_size) {
  int *sendcnts = (int *)malloc(comm_size * sizeof(int));
  int *displs = (int *)malloc(comm_size * sizeof(int));
  int disp = 0;

  for (int i = 0; i < comm_size; i++) {
    sendcnts[i] = elem_size * compute_local_size(g_size, comm_size, i);
  }
  for (int i = 0; i < comm_size; i++) {
    displs[i] = disp;
    disp = disp + sendcnts[i];
  }

  MPI_Scatterv(g_array, sendcnts, displs, MPI_INT, l_array, l_size * elem_size,
               MPI_INT, 0, MPI_COMM_WORLD);

  free(sendcnts);
  free(displs);
}

template<typename Func>
void load_mesh(std::string filename, INSData *data, Func bcNum) {
  int rank;
  int comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  int numNodes_g, numCells_g, numEdges_g, numBoundaryEdges_g;
  double *coords_g;
  int *cells_g, *edge2node_g, *edge2cell_g, *edgeNum_g;
  int *bedge2node_g, *bedge2cell_g, *bedgeNum_g, *bedge_type_g;

  std::cout << "MPI_ROOT: " << MPI_ROOT << std::endl;

  if(rank == 0) {
    // Read CGNS grid
    int file;
    if(cg_open(filename.c_str(), CG_MODE_READ, &file)) {
      cg_error_exit();
    }

    int baseIndex = 1;
    int zoneIndex = 1;
    cgsize_t cg_numNodes;
    char zoneName[33];
    // Get zone name and size
    cg_zone_read(file, baseIndex, zoneIndex, zoneName, &cg_numNodes);
    numNodes_g = (int) cg_numNodes;
    std::cout << "Zone Name: " << zoneName << std::endl;
    std::cout << "Zone Size: " << numNodes_g << std::endl;

    // Get vertices
    std::vector<double> x(numNodes_g);
    std::vector<double> y(numNodes_g);
    cgsize_t minVertex = 1;
    cgsize_t maxVertex = cg_numNodes;
    cg_coord_read(file, baseIndex, zoneIndex, "CoordinateX",
                  CGNS_ENUMV(RealDouble), &minVertex, &maxVertex, x.data());
    cg_coord_read(file, baseIndex, zoneIndex, "CoordinateY",
                  CGNS_ENUMV(RealDouble), &minVertex, &maxVertex, y.data());

    coords_g = (double *)malloc(2 * numNodes_g * sizeof(double));
    for(int i = 0; i < x.size(); i++) {
      coords_g[2 * i] = x[i];
      coords_g[2 * i + 1] = y[i];
    }

    // Get number of sections
    int numSections;
    cg_nsections(file, baseIndex, zoneIndex, &numSections);
    std::cout << "Number of sections: " << numSections << std::endl << std::endl;

    // Get cell section
    char sectionName[33];
    CGNS_ENUMT(ElementType_t) elementType;
    cgsize_t elementStart, elementEnd;
    int elementNumBoundary, parentFlag;
    cg_section_read(file, baseIndex, zoneIndex, 1, sectionName, &elementType,
                    &elementStart, &elementEnd, &elementNumBoundary, &parentFlag);
    std::cout << "Section 1: " << sectionName << std::endl;
    std::cout << "Element Type: " << ElementTypeName[elementType] << std::endl;
    std::cout << "Start: " << elementStart << " End: " << elementEnd << std::endl;
    std::cout << "Element Number Boundary: " << elementNumBoundary;
    std::cout << " Parent Flag: " << parentFlag << std::endl;

    // Get cells
    numCells_g = elementEnd - elementStart + 1;
    cells_g = (int *)malloc(numCells_g * 3 * sizeof(int));
    cgns_load_cells<cgsize_t>(file, baseIndex, zoneIndex, cells_g, numCells_g);

    // Get edge data
    cg_gopath(file, "/Base/Zone1/Edges");
    char arrayName[33];
    DataType_t arrayDataType;
    int arrayRank;
    cgsize_t arrayDims[2];
    cg_array_info(1, arrayName, &arrayDataType, &arrayRank, arrayDims);
    std::cout << "Array Name: " << arrayName << std::endl;
    std::cout << "Array Dims: " << arrayDims[0] << " " << arrayDims[1] << std::endl;
    numEdges_g = arrayDims[1];
    edge2node_g = (int *)malloc(2 * numEdges_g * sizeof(int));
    edge2cell_g = (int *)malloc(2 * numEdges_g * sizeof(int));
    edgeNum_g   = (int *)malloc(2 * numEdges_g * sizeof(int));
    std::vector<int> edgeData(arrayDims[0] * arrayDims[1]);
    cg_array_read(1, edgeData.data());

    for(int i = 0; i < numEdges_g; i++) {
      // - 1 as CGNS counts points from 1 but OP2 counts from 0
      // Cell index do start from one in this data
      edge2node_g[i * 2]     = edgeData[i * 6] - 1;
      edge2node_g[i * 2 + 1] = edgeData[i * 6 + 1] - 1;
      edge2cell_g[i * 2]     = edgeData[i * 6 + 2];
      edge2cell_g[i * 2 + 1] = edgeData[i * 6 + 3];
      edgeNum_g[i * 2]       = edgeData[i * 6 + 4];
      edgeNum_g[i * 2 + 1]   = edgeData[i * 6 + 5];
    }

    // Get boundary edge data
    cg_gopath(file, "/Base/Zone1/BoundaryEdges");
    char barrayName[33];
    DataType_t barrayDataType;
    int barrayRank;
    cgsize_t barrayDims[2];
    cg_array_info(1, barrayName, &barrayDataType, &barrayRank, barrayDims);
    std::cout << "Array Name: " << barrayName << std::endl;
    std::cout << "Array Dims: " << barrayDims[0] << " " << barrayDims[1] << std::endl;
    numBoundaryEdges_g = barrayDims[1];
    bedge2node_g = (int *)malloc(2 * numBoundaryEdges_g * sizeof(int));
    bedge2cell_g = (int *)malloc(numBoundaryEdges_g * sizeof(int));
    bedgeNum_g   = (int *)malloc(numBoundaryEdges_g * sizeof(int));
    bedge_type_g = (int *)malloc(numBoundaryEdges_g * sizeof(int));
    std::vector<int> bedgeData(barrayDims[0] * barrayDims[1]);
    cg_array_read(1, bedgeData.data());

    for(int i = 0; i < numBoundaryEdges_g; i++) {
      // - 1 as CGNS counts points from 1 but OP2 counts from 0
      // Cell index do start from one in this data
      bedge2node_g[i * 2]     = bedgeData[i * 4] - 1;
      bedge2node_g[i * 2 + 1] = bedgeData[i * 4 + 1] - 1;
      bedge2cell_g[i]         = bedgeData[i * 4 + 2];
      bedgeNum_g[i]           = bedgeData[i * 4 + 3];
      // Set boundary type
      // 0 = inflow, 1 = outflow, 2 = wall
      double x1 = coords_g[2 * bedge2node_g[i * 2]];
      double y1 = coords_g[2 * bedge2node_g[i * 2] + 1];
      double x2 = coords_g[2 * bedge2node_g[i * 2 + 1]];
      double y2 = coords_g[2 * bedge2node_g[i * 2 + 1] + 1];

      bedge_type_g[i] = bcNum(x1, x2, y1, y2);
    }

    cg_close(file);
  }

  MPI_Bcast(&numNodes_g, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&numCells_g, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&numEdges_g, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&numBoundaryEdges_g, 1, MPI_INT, 0, MPI_COMM_WORLD);

  data->numNodes         = compute_local_size(numNodes_g, comm_size, rank);
  data->numCells         = compute_local_size(numCells_g, comm_size, rank);
  data->numEdges         = compute_local_size(numEdges_g, comm_size, rank);
  data->numBoundaryEdges = compute_local_size(numBoundaryEdges_g, comm_size, rank);
  // std::cout << data->numNodes << "," << data->numCells << "," << data->numEdges << "," << data->numBoundaryEdges << std::endl;

  data->coords          = (double *)malloc(2 * data->numNodes * sizeof(double));
  data->cgnsCells       = (int *)malloc(data->numCells * 3 * sizeof(int));
  data->edge2node_data  = (int *)malloc(2 * data->numEdges * sizeof(int));
  data->edge2cell_data  = (int *)malloc(2 * data->numEdges * sizeof(int));
  data->edgeNum_data    = (int *)malloc(2 * data->numEdges * sizeof(int));
  data->bedge2node_data = (int *)malloc(2 * data->numBoundaryEdges * sizeof(int));
  data->bedge2cell_data = (int *)malloc(data->numBoundaryEdges * sizeof(int));
  data->bedgeNum_data   = (int *)malloc(data->numBoundaryEdges * sizeof(int));
  data->bedge_type_data = (int *)malloc(data->numBoundaryEdges * sizeof(int));

  scatter_double_array(coords_g, data->coords, comm_size, numNodes_g, data->numNodes, 2);
  scatter_int_array(cells_g, data->cgnsCells, comm_size, numCells_g, data->numCells, 3);
  scatter_int_array(edge2node_g, data->edge2node_data, comm_size, numEdges_g, data->numEdges, 2);
  scatter_int_array(edge2cell_g, data->edge2cell_data, comm_size, numEdges_g, data->numEdges, 2);
  scatter_int_array(edgeNum_g, data->edgeNum_data, comm_size, numEdges_g, data->numEdges, 2);
  scatter_int_array(bedge2node_g, data->bedge2node_data, comm_size, numBoundaryEdges_g, data->numBoundaryEdges, 2);
  scatter_int_array(bedge2cell_g, data->bedge2cell_data, comm_size, numBoundaryEdges_g, data->numBoundaryEdges, 1);
  scatter_int_array(bedgeNum_g, data->bedgeNum_data, comm_size, numBoundaryEdges_g, data->numBoundaryEdges, 1);
  scatter_int_array(bedge_type_g, data->bedge_type_data, comm_size, numBoundaryEdges_g, data->numBoundaryEdges, 1);

  if(rank == 0) {
    free(coords_g);
    free(cells_g);
    free(edge2node_g);
    free(edge2cell_g);
    free(edgeNum_g);
    free(bedge2node_g);
    free(bedge2cell_g);
    free(bedgeNum_g);
  }
}

#else

template<typename Func>
void load_mesh(std::string filename, INSData *nsData, Func bcNum) {
  // Read CGNS grid
  int file;
  if(cg_open(filename.c_str(), CG_MODE_READ, &file)) {
    cg_error_exit();
  }

  int baseIndex = 1;
  int zoneIndex = 1;
  cgsize_t cg_numNodes;
  char zoneName[33];
  // Get zone name and size
  cg_zone_read(file, baseIndex, zoneIndex, zoneName, &cg_numNodes);
  nsData->numNodes = (int) cg_numNodes;
  std::cout << "Zone Name: " << zoneName << std::endl;
  std::cout << "Zone Size: " << nsData->numNodes << std::endl;

  // Get vertices
  std::vector<double> x(nsData->numNodes);
  std::vector<double> y(nsData->numNodes);
  cgsize_t minVertex = 1;
  cgsize_t maxVertex = cg_numNodes;
  cg_coord_read(file, baseIndex, zoneIndex, "CoordinateX",
                CGNS_ENUMV(RealDouble), &minVertex, &maxVertex, x.data());
  cg_coord_read(file, baseIndex, zoneIndex, "CoordinateY",
                CGNS_ENUMV(RealDouble), &minVertex, &maxVertex, y.data());

  nsData->coords = (double *)malloc(2 * nsData->numNodes * sizeof(double));
  for(int i = 0; i < x.size(); i++) {
    nsData->coords[2 * i] = x[i];
    nsData->coords[2 * i + 1] = y[i];
  }

  // Get number of sections
  int numSections;
  cg_nsections(file, baseIndex, zoneIndex, &numSections);
  std::cout << "Number of sections: " << numSections << std::endl << std::endl;

  // Get cell section
  char sectionName[33];
  CGNS_ENUMT(ElementType_t) elementType;
  cgsize_t elementStart, elementEnd;
  int elementNumBoundary, parentFlag;
  cg_section_read(file, baseIndex, zoneIndex, 1, sectionName, &elementType,
                  &elementStart, &elementEnd, &elementNumBoundary, &parentFlag);
  std::cout << "Section 1: " << sectionName << std::endl;
  std::cout << "Element Type: " << ElementTypeName[elementType] << std::endl;
  std::cout << "Start: " << elementStart << " End: " << elementEnd << std::endl;
  std::cout << "Element Number Boundary: " << elementNumBoundary;
  std::cout << " Parent Flag: " << parentFlag << std::endl;

  // Get cells
  nsData->numCells = elementEnd - elementStart + 1;
  nsData->cgnsCells = (int *)malloc(nsData->numCells * 3 * sizeof(int));
  cgns_load_cells<cgsize_t>(file, baseIndex, zoneIndex, nsData->cgnsCells, nsData->numCells);

  // Get edge data
  cg_gopath(file, "/Base/Zone1/Edges");
  char arrayName[33];
  DataType_t arrayDataType;
  int arrayRank;
  cgsize_t arrayDims[2];
  cg_array_info(1, arrayName, &arrayDataType, &arrayRank, arrayDims);
  std::cout << "Array Name: " << arrayName << std::endl;
  std::cout << "Array Dims: " << arrayDims[0] << " " << arrayDims[1] << std::endl;
  nsData->numEdges = arrayDims[1];
  nsData->edge2node_data = (int *)malloc(2 * nsData->numEdges * sizeof(int));
  nsData->edge2cell_data = (int *)malloc(2 * nsData->numEdges * sizeof(int));
  nsData->edgeNum_data   = (int *)malloc(2 * nsData->numEdges * sizeof(int));
  std::vector<int> edgeData(arrayDims[0] * arrayDims[1]);
  cg_array_read(1, edgeData.data());

  for(int i = 0; i < nsData->numEdges; i++) {
    // - 1 as CGNS counts points from 1 but OP2 counts from 0
    // Cell index do start from one in this data
    nsData->edge2node_data[i * 2]     = edgeData[i * 6] - 1;
    nsData->edge2node_data[i * 2 + 1] = edgeData[i * 6 + 1] - 1;
    nsData->edge2cell_data[i * 2]     = edgeData[i * 6 + 2];
    nsData->edge2cell_data[i * 2 + 1] = edgeData[i * 6 + 3];
    nsData->edgeNum_data[i * 2]       = edgeData[i * 6 + 4];
    nsData->edgeNum_data[i * 2 + 1]   = edgeData[i * 6 + 5];
  }

  // Get boundary edge data
  cg_gopath(file, "/Base/Zone1/BoundaryEdges");
  char barrayName[33];
  DataType_t barrayDataType;
  int barrayRank;
  cgsize_t barrayDims[2];
  cg_array_info(1, barrayName, &barrayDataType, &barrayRank, barrayDims);
  std::cout << "Array Name: " << barrayName << std::endl;
  std::cout << "Array Dims: " << barrayDims[0] << " " << barrayDims[1] << std::endl;
  nsData->numBoundaryEdges = barrayDims[1];
  nsData->bedge2node_data = (int *)malloc(2 * nsData->numBoundaryEdges * sizeof(int));
  nsData->bedge2cell_data = (int *)malloc(nsData->numBoundaryEdges * sizeof(int));
  nsData->bedgeNum_data   = (int *)malloc(nsData->numBoundaryEdges * sizeof(int));
  nsData->bedge_type_data = (int *)malloc(nsData->numBoundaryEdges * sizeof(int));
  std::vector<int> bedgeData(barrayDims[0] * barrayDims[1]);
  cg_array_read(1, bedgeData.data());

  for(int i = 0; i < nsData->numBoundaryEdges; i++) {
    // - 1 as CGNS counts points from 1 but OP2 counts from 0
    // Cell index do start from one in this data
    nsData->bedge2node_data[i * 2]     = bedgeData[i * 4] - 1;
    nsData->bedge2node_data[i * 2 + 1] = bedgeData[i * 4 + 1] - 1;
    nsData->bedge2cell_data[i]         = bedgeData[i * 4 + 2];
    nsData->bedgeNum_data[i]           = bedgeData[i * 4 + 3];
    // Set boundary type
    // 0 = inflow, 1 = outflow, 2 = wall
    double x1 = nsData->coords[2 * nsData->bedge2node_data[i * 2]];
    double y1 = nsData->coords[2 * nsData->bedge2node_data[i * 2] + 1];
    double x2 = nsData->coords[2 * nsData->bedge2node_data[i * 2 + 1]];
    double y2 = nsData->coords[2 * nsData->bedge2node_data[i * 2 + 1] + 1];

    nsData->bedge_type_data[i] = bcNum(x1, x2, y1, y2);
  }

  cg_close(file);
}

#endif

#endif
