#~----------------------------------------------------------------------------~#
#  @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
# /@@/////  /@@          @@////@@ @@////// /@@
# /@@       /@@  @@@@@  @@    // /@@       /@@
# /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
# /@@////   /@@/@@@@@@@/@@       ////////@@/@@
# /@@       /@@/@@//// //@@    @@       /@@/@@
# /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
# //       ///  //////   //////  ////////  // 
# 
# Copyright (c) 2016 Los Alamos National Laboratory, LLC
# All rights reserved
#~----------------------------------------------------------------------------~#

set(io_HEADERS
  io.h
  io_base.h
  simple_definition.h
)

cinch_add_unit(simple_definition
  SOURCES test/simple_definition.cc
  INPUTS test/simple2d-8x8.msh test/simple2d-4x4.msh
)

# enable IO really means enable exodus
if(ENABLE_EXODUS)

  set(io_HEADERS
    ${io_HEADERS}
    io_exodus.h
    PARENT_SCOPE
  )

  cinch_add_unit(io
    SOURCES test/io.cc
  )

# no exodus was found
else()

  set(io_HEADERS
    ${io_HEADERS}
    PARENT_SCOPE
  )

endif(ENABLE_EXODUS)

set(io_SOURCES
  PARENT_SCOPE
  )

#~---------------------------------------------------------------------------~-#
# Formatting options
# vim: set tabstop=2 shiftwidth=2 expandtab :
#~---------------------------------------------------------------------------~-#
