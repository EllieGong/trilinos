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

#include "SundanceQuadratureEvalMediator.hpp"
#include "SundanceCoordExpr.hpp"
#include "SundanceTempStack.hpp"
#include "SundanceCellDiameterExpr.hpp"
#include "SundanceCellVectorExpr.hpp"
#include "SundanceDiscreteFunction.hpp"
#include "SundanceDiscreteFuncElement.hpp"
#include "SundanceCellJacobianBatch.hpp"
#include "SundanceOut.hpp"
#include "SundanceTabs.hpp"
#include "SundanceExceptions.hpp"

#include "Teuchos_BLAS.hpp"


using namespace SundanceStdFwk;
using namespace SundanceStdFwk::Internal;
using namespace SundanceCore;
using namespace SundanceCore;
using namespace SundanceStdMesh;
using namespace SundanceStdMesh::Internal;
using namespace SundanceUtils;
using namespace Teuchos;
using namespace TSFExtended;


QuadratureEvalMediator
::QuadratureEvalMediator(const Mesh& mesh, 
  int cellDim,
  const QuadratureFamily& quad)
  : StdFwkEvalMediator(mesh, cellDim),
    numEvaluationCases_(-1),
    quad_(quad),
    numQuadPtsForCellType_(),
    quadPtsForReferenceCell_(),
    quadPtsReferredToMaxCell_(),
    physQuadPts_(),
    refFacetBasisVals_(2)
{}

void QuadratureEvalMediator::setCellType(const CellType& cellType,
  const CellType& maxCellType) 
{
  StdFwkEvalMediator::setCellType(cellType, maxCellType);

  if (cellType != maxCellType)
  {
    numEvaluationCases_ = numFacets(maxCellType, cellDim());
  }
  else
  {
    numEvaluationCases_ = 1;
  }

  if (quadPtsReferredToMaxCell_.containsKey(cellType)) return;

  RefCountPtr<Array<Point> > pts = rcp(new Array<Point>());
  RefCountPtr<Array<double> > wgts = rcp(new Array<double>()); 

  quad_.getPoints(cellType, *pts, *wgts);
  quadPtsForReferenceCell_.put(cellType, pts);

  numQuadPtsForCellType_.put(cellType, pts->size());

  RefCountPtr<Array<Array<Point> > > facetPts 
    = rcp(new Array<Array<Point> >(numEvaluationCases()));
  RefCountPtr<Array<Array<double> > > facetWgts 
    = rcp(new Array<Array<double> >(numEvaluationCases()));
  
  for (int fc=0; fc<numEvaluationCases(); fc++)
  {
    if (cellType != maxCellType)
    {
      quad_.getFacetPoints(maxCellType, cellDim(), fc, 
        (*facetPts)[fc], (*facetWgts)[fc]);
    }
    else
    {
      quad_.getPoints(maxCellType, (*facetPts)[fc], (*facetWgts)[fc]);
    }
  }
  quadPtsReferredToMaxCell_.put(cellType, facetPts);
}

int QuadratureEvalMediator::numQuadPts(const CellType& ct) const 
{
  TEST_FOR_EXCEPTION(!numQuadPtsForCellType_.containsKey(ct),
    RuntimeError,
    "no quadrature points have been tabulated for cell type=" << ct);
  return numQuadPtsForCellType_.get(ct);
}

void QuadratureEvalMediator::evalCellDiameterExpr(const CellDiameterExpr* expr,
  RefCountPtr<EvalVector>& vec) const 
{
  Tabs tabs;
  SUNDANCE_MSG2(verb(),tabs 
    << "QuadratureEvalMediator evaluating cell diameter expr " 
    << expr->toString());

  int nQuad = numQuadPts(cellType());
  int nCells = cellLID()->size();

  SUNDANCE_MSG3(verb(),tabs << "number of quad pts=" << nQuad);
  Array<double> diameters;
  mesh().getCellDiameters(cellDim(), *cellLID(), diameters);

  vec->resize(nQuad*nCells);
  double * const xx = vec->start();
  int k=0;
  for (int c=0; c<nCells; c++)
  {
    double h = diameters[c];
    for (int q=0; q<nQuad; q++, k++) 
    {
      xx[k] = h;
    }
  }
}

