/* @HEADER@ */
/* @HEADER@ */

#include "SundanceDOFMapBuilder.hpp"
#include "SundanceOut.hpp"
#include "SundanceTabs.hpp"
#include "SundanceBasisFamily.hpp"
#include "SundanceHomogeneousDOFMap.hpp"
#include "SundanceMaximalCellFilter.hpp"
#include "SundanceCellFilter.hpp"
#include "SundanceCellSet.hpp"
#include "Teuchos_Time.hpp"
#include "Teuchos_TimeMonitor.hpp"


using namespace SundanceStdFwk;
using namespace SundanceStdFwk::Internal;
using namespace SundanceCore;
using namespace SundanceCore::Internal;
using namespace SundanceStdMesh;
using namespace SundanceStdMesh::Internal;
using namespace SundanceUtils;
using namespace Teuchos;

static Time& DOFBuilderCtorTimer() 
{
  static RefCountPtr<Time> rtn 
    = TimeMonitor::getNewTimer("DOF map building"); 
  return *rtn;
}

DOFMapBuilder::DOFMapBuilder(const Mesh& mesh, 
                             const RefCountPtr<EquationSet>& eqn)
  : mesh_(mesh),
    eqn_(eqn),
    rowMap_(),
    colMap_(),
    isBCRow_(rcp(new Array<int>()))
{
  TimeMonitor timer(DOFBuilderCtorTimer());
  init();
}

void DOFMapBuilder::init()
{
  /* If the lists of test and unknown functions have the same basis families
   * in the same order, then the row and column spaces can share a single DOF
   * map. */
  if (isSymmetric())
    {
      /* if every test function is defined on the maximal cell set, and if
       * all test functions share a common basis, then we 
       * can build a homogeneous DOF map. */
      if (testsAreHomogeneous() && testsAreOmnipresent())
        {
          Expr test0 = eqn_->testFunc(0);
          BasisFamily basis0 = BasisFamily::getBasis(test0);
          rowMap_ = rcp(new HomogeneousDOFMap(mesh_, basis0, 
                                              eqn_->numTests()));
          colMap_ = rowMap_;
        }
      else
        {
          SUNDANCE_ERROR("DOFMapBuilder::init() non-homogeneous test function spaces not yet supported");
        }
    }
  else
    {
      /* if every test function is defined on the maximal cell set, and if
       * all test functions share a common basis, then we 
       * can build a homogeneous DOF map. */
      if (testsAreHomogeneous() && testsAreOmnipresent())
        {
          Expr test0 = eqn_->testFunc(0);
          BasisFamily basis0 = BasisFamily::getBasis(test0);
          rowMap_ = rcp(new HomogeneousDOFMap(mesh_, basis0,  
                                              eqn_->numTests()));
        }
      else
        {
          SUNDANCE_ERROR("DOFMapBuilder::init() non-homogeneous test function spaces not yet supported");
        }

      /* if every unk function is defined on the maximal cell set, and if
       * all unk functions share a common basis, then we 
       * can build a homogeneous DOF map. */
      if (unksAreHomogeneous() && unksAreOmnipresent())
        {
          Expr unk0 = eqn_->unkFunc(0);
          BasisFamily basis0 = BasisFamily::getBasis(unk0);
          colMap_ = rcp(new HomogeneousDOFMap(mesh_, basis0, 
                                              eqn_->numUnks()));
        }
      else
        {
          SUNDANCE_ERROR("DOFMapBuilder::init() non-homogeneous unknown function spaces not yet supported");
        }
    }
  markBCRows();
}

Array<BasisFamily> DOFMapBuilder::testBasisArray() const 
{
  Array<BasisFamily> rtn;
  for (int i=0; i<eqn_->numTests(); i++) 
    {
      rtn.append(BasisFamily::getBasis(eqn_->testFunc(i)));
    }
  return rtn;
}

Array<BasisFamily> DOFMapBuilder::unkBasisArray() const 
{
  Array<BasisFamily> rtn;
  for (int i=0; i<eqn_->numUnks(); i++) 
    {
      rtn.append(BasisFamily::getBasis(eqn_->unkFunc(i)));
    }
  return rtn;
}
  
bool DOFMapBuilder::unksAreHomogeneous() const 
{
  if (eqn_->numUnks() > 1)
    {
      BasisFamily basis0 = BasisFamily::getBasis(eqn_->unkFunc(0));
      for (int i=1; i<eqn_->numUnks(); i++) 
        {
          BasisFamily basis = BasisFamily::getBasis(eqn_->unkFunc(i));
          if (!(basis == basis0)) return false;
        }
    }
  return true;
}

bool DOFMapBuilder::testsAreHomogeneous() const 
{
  if (eqn_->numTests() > 1)
    {
      BasisFamily basis0 = BasisFamily::getBasis(eqn_->testFunc(0));
      for (int i=1; i<eqn_->numTests(); i++) 
        {
          BasisFamily basis = BasisFamily::getBasis(eqn_->testFunc(i));
          if (!(basis == basis0)) return false;
        }
    }
  return true;
}

bool DOFMapBuilder::unksAreOmnipresent() const
{
  for (int r=0; r<eqn_->numRegions(); r++)
    {
      if (regionIsMaximal(r))
        {
          if (eqn_->unksOnRegion(r).size() == eqn_->numUnks()) return true;
          else return false;
        }
    }
  return false;
}

bool DOFMapBuilder::testsAreOmnipresent() const
{
  for (int r=0; r<eqn_->numRegions(); r++)
    {
      if (regionIsMaximal(r))
        {
          if (eqn_->testsOnRegion(r).size() == eqn_->numTests()) return true;
          else return false;
        }
    }
  return false;
}


bool DOFMapBuilder::isSymmetric() const 
{
  if (eqn_->numTests() != eqn_->numUnks()) return false;

  for (int i=0; i<eqn_->numTests(); i++) 
    {
      BasisFamily basis1 = BasisFamily::getBasis(eqn_->testFunc(i));
      BasisFamily basis2 = BasisFamily::getBasis(eqn_->unkFunc(i));
      if (!(basis1 == basis2)) return false;
    }
  return true;
}

bool DOFMapBuilder::regionIsMaximal(int r) const 
{
  const CellFilterStub* reg = eqn_->region(r).get();
  return (dynamic_cast<const MaximalCellFilter*>(reg) != 0);
}

void DOFMapBuilder::markBCRows()
{
  isBCRow_->resize(rowMap_->numLocalDOFs());
  Array<int> dofs;
  Array<int> cellLID;

  for (int r=0; r<eqn_->numRegions(); r++)
    {
      if (!eqn_->isBCRegion(r)) continue;

      /* find the cells in this region */
      CellFilter region = eqn_->region(r);
      int dim = region.dimension(mesh_);
      CellSet cells = region.getCells(mesh_);
      cellLID.resize(0);
      for (CellIterator c=cells.begin(); c != cells.end(); c++)
        {
          cellLID.append(*c);
        }
      int nTestNodes;
      /* find the functions that appear in BCs on this region */
      const Set<int>& bcFuncs = eqn_->bcTestsOnRegion(r);
      Array<int> bcFuncID = bcFuncs.elements();
      for (int f=0; f<bcFuncID.size(); f++) 
        {
          bcFuncID[f] = eqn_->reducedTestID(bcFuncID[f]);
        }

      rowMap_->getDOFsForCellBatch(dim, cellLID, bcFuncID, dofs, nTestNodes);
      int offset = rowMap_->lowestLocalDOF();
      for (int n=0; n<dofs.size(); n++) (*isBCRow_)[dofs[n]-offset]=true;
    }
}
