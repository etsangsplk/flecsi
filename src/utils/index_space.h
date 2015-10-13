/*~--------------------------------------------------------------------------~*
 * Copyright (c) 2015 Los Alamos National Security, LLC
 * All rights reserved.
 *~--------------------------------------------------------------------------~*/

#ifndef flexi_index_space_h
#define flexi_index_space_h

#include "iterator.h"

/*!
 * \file index_space.h
 * \authors bergen
 * \date Initial file creation: Oct 09, 2015
 */

namespace flexi {

/*!
  \class index_space index_space.h
  \brief index_space provides...
 */

class index_space_t
{
public:

  using iterator_t = iterator<index_space_t>;
  using type_t = size_t;

  //! Default constructor
  index_space_t(size_t size) : size_(size), index_(0) {}

  //! Copy constructor
  index_space_t(const index_space_t & is) {
    size_ = is.size_;
    index_ = is.index_;
  } // index_space_t

  //! Assignment operator
  index_space_t & operator = (const index_space_t & is) {
    size_ = is.size_;
  } // operator =

  //! Destructor
   ~index_space_t() {}

  size_t operator [] (size_t index) const {
    return index;
  } // operator []

  size_t & operator [] (size_t index) {
    index_ = index;
    return index_;
  } // operator []

  iterator_t begin() { return { *this, 0 }; }
  iterator_t end() { return { *this, size_}; }

private:

  size_t size_;
  size_t index_;

}; // class index_space_t

} // namespace flexi

#endif // flexi_index_space_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options for vim.
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/