void QuadratureEvalMediator::evalCellVectorExpr(const CellVectorExpr* expr,
  RefCountPtr<EvalVector>& vec) const 
{
  Tabs tabs;
  SUNDANCE_MSG2(verb(),tabs 
    << "QuadratureEvalMediator evaluating cell normal expr " 
    << expr->toString());

  int nQuad = numQuadPts(cellType());
  int nCells = cellLID()->size();

  SUNDANCE_MSG3(verb(),tabs << "number of quad pts=" << nQuad);
  Array<Point> normals;
  mesh().outwardNormals(*cellLID(), normals);
  int dir = expr->componentIndex();

  vec->resize(nQuad*nCells);
  double * const xx = vec->start();
  int k=0;
  for (int c=0; c<nCells; c++)
  {
    double n = normals[c][dir];
    for (int q=0; q<nQuad; q++, k++) 
    {
      xx[k] = n;
    }
  }
}

void QuadratureEvalMediator::evalCoordExpr(const CoordExpr* expr,
  RefCountPtr<EvalVector>& vec) const 
{
  Tabs tabs;
  SUNDANCE_MSG2(verb(),tabs 
    << "QuadratureEvalMediator evaluating coord expr " 
    << expr->toString());
  
  computePhysQuadPts();
  int nQuad = physQuadPts_.length();
  int d = expr->dir();
  
  SUNDANCE_MSG3(verb(),tabs << "number of quad pts=" << nQuad);

  vec->resize(nQuad);
  double * const xx = vec->start();
  for (int q=0; q<nQuad; q++) 
  {
    xx[q] = physQuadPts_[q][d];
  }
}

RefCountPtr<Array<Array<Array<double> > > > QuadratureEvalMediator
::getFacetRefBasisVals(const BasisFamily& basis, int diffOrder) const
{
  Tabs tab;
  RefCountPtr<Array<Array<Array<double> > > > rtn ;

  if (cellDim() != maxCellDim())
  {
    if (!cofacetCellsAreReady()) setupFacetTransformations();
    
    TEST_FOR_EXCEPTION(!cofacetCellsAreReady(), RuntimeError, 
      "cofacet cells not ready in getFacetRefBasisVals()");
  }
  typedef OrderedPair<BasisFamily, CellType> key;

  int nDerivResults = 1;
  if (diffOrder==1) nDerivResults = maxCellDim();

  if (!refFacetBasisVals_[diffOrder].containsKey(key(basis, cellType())))
  {
    SUNDANCE_OUT(this->verb() > VerbMedium,
      tab << "computing basis values on facet quad pts");
    rtn = rcp(new Array<Array<Array<double> > >(numEvaluationCases()));

    Array<Array<Array<Array<double> > > > tmp(nDerivResults);

    for (int fc=0; fc<numEvaluationCases(); fc++)
    {
      (*rtn)[fc].resize(basis.dim());

      for (int r=0; r<nDerivResults; r++)
      {
        tmp[r].resize(basis.dim());
        MultiIndex mi;
        if (diffOrder==1)
        {
          mi[r]=1;
        }
        basis.ptr()->refEval(maxCellType(), maxCellType(), 
          (*(quadPtsReferredToMaxCell_.get(cellType())))[fc], 
          mi, tmp[r]);
      }
      /* the tmp array contains values indexed as [quad][node]. 
       * We need to put this into fortran order with quad index running
       * fastest */
      int dim = maxCellDim();
      int nQuad = tmp[0][0].size();
      int nNodes = tmp[0][0][0].size();
      int nTot = dim * nQuad * nNodes;
      for (int d=0; d<basis.dim(); d++)
      {
        (*rtn)[fc][d].resize(nTot);
        for (int r=0; r<nDerivResults; r++)
        {
          for (int q=0; q<nQuad; q++)
          {
            for (int n=0; n<nNodes; n++)
            {
              (*rtn)[fc][d][(n*nQuad + q)*nDerivResults + r] = tmp[r][d][q][n];
            }
          }
        }
      }
    }
    refFacetBasisVals_[diffOrder].put(key(basis, cellType()), rtn);
  }
  else
  {
    SUNDANCE_OUT(this->verb() > VerbMedium,
      tab << "reusing facet basis values on quad pts");
    rtn = refFacetBasisVals_[diffOrder].get(key(basis, cellType()));
  }

  return rtn;
}

