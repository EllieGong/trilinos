/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//                                                                                 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA                                                                                
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */

#ifndef SUNDANCE_EXPLICITCELLITERATOR_H
#define SUNDANCE_EXPLICITCELLITERATOR_H

#ifndef DOXYGEN_DEVELOPER_ONLY

#include "SundanceDefs.hpp"
#include "SundanceSet.hpp"
#include "SundanceMap.hpp"
#include "SundanceCellType.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "SundanceMesh.hpp"

namespace SundanceStdFwk
{
 using namespace SundanceUtils;
using namespace SundanceStdMesh;
using namespace SundanceStdMesh::Internal;
  namespace Internal
  {
    /**
     *
     */
    class ExplicitCellIterator
    {
    public:
      /** */
      ExplicitCellIterator(const Set<int>::const_iterator& iter);
      
      /** Dereferencing operator */
      const int& operator*() const {return *iter_;}
      
      /** Postfix increment: advances iterator and returns previous value  */
      CellIterator operator++(int) 
      {
        CellIterator old = *this;
        iter_++;
        return old;
      }
      

      /** Prefix increment: advances iterator, returning new value */
      CellIterator& operator++()
      {
        iter_++;
        return *this;
      }

      /** */
      bool operator==(const CellIterator& other) const 
      {
        return iter_ == other.iter_;
      }

      /** */
      bool operator!=(const CellIterator& other) const 
      {
        return currentLID_ == other.currentLID_;
      }

      
    private:

      /** The LID to which this iterator is currently pointing. */
      int currentLID_;

      /** Unmanaged pointer to the cell set through which this iterator
       * is iterating */
      CellSetBase* cellSet_;

      
      
    }
  }

}

#endif  /* DOXYGEN_DEVELOPER_ONLY */

#endif
