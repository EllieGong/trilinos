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

#ifndef SUNDANCE_QUADRATUREINTEGRAL_H
#define SUNDANCE_QUADRATUREINTEGRAL_H

#include "SundanceDefs.hpp"
#include "SundanceQuadratureIntegralBase.hpp"

namespace SundanceStdFwk
{
using namespace SundanceUtils;
using namespace SundanceStdMesh;
using namespace SundanceStdMesh::Internal;
using namespace SundanceCore;
using namespace SundanceCore;

namespace Internal
{
using namespace Teuchos;

/** 
 *  
 *
 */
class QuadratureIntegral 
  : public QuadratureIntegralBase
{
public:
  /** Construct a zero-form to be computed by quadrature */
  QuadratureIntegral(int spatialDim,
    const CellType& maxCellType,
    int dim, 
    const CellType& cellType,
    const QuadratureFamily& quad,
    int verb);

  /** Construct a one form to be computed by quadrature */
  QuadratureIntegral(int spatialDim,
    const CellType& maxCellType,
    int dim, 
    const CellType& cellType,
    const BasisFamily& testBasis,
    int alpha,
    int testDerivOrder,
    const QuadratureFamily& quad,
    int verb);

  /** Construct a two-form to be computed by quadrature */
  QuadratureIntegral(int spatialDim,
    const CellType& maxCellType,
    int dim,
    const CellType& cellType,
    const BasisFamily& testBasis,
    int alpha,
    int testDerivOrder,
    const BasisFamily& unkBasis,
    int beta,
    int unkDerivOrder,
    const QuadratureFamily& quad,
    int verb);

  /** virtual dtor */
  virtual ~QuadratureIntegral(){;}

  /** */
  virtual void transformZeroForm(const CellJacobianBatch& JTrans,
				 const CellJacobianBatch& JVol,
				 const Array<int>& isLocalFlag,
				 const Array<int>& facetIndex,
				 const double* const coeff,
				 RefCountPtr<Array<double> >& A) const ;
      
  /** */
  virtual void transformTwoForm(const CellJacobianBatch& JTrans,
				const CellJacobianBatch& JVol,
				const Array<int>& facetIndex,
				const double* const coeff,
				RefCountPtr<Array<double> >& A) const ;
      
  /** */
  void transformOneForm(const CellJacobianBatch& JTrans,
			const CellJacobianBatch& JVol,
			const Array<int>& facetIndex,
			const double* const coeff,
			RefCountPtr<Array<double> >& A) const ;

private:

  /** Do the integration by summing reference quantities over quadrature
   * points and then transforming the sum to physical quantities.  */
  void transformSummingFirst(int nCells,
    const Array<int>& facetIndex,
    const double* const GPtr,
    const double* const coeff,
    RefCountPtr<Array<double> >& A) const ;

  /** Do the integration by transforming to physical coordinates 
   * at each quadrature point, and then summing */
  void transformSummingLast(int nCells,
    const Array<int>& facetIndex,
    const double* const GPtr,
    const double* const coeff,
    RefCountPtr<Array<double> >& A) const ;

  /** Determine whether to do this batch of integrals using the
   * sum-first method or the sum-last method */
  bool useSumFirstMethod() const {return useSumFirstMethod_;}
      
  /** */
  inline double& wValue(int facetCase, 
    int q, int testDerivDir, int testNode,
    int unkDerivDir, int unkNode)
    {return W_[facetCase][unkNode
        + nNodesUnk()
        *(testNode + nNodesTest()
          *(unkDerivDir + nRefDerivUnk()
            *(testDerivDir + nRefDerivTest()*q)))];}

      

  /** */
  inline const double& wValue(int facetCase, 
    int q, 
    int testDerivDir, int testNode,
    int unkDerivDir, int unkNode) const 
    {
      return W_[facetCase][unkNode
        + nNodesUnk()
        *(testNode + nNodesTest()
          *(unkDerivDir + nRefDerivUnk()
            *(testDerivDir + nRefDerivTest()*q)))];
    }
      
  /** */
  inline double& wValue(int facetCase, 
    int q, int testDerivDir, int testNode)
    {return W_[facetCase][testNode + nNodesTest()*(testDerivDir + nRefDerivTest()*q)];}


  /** */
  inline const double& wValue(int facetCase, 
    int q, int testDerivDir, int testNode) const 
    {return W_[facetCase][testNode + nNodesTest()*(testDerivDir + nRefDerivTest()*q)];}

  /* */
  Array<Array<double> > W_;

  /* */
  bool useSumFirstMethod_;
      
};
}
}


#endif