Array<Array<double> >* QuadratureEvalMediator
::getRefBasisVals(const BasisFamily& basis, int diffOrder) const
{
  Tabs tab;
  RefCountPtr<Array<Array<Array<double> > > > fRtn 
    = getFacetRefBasisVals(basis, diffOrder);
  return &((*fRtn)[0]);
}


void QuadratureEvalMediator
::evalDiscreteFuncElement(const DiscreteFuncElement* expr,
  const Array<MultiIndex>& multiIndices,
  Array<RefCountPtr<EvalVector> >& vec) const
{
  const DiscreteFunctionData* f = DiscreteFunctionData::getData(expr);
  TEST_FOR_EXCEPTION(f==0, InternalError,
    "QuadratureEvalMediator::evalDiscreteFuncElement() called "
    "with expr that is not a discrete function");
  Tabs tab;

  SUNDANCE_MSG1(verb(),tab << "QuadEvalMed evaluating DF " << expr->name());

  int nQuad = numQuadPts(cellType());
  int myIndex = expr->myIndex();

  for (unsigned int i=0; i<multiIndices.size(); i++)
  {
    Tabs tab1;
    const MultiIndex& mi = multiIndices[i];
    SUNDANCE_MSG2(verb(),
      tab1 << "evaluating DF for multiindex " << mi << endl
      << tab1 << "num cells = " << cellLID()->size() << endl
      << tab1 << "num quad points = " << nQuad << endl
      << tab1 << "my index = " << expr->myIndex() << endl
      << tab1 << "num funcs = " << f->discreteSpace().nFunc());

    vec[i]->resize(cellLID()->size() * nQuad);
  
    if (mi.order() == 0)
    {
      Tabs tab2;
      if (!fCache().containsKey(f) || !fCacheIsValid()[f])
      {
        fillFunctionCache(f, mi);
      }
      else
      {
        SUNDANCE_MSG2(verb(),tab2 << "reusing function cache");
      }

      const RefCountPtr<const MapStructure>& mapStruct = mapStructCache()[f];
      int chunk = mapStruct->chunkForFuncID(myIndex);
      int funcIndex = mapStruct->indexForFuncID(myIndex);
      int nFuncs = mapStruct->numFuncs(chunk);

      SUNDANCE_MSG3(verb(),tab2 << "chunk number = " << chunk << endl
        << tab2 << "function index=" << funcIndex << " of nFuncs=" 
        << nFuncs);

      const RefCountPtr<Array<Array<double> > >& cacheVals 
        = fCache()[f];

      SUNDANCE_MSG4(verb(),tab2 << "cached function values=" << (*cacheVals)[chunk]);

      const double* cachePtr = &((*cacheVals)[chunk][0]);
      double* vecPtr = vec[i]->start();
          
      int cellSize = nQuad*nFuncs;
      int offset = funcIndex*nQuad;
      SUNDANCE_MSG3(verb(),tab2 << "cell size=" << cellSize << ", offset=" 
        << offset);
      int k = 0;
      for (unsigned int c=0; c<cellLID()->size(); c++)
      {
        for (int q=0; q<nQuad; q++, k++)
        {
          vecPtr[k] = cachePtr[c*cellSize + offset + q];
        }
      }
      SUNDANCE_MSG4(verb(),tab2 << "result vector=");
      if (verb() >= VerbExtreme)
      {
        vec[i]->print(cerr);
        computePhysQuadPts();
        k=0;
        for (unsigned int c=0; c<cellLID()->size(); c++)
        {
          Out::os() << tab2 << "-------------------------------------------"
                    << endl;
          Out::os() << tab2 << "c=" << c << " cell LID=" << (*cellLID())[c]
                    << endl;
          Tabs tab3;
          for (int q=0; q<nQuad; q++, k++)
          {
            Out::os() << tab3 << "q=" << q << " " << physQuadPts_[k]
                      << " val=" << vecPtr[k] << endl;
          }
        }
      }
    }
    else
    {
      Tabs tab2;
      if (!dfCache().containsKey(f) || !dfCacheIsValid()[f])
      {
        fillFunctionCache(f, mi);
      }
      else
      {
        SUNDANCE_MSG3(verb(),tab2 << "reusing function cache");
      }

      RefCountPtr<const MapStructure> mapStruct = mapStructCache()[f];
      int chunk = mapStruct->chunkForFuncID(myIndex);
      int funcIndex = mapStruct->indexForFuncID(myIndex);
      int nFuncs = mapStruct->numFuncs(chunk);


      SUNDANCE_MSG3(verb(),tab2 << "chunk number = " << chunk << endl
        << tab2 << "function index=" << funcIndex << " of nFuncs=" 
        << nFuncs);

      const RefCountPtr<Array<Array<double> > >& cacheVals 
        = dfCache()[f];

      SUNDANCE_MSG4(verb(),tab2 << "cached function values=" << (*cacheVals)[chunk]);

      int dim = maxCellDim();
      int pDir = mi.firstOrderDirection();
      const double* cachePtr = &((*cacheVals)[chunk][0]);
      double* vecPtr = vec[i]->start();

      int cellSize = nQuad*nFuncs*dim;
      int offset = funcIndex * nQuad * dim;
      int k = 0;

      SUNDANCE_MSG3(verb(),tab2 << "dim=" << dim << ", pDir=" << pDir
        << ", cell size=" << cellSize << ", offset=" 
        << offset);
      for (unsigned int c=0; c<cellLID()->size(); c++)
      {
        for (int q=0; q<nQuad; q++, k++)
        {
          vecPtr[k] = cachePtr[c*cellSize + offset + q*dim + pDir];
        }
      }
      SUNDANCE_MSG4(verb(),tab2 << "result vector=");
      if (verb() >= VerbExtreme)
      {
        vec[i]->print(cerr);
        computePhysQuadPts();
        k=0;
        for (unsigned int c=0; c<cellLID()->size(); c++)
        {
          Out::os() << tab2 << "-------------------------------------------"
                    << endl;
          Out::os() << tab2 << "c=" << c << " cell LID=" << (*cellLID())[c]
                    << endl;
          Tabs tab3;
          for (int q=0; q<nQuad; q++, k++)
          {
            Out::os() << tab3 << "q=" << q << " " << physQuadPts_[k]
                      << " val=" << vecPtr[k] << endl;
          }
        }
      }
    }
  }
}

