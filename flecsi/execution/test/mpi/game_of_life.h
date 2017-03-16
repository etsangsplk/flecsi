/*~-------------------------------------------------------------------------~~*
 *  @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
 * /@@/////  /@@          @@////@@ @@////// /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
 * //       ///  //////   //////  ////////  //
 *
 * Copyright (c) 2017 Los Alamos National Laboratory, LLC
 * All rights reserved
 *~-------------------------------------------------------------------------~~*/


#ifndef FLECSI_GAME_OF_LIFE_H
#define FLECSI_GAME_OF_LIFE_H

#include <cinchtest.h>
#include <numeric>
#include <mpi.h>

#include "flecsi/partition/weaver.h"
#include "flecsi/topology/index_space.h"
#include "flecsi/topology/graph_utils.h"
#include "flecsi/execution/execution.h"
#include "flecsi/data/data.h"

#include "simple_distributed_mesh.h"

template<typename T>
using accessor_t = flecsi::data::serial::dense_accessor_t<T, flecsi::data::serial_meta_data_t<flecsi::default_user_meta_data_t> >;

void stencil_task(simple_distributed_mesh_t &mesh,
                  accessor_t<int>& a0,
                  accessor_t<int>& a1)
{
  // TODO: iterate over some kind of index space
  for (int cell = 0; cell < mesh.num_primary_cells(); cell++) {
    //TODO: make 'neighbors' some kind of index space as well.
    auto neighbors = mesh.get_local_cell_neighbors(cell);

    int count = 0;
    for (auto neighbor : neighbors) {
      if (a0(neighbor) == 1) {
        count++;
      }
    }

    if (a0(cell)) {
      if (count < 2)
        a1(cell) = 0;
      else if (count <= 3)
        a1(cell) = 1;
      else
        a1(cell) = 0;
    } else {
      if (count == 3)
        a1(cell) = 1;
    }
  }
}

void halo_exchange_task(simple_distributed_mesh_t &mesh,
                        accessor_t<int>& acc)
{
  auto ranks = mesh.get_shared_peers();

  MPI_Group comm_grp, rma_group;
  MPI_Comm_group(MPI_COMM_WORLD, &comm_grp);
  MPI_Group_incl(comm_grp, ranks.size(), ranks.data(), &rma_group);
  MPI_Group_free(&comm_grp);

  // A pull model using MPI_Get:
  // 1. create MPI window for primary cell portion of the local buffer, some of
  // them are shared cells that can be fetch by peer as ghost cells.
  // TODO: need a way to automaticaly filled the data type parameters depending on the
  // value type of accessor.
  MPI_Win win;
  MPI_Win_create(&acc[0], mesh.num_primary_cells() * sizeof(int),
                 sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD,
                 &win);

  // 2. iterate through each ghost cell and MPI_Get from the peer.
  MPI_Win_post(rma_group, 0, win);
  MPI_Win_start(rma_group, 0, win);

  // TODO: still exposes ghost_cell_info.
  size_t idx = mesh.num_primary_cells();
  for (auto& cell : mesh.get_ghost_cells_info()) {
    auto local_id = mesh.local_cell_id(cell.id)
    MPI_Get(&acc[local_id], 1, MPI_INT,
            cell.rank, cell.offset, 1, MPI_INT, win);
  }

  MPI_Win_complete(win);
  MPI_Win_wait(win);

  MPI_Group_free(&rma_group);
  MPI_Win_free(&win);
}

flecsi_register_task(stencil_task, loc, single);
flecsi_register_task(halo_exchange_task, loc, single);

void driver(int argc, char **argv) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  simple_distributed_mesh_t mesh("simple2d-8x8.msh");

  flecsi_register_data(mesh, gof, alive, int, dense, 2, cells);

  auto acc0 = flecsi_get_accessor(mesh, gof, alive, int, dense, 0);
  auto acc1 = flecsi_get_accessor(mesh, gof, alive, int, dense, 1);

  // populate data storage.
  // TODO: change to mesh.cells(primary)
  for (size_t id = 0; id < mesh.indices(cells); id++) {
    acc0[id] = acc0[id] = 0;
  }

  // initialize the center 3 cells to be alive (a row), it is a period 2 blinker
  // going horizontal and then vertical.
  // FIXME: this still exposes what is supposed to be private information of the mesh (i.e.
  // primary cells and their global id. Design a better local <-> global lookup system
  // maybe based on filtering of index space.
  auto primary_cells = mesh.get_primary_cells();
  if (primary_cells.count(27) != 0) {
    auto local_id = mesh.local_cell_id(27);
    acc0(local_id) = acc1(local_id) = 1;
  }
  if (primary_cells.count(35) != 0) {
    auto local_id = mesh.local_cell_id(35);
    acc0(local_id) = acc1(local_id) = 1;
  }
  if (primary_cells.count(43) != 0) {
    auto local_id = mesh.local_cell_id(43);
    acc0(local_id) = acc1(local_id) = 1;
  }

  // add entries for ghost cells
  // TODO: add mesh::get_ghost_cells() -> index_space (in global or local?) of ghost cells.
  // or something like mesh::cells(ghost_cell).
  for (auto ghost : mesh.get_ghost_cells_info()) {
    auto local_id = mesh.local_cell_id(ghost.id);
    acc0(local_id) = acc1(local_id) = 0;
  }

  for (int i = 0; i < 5; i++) {
    if (i % 2 == 0) {
      flecsi_execute_task(halo_exchange_task, loc, single, mesh, acc0);
      flecsi_execute_task(stencil_task, loc, single, mesh, acc0, acc1);
      if (primary_cells.count(27) != 0) {
        auto local_id = mesh.local_cell_id(27);
        ASSERT_EQ(acc1(local_id), 0);
      }
      if (primary_cells.count(43) != 0) {
        auto local_id = mesh.local_cell_id(43);
        ASSERT_EQ(acc1(local_id), 0);
      }
      if (primary_cells.count(34) != 0) {
        auto local_id = mesh.local_cell_id(34);
        ASSERT_EQ(acc1(local_id), 1);
      }
      if (primary_cells.count(35) != 0) {
        auto local_id = mesh.local_cell_id(35);
        ASSERT_EQ(acc1(local_id), 1);
      }
      if (primary_cells.count(36) != 0) {
        auto local_id = mesh.local_cell_id(36);
        ASSERT_EQ(acc1(local_id), 1);
      }
    } else {
      flecsi_execute_task(halo_exchange_task, loc, single, mesh, acc1);
      flecsi_execute_task(stencil_task, loc, single, mesh, acc1, acc0);
      if (primary_cells.count(34) != 0) {
        auto local_id = mesh.local_cell_id(34);
        ASSERT_EQ(acc0(local_id), 0);
      }
      if (primary_cells.count(36) != 0) {
        auto local_id = mesh.local_cell_id(36);
        ASSERT_EQ(acc0(local_id), 0);
      }
      if (primary_cells.count(27) != 0) {
        auto local_id = mesh.local_cell_id(27);
        ASSERT_EQ(acc0(local_id), 1);
      }
      if (primary_cells.count(35) != 0) {
        auto local_id = mesh.local_cell_id(35);
        ASSERT_EQ(acc0(local_id), 1);
      }
      if (primary_cells.count(43) != 0) {
        auto local_id = mesh.local_cell_id(43);
        ASSERT_EQ(acc0(local_id), 1);
      }
    }
  }
} // driver

#endif //FLECSI_GAME_OF_LIFE_H
/*~------------------------------------------------------------------------~--*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~------------------------------------------------------------------------~--*/
