#include "PB_JacobiPreconditionerFactory.hpp"

using Teuchos::rcp;

namespace PB {

JacobiPreconditionerFactory::JacobiPreconditionerFactory(const LinearOp & invD0,const LinearOp & invD1)
      : invOpsStrategy_(rcp(new StaticInvDiagStrategy(invD0,invD1)))
{ }

JacobiPreconditionerFactory::JacobiPreconditionerFactory(const RCP<const BlockInvDiagonalStrategy> & strategy)
         : invOpsStrategy_(strategy)
{ }

LinearOp JacobiPreconditionerFactory::buildPreconditionerOperator(BlockedLinearOp & blo) const
{
   int rows = blo->productRange()->numBlocks();
   int cols = blo->productDomain()->numBlocks();
 
   TEUCHOS_ASSERT(rows==cols);
   TEUCHOS_ASSERT(rows==invOpsStrategy_->numDiagonalBlocks());

   // get diagonal blocks
   const std::vector<Teuchos::RCP<const Thyra::LinearOpBase<double> >  > & invDiag
         = invOpsStrategy_->getInvD(blo);

   // create a blocked linear operator
   BlockedLinearOp precond = createNewBlockedOp();

   // start filling the blocked operator
   precond->beginBlockFill(rows,rows); // this is assuming the matrix is square

   // build blocked diagonal matrix
   for(int i=0;i<rows;i++)
      precond->setBlock(i,i,invDiag[i]);
   
   precond->endBlockFill();
   // done filling the blocked operator
   
   return precond; 
}

} // end namspace PB