void QuadratureEvalMediator::fillFunctionCache(const DiscreteFunctionData* f,
  const MultiIndex& mi) const 
{
  Tabs tab0;
  
  SUNDANCE_MSG2(verb(), tab0 << "QuadratureEvalMediator::fillFunctionCache()");
  SUNDANCE_MSG2(verb(), tab0 << "multiIndex=" << mi);
  
  
  int diffOrder = mi.order();

  int flops = 0;
  double jFlops = CellJacobianBatch::totalFlops();

  RefCountPtr<Array<Array<double> > > localValues;
  RefCountPtr<const MapStructure> mapStruct;

  Teuchos::BLAS<int,double> blas;

  {
    Tabs tab1;
    SUNDANCE_MSG2(verb(), tab1 << "packing local values");
    if (!localValueCacheIsValid().containsKey(f) 
      || !localValueCacheIsValid().get(f))
    {
      Tabs tab2;
      SUNDANCE_MSG2(verb(), tab2 << "filling cache");
      localValues = rcp(new Array<Array<double> >());
      if (cellDim() != maxCellDim())
      {
        if (!cofacetCellsAreReady()) setupFacetTransformations();
        mapStruct = f->getLocalValues(maxCellDim(), *cofacetCellLID(), 
          *localValues);
      }
      else
      {
        mapStruct = f->getLocalValues(maxCellDim(), *cellLID(), *localValues);
      }

      TEST_FOR_EXCEPT(mapStruct.get() == 0);
      localValueCache().put(f, localValues);
      mapStructCache().put(f, mapStruct);
      localValueCacheIsValid().put(f, true);
    }
    else
    {
      Tabs tab2;
      SUNDANCE_MSG2(verb(), tab2 << "reusing cache");
      localValues = localValueCache().get(f);
      mapStruct = mapStructCache().get(f);
    }
  }

  RefCountPtr<Array<Array<double> > > cacheVals;

  if (mi.order()==0)
  {
    if (fCache().containsKey(f))
    {
      cacheVals = fCache().get(f);
    }
    else
    {
      cacheVals = rcp(new Array<Array<double> >(mapStruct->numBasisChunks()));
      fCache().put(f, cacheVals);
    }
    fCacheIsValid().put(f, true);
  }
  else
  {
    if (dfCache().containsKey(f))
    {
      cacheVals = dfCache().get(f);
    }
    else
    {
      cacheVals = rcp(new Array<Array<double> >(mapStruct->numBasisChunks()));
      dfCache().put(f, cacheVals);
    }
    dfCacheIsValid().put(f, true);
  }


  
  for (int chunk=0; chunk<mapStruct->numBasisChunks(); chunk++)
  {
    Tabs tab1;
    SUNDANCE_MSG2(verb(), tab1 << "processing dof map chunk=" << chunk
      << " of " << mapStruct->numBasisChunks());
    BasisFamily basis = rcp_dynamic_cast<BasisFamilyBase>(mapStruct->basis(chunk));
    SUNDANCE_MSG4(verb(), tab1 << "basis=" << basis);

    int nFuncs = mapStruct->numFuncs(chunk);
    SUNDANCE_MSG2(verb(), tab1 << "num funcs in this chunk=" << nFuncs);
    

    Array<double>& cache = (*cacheVals)[chunk];

    int nQuad = numQuadPts(cellType());
    int nCells = cellLID()->size();
    SUNDANCE_MSG2(verb(), tab1 << "num quad points=" << nQuad);
    SUNDANCE_MSG2(verb(), tab1 << "num cells=" << nCells);

    int nDir;

    if (mi.order()==1)
    {
      nDir = maxCellDim();
    }
    else
    {
      nDir = 1;
    }
    cache.resize(cellLID()->size() * nQuad * nDir * nFuncs);

      
    /* 
     * Sum over nodal values, which we can do with a matrix-matrix multiply
     * between the ref basis values and the local function values.
     *
     * There are two cases: (1) When we are evaluating 
     * on a facet, we must use different sets of reference function values
     * on the different facets. We must therefore loop over the evaluation
     * cells, using a vector of reference values chosen according to the
     * facet number of the current cell.  Let A be the
     * (nQuad*nDir)-by-(nNode) matrix of reference basis values for the
     * current cell's facet index and B be the (nNode)-by-(nFuncs) matrix of
     * function coefficient values for the current cell. Then C = A * B is
     * the (nQuad*nDir)-by-(nFunc) matrix of the function's derivative
     * values at the quadrature points in the current cell.  Each
     * matrix-matrix multiplication is done with a call to dgemm.
     *
     * (2) In other cases, we're either evaluating spatial derivatives on a
     * maximal cell or evaluating 0-order derivatives on a submaximal
     * cell. In these cases, all cells in the workset have the same
     * reference values. This lets us reuse the same matrix A on all matrix
     * multiplications, so that we can assemble one big
     * (nNode)-by-(nFuncs*nCells) matrix B and do all cells with a single
     * dgemm call to multiply A*B. The result C is then a single
     * (nQuad*nDir)-by-(nFuncs*nCells) matrix.

    */
    if (cellDim() != maxCellDim())
    {
      Tabs tab2;
      SUNDANCE_MSG2(verb(), 
        tab2 << "evaluating by reference to maximal cell");
      
      RefCountPtr<Array<Array<Array<double> > > > refFacetBasisValues 
        = getFacetRefBasisVals(basis, mi.order());
      /* Note: even though we're ultimately not evaluating on 
       * maxCellType() here, use maxCellType() for both arguments
       * to nReferenceDOFs() because derivatives need to be
       * evaluated using all DOFs from the maximal cell, not just
       * those on the facet.
       */
      int nNodes = basis.nReferenceDOFs(maxCellType(), maxCellType());
      int nRowsA = nQuad*nDir;
      int nColsA = nNodes;
      int nColsB = nFuncs; 
      int lda = nRowsA;
      int ldb = nNodes;
      int ldc = lda;
      double alpha = 1.0;
      double beta = 0.0;

      SUNDANCE_MSG2(verb(), tab2 << "building transformation matrices cell-by-cell");
      int vecComp = 0;
      for (int c=0; c<nCells; c++)
      {
        int facetIndex = facetIndices()[c];
        double* A = &((*refFacetBasisValues)[facetIndex][vecComp][0]);
        double* B = &((*localValues)[chunk][c*nNodes*nFuncs]);
        double* C = &((*cacheVals)[chunk][c*nRowsA*nColsB]);
        blas.GEMM( Teuchos::NO_TRANS, Teuchos::NO_TRANS, nRowsA, nColsB, nColsA,
          alpha, A, lda, B, ldb, beta, C, ldc);
      }
    }
    else /* cellDim() == maxCellDim() */
    {
      /* 
       * Sum over nodal values, which we can do with a matrix-matrix multiply
       * between the ref basis values and the local function values.
       */
      Tabs tab2;
      SUNDANCE_MSG2(verb(), tab2 << "building batch transformation matrix");

      Array<Array<double> >* refBasisValues 
        = getRefBasisVals(basis, diffOrder);
      int nNodes = basis.nReferenceDOFs(maxCellType(), maxCellType());
      int nRowsA = nQuad*nDir;
      int nColsA = nNodes;
      int nColsB = nFuncs*nCells; 
      int lda = nRowsA;
      int ldb = nNodes;
      int ldc = lda;
      double alpha = 1.0;
      double beta = 0.0;
      int vecComp = 0;
      if (verb() >= VerbExtreme)
      {
        Tabs tab3;
        Out::os() << tab2 << "Printing values at nodes" << endl;
        for (int c=0; c<nCells; c++)
        {
          Out::os() << tab3 << "-------------------------------------------"
                    << endl;
          Out::os() << tab3 << "c=" << c << " cell LID=" << (*cellLID())[c]
                    << endl;
          Tabs tab4;
          int offset = c*nNodes*nFuncs;
          for (int n=0; n<nNodes; n++)
          {
            Out::os() << tab4 << "n=" << n;
            for (int f=0; f<nFuncs; f++)
            {
              Out::os() << " " << (*localValues)[chunk][offset + n*nFuncs + f];
            }
            Out::os() << endl;
          }
        }
        Out::os() << tab2 << "-------------------------------------------";
        Out::os() << tab2 << "Printing reference basis at nodes" << endl;
        Out::os() << tab2 << "-------------------------------------------"
                  << endl;
        for (int n=0; n<nNodes; n++)
        {
          Out::os() << tab3 << "node=" << n << endl;
          for (int q=0; q<nQuad; q++)
          {
            Tabs tab4;
            Out::os() << tab4 << "q=" << q;
            for (int r=0; r<nDir; r++)
            {
              Out::os () << " " 
                         << (*refBasisValues)[vecComp][(n*nQuad + q)*nDir + r];
            }
            Out::os() << endl;
          }
        }
      }
      double* A = &((*refBasisValues)[vecComp][0]);
      double* B = &((*localValues)[chunk][0]);
      double* C = &((*cacheVals)[chunk][0]);
      blas.GEMM( Teuchos::NO_TRANS, Teuchos::NO_TRANS, nRowsA, nColsB, nColsA, alpha,
        A, lda, B, ldb, beta, C, ldc );
    }

    SUNDANCE_MSG2(verb(), tab1 << "doing transformations via dgemm");
    /* Transform derivatives to physical coordinates */
    const CellJacobianBatch& J = JTrans();
    double* C = &((*cacheVals)[chunk][0]);
    if (mi.order()==1)
    {
      Tabs tab2;
      SUNDANCE_MSG2(verb(), tab2 << "Jacobian batch nCells=" << J.numCells());
      SUNDANCE_MSG2(verb(), tab2 << "Jacobian batch cell dim=" << J.cellDim());
      SUNDANCE_MSG2(verb(), tab2 << "Jacobian batch spatial dim=" << J.spatialDim());
    
      int nRhs = nQuad * nFuncs;
      for (unsigned int c=0; c<cellLID()->size(); c++)
      {
        double* rhsPtr = &(C[(nRhs * nDir)*c]);
        J.applyInvJ(c, 0, rhsPtr, nRhs, false);
      }
    }
    SUNDANCE_MSG2(verb(), tab1 << "done transformations");
  }

  jFlops = CellJacobianBatch::totalFlops() - jFlops;
  addFlops(flops + jFlops);

  SUNDANCE_MSG2(verb(), 
    tab0 << "done QuadratureEvalMediator::fillFunctionCache()");
}

void QuadratureEvalMediator::computePhysQuadPts() const 
{
  if (cacheIsValid()) 
  {
    Tabs tab0(0);
    SUNDANCE_MSG2(verb(), tab0 << "reusing phys quad points");
  }
  else
  {
    Tabs tab0(0);
    double jFlops = CellJacobianBatch::totalFlops();
    SUNDANCE_MSG2(verb(), tab0 << "computing phys quad points");
    physQuadPts_.resize(0);
    if (cellDim() != maxCellDim())
    {
      Tabs tab1;
      SUNDANCE_MSG2(verb(), tab1 << "using cofacets");
      SUNDANCE_MSG3(verb(), tab1 << "num cofacets = " << cofacetCellLID()->size());
      Array<Point> tmp;
      Array<int> cell(1);
      for (unsigned int c=0; c<cellLID()->size(); c++)
      {
        cell[0] = (*cofacetCellLID())[c];
        int facetIndex = facetIndices()[c];
        const Array<Point>& refFacetPts 
          =  (*(quadPtsReferredToMaxCell_.get(cellType())))[facetIndex];
        tmp.resize(refFacetPts.size());
        mesh().pushForward(maxCellDim(), cell, refFacetPts, tmp);
        for (int q=0; q<tmp.size(); q++)
        {
          physQuadPts_.append(tmp[q]);
        }
      }
    }
    else
    {
      const Array<Point>& refPts 
        = *(quadPtsForReferenceCell_.get(cellType()));
      mesh().pushForward(cellDim(), *cellLID(), 
        refPts, physQuadPts_); 
    }
    addFlops(CellJacobianBatch::totalFlops() - jFlops);
    cacheIsValid() = true;
    SUNDANCE_OUT(this->verb() > VerbMedium, 
      "phys quad: " << physQuadPts_);
  }
}


void QuadratureEvalMediator::print(ostream& os) const 
{
  if (cacheIsValid())
  {
    Tabs tab0;
    os << tab0 << "Physical quadrature points" << endl;
    int i=0;
    for (unsigned int c=0; c<cellLID()->size(); c++)
    {
      Tabs tab1;
      os << tab1 << "cell " << c << endl;
      for (unsigned int q=0; q<physQuadPts_.size()/cellLID()->size(); q++, i++)
      {
        Tabs tab2;
        os << tab2 << "q=" << q << " " << physQuadPts_[i] << endl;
      }
    }
  }
}


