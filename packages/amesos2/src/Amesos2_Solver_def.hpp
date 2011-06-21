// @HEADER
//
// ***********************************************************************
//
//           Amesos2: Templated Direct Sparse Solver Package
//                  Copyright 2010 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
//
// @HEADER

/**
  \file   Amesos2_Solver_def.hpp
  \class  Amesos::Solver
  \author Eric T Bavier <etbavier@sandia.gov>
  \date   Thu May 27 14:02:35 CDT 2010

  \brief  Templated class for Amesos2 solvers.  Definition.
*/

#ifndef AMESOS2_SOLVER_DEF_HPP
#define AMESOS2_SOLVER_DEF_HPP


namespace Amesos {


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
Solver<ConcreteSolver,Matrix,Vector>::Solver(
  Teuchos::RCP<Matrix> A,
  Teuchos::RCP<Vector> X,
  Teuchos::RCP<Vector> B )
  : matrixA_(createMatrixAdapter<Matrix>(A))
  , multiVecX_(new MultiVecAdapter<Vector>(X))
  , multiVecB_(new MultiVecAdapter<Vector>(B))
  , globalNumNonZeros_(matrixA_->getGlobalNNZ())
  , status_(matrixA_->getComm())
{
  globalNumRows_     = matrixA_->getGlobalNumRows();
  globalNumCols_     = matrixA_->getGlobalNumCols();
  globalNumNonZeros_ = matrixA_->getGlobalNNZ();

  TEST_FOR_EXCEPTION(
    !matrixShapeOK(),
    std::invalid_argument,
    "Matrix shape inappropriate for this solver");

  TEST_FOR_EXCEPTION(
    multiVecX_->getGlobalLength() != matrixA_->getGlobalNumCols(),
    std::invalid_argument,
    "LHS MultiVector must have length equal to the number of global columns in A");

  TEST_FOR_EXCEPTION(
    multiVecB_->getGlobalLength() != matrixA_->getGlobalNumRows(),
    std::invalid_argument,
    "RHS MultiVector must have length equal to the number of global rows in A");

  TEST_FOR_EXCEPTION(
    multiVecX_->getGlobalNumVectors() != multiVecB_->getGlobalNumVectors(),
    std::invalid_argument,
    "LHS and RHS MultiVectors must have the same number of vectors");
}


/// Destructor
template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
Solver<ConcreteSolver,Matrix,Vector>::~Solver( )
{
  // TODO: The below code is still dependent on there being some
  // kind of status and control code available to Amesos2::Solver,
  // either in the form of a shared library or private inheritance
  // of classes that provide that functionality.

  // print out some information if required by the user
  // if ((control_.verbose_ && control_.printTiming_) || control_.verbose_ == 2) printTiming();
  // if ((control_.verbose_ && control_.printStatus_) || control_.verbose_ == 2) printStatus();

}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
SolverBase&
Solver<ConcreteSolver,Matrix,Vector>::preOrdering()
{
  Teuchos::TimeMonitor LocalTimer1(timers_.totalTime_);
  static_cast<solver_type*>(this)->preOrdering_impl();
  ++status_.numPreOrder_;
  status_.preOrderingDone_ = true;
  status_.symbolicFactorizationDone_ = false;
  status_.numericFactorizationDone_  = false;


  return *this;
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
SolverBase&
Solver<ConcreteSolver,Matrix,Vector>::symbolicFactorization()
{
  Teuchos::TimeMonitor LocalTimer1(timers_.totalTime_);

  if( !preOrderingDone() ){
    preOrdering();
  }

  static_cast<solver_type*>(this)->symbolicFactorization_impl();
  ++status_.numSymbolicFact_;
  status_.symbolicFactorizationDone_ = true;
  status_.numericFactorizationDone_  = false;

  return *this;
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
SolverBase&
Solver<ConcreteSolver,Matrix,Vector>::numericFactorization()
{
  Teuchos::TimeMonitor LocalTimer1(timers_.totalTime_);

  if( !symbolicFactorizationDone() ){
    symbolicFactorization();
  }

  static_cast<solver_type*>(this)->numericFactorization_impl();
  ++status_.numNumericFact_;
  status_.numericFactorizationDone_ = true;

  return *this;
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
void
Solver<ConcreteSolver,Matrix,Vector>::solve()
{
  Teuchos::TimeMonitor LocalTimer1(timers_.totalTime_);

  if( !numericFactorizationDone() ){
    numericFactorization();
  }

  static_cast<solver_type*>(this)->solve_impl();
  ++status_.numSolve_;
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
bool
Solver<ConcreteSolver,Matrix,Vector>::matrixShapeOK()
{
  Teuchos::TimeMonitor LocalTimer1(timers_.totalTime_);
  return( static_cast<solver_type*>(this)->matrixShapeOK_impl() );
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
SolverBase&
Solver<ConcreteSolver,Matrix,Vector>::setParameters(
  const Teuchos::RCP<Teuchos::ParameterList> & parameterList )
{
  Teuchos::TimeMonitor LocalTimer1(timers_.totalTime_);

  // Do everything here that is for generic status and control parameters
  control_.setControlParameters(parameterList);
  status_.setStatusParameters(parameterList);

  // Finally, hook to the implementation's parameter list parser
  // First check if there is a dedicated sublist for this solver and use that if there is
  if( parameterList->isSublist(name()) ){
    static_cast<solver_type*>(this)->setParameters_impl(Teuchos::sublist(parameterList, name()));
  } else {
    // Just see if there is anything in the parent sublist that the solver recognizes
    static_cast<solver_type*>(this)->setParameters_impl(parameterList);
  }

  return *this;
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
Teuchos::RCP<const Teuchos::ParameterList>
Solver<ConcreteSolver,Matrix,Vector>::getValidParameters() const
{
  Teuchos::TimeMonitor LocalTimer1( const_cast<Teuchos::Time&>(timers_.totalTime_) );

  using Teuchos::ParameterList;
  using Teuchos::RCP;
  using Teuchos::rcp;

  RCP<ParameterList> control_params = rcp(new ParameterList());
  control_params->set("Transpose",false);
  control_params->set("AddToDiag","");
  control_params->set("AddZeroToDiag",false);
  control_params->set("MatrixProperty","general");
  control_params->set("ScaleMethod",0);

  Teuchos::RCP<ParameterList> status_params = rcp(new ParameterList());
  status_params->set("PrintTiming",false);
  status_params->set("PrintStatus",false);
  status_params->set("ComputeVectorNorms",false);
  status_params->set("ComputeTrueResidual",false);

  RCP<const ParameterList>
    solver_params = static_cast<const solver_type*>(this)->getValidParameters_impl();

  RCP<ParameterList> all_params = rcp(new ParameterList());
  all_params->setParameters(*control_params);
  all_params->setParameters(*status_params);
  all_params->set(name(), *solver_params);

  return all_params;
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
std::string
Solver<ConcreteSolver,Matrix,Vector>::description() const
{
  std::ostringstream oss;
  oss << name() << " solver interface";
  return oss.str();
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
void
Solver<ConcreteSolver,Matrix,Vector>::describe(
  Teuchos::FancyOStream &out,
  const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const
{
  if( matrixA_.is_null() || (status_.myPID_ != 0) ){ return; }
  using std::endl;
  using std::setw;
  using Teuchos::VERB_DEFAULT;
  using Teuchos::VERB_NONE;
  using Teuchos::VERB_LOW;
  using Teuchos::VERB_MEDIUM;
  using Teuchos::VERB_HIGH;
  using Teuchos::VERB_EXTREME;
  Teuchos::EVerbosityLevel vl = verbLevel;
  if (vl == VERB_DEFAULT) vl = VERB_LOW;
  Teuchos::RCP<const Teuchos::Comm<int> > comm = this->getComm();
  size_t width = 1;
  for( size_t dec = 10; dec < globalNumRows_; dec *= 10 ) {
    ++width;
  }
  width = std::max<size_t>(width,11) + 2;
  Teuchos::OSTab tab(out);
  //    none: print nothing
  //     low: print O(1) info from node 0
  //  medium: print O(P) info, num entries per node
  //    high: print O(N) info, num entries per row
  // extreme: print O(NNZ) info: print indices and values
  //
  // for medium and higher, print constituent objects at specified verbLevel
  if( vl != VERB_NONE ) {
    std::string p = name();
    Util::printLine(out);
    out << this->description() << std::endl << std::endl;

    out << p << "Matrix has " << globalNumRows_ << "rows"
        << " and " << globalNumNonZeros_ << "nonzeros"
        << std::endl;
    if( vl == VERB_MEDIUM || vl == VERB_HIGH || vl == VERB_EXTREME ){
      out << p << "Nonzero elements per row = "
          << double(globalNumNonZeros_)/globalNumRows_
          << std::endl;
      out << p << "Percentage of nonzero elements = "
          << 100.0 * globalNumNonZeros_ / pow(double(globalNumRows_),double(2.0))
          << std::endl;
    }
    if( vl == VERB_HIGH || vl == VERB_EXTREME ){
      out << p << "Use transpose = " << control_.useTranspose_
          << std::endl;
    }
    if ( vl == VERB_EXTREME ){
      printTiming(out,vl);
    }
    Util::printLine(out);
  }
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
void
Solver<ConcreteSolver,Matrix,Vector>::printTiming(
  Teuchos::FancyOStream &out,
  const Teuchos::EVerbosityLevel verbLevel) const
{
  if( matrixA_.is_null() || (status_.myPID_ != 0) ){ return; }

  double symTime  = timers_.symFactTime_.totalElapsedTime();
  double numTime  = timers_.numFactTime_.totalElapsedTime();
  double solTime  = timers_.solveTime_.totalElapsedTime();
  double totTime  = timers_.totalTime_.totalElapsedTime();
  double overhead = totTime - (symTime + numTime + solTime);

  std::string p = name() + " : ";
  Util::printLine(out);

  out << p << "Time to convert matrix to implementation format = "
      << timers_.mtxConvTime_.totalElapsedTime() << " (s)"
      << std::endl;
  out << p << "Time to redistribute matrix = "
      << timers_.mtxRedistTime_.totalElapsedTime() << " (s)"
      << std::endl;

  out << p << "Time to convert vectors to implementation format = "
      << timers_.vecConvTime_.totalElapsedTime() << " (s)"
      << std::endl;
  out << p << "Time to redistribute vectors = "
      << timers_.vecRedistTime_.totalElapsedTime() << " (s)"
      << std::endl;

  out << p << "Number of symbolic factorizations = "
      << getNumSymbolicFact()
      << std::endl;
  out << p << "Time for sym fact = "
      << symTime << " (s), avg = "
      << symTime / getNumSymbolicFact() << " (s)"
      << std::endl;

  out << p << "Number of numeric factorizations = "
      << getNumNumericFact()
      << std::endl;
  out << p << "Time for num fact = "
      << numTime << " (s), avg = "
      << numTime / getNumNumericFact() << " (s)"
      << std::endl;

  out << p << "Number of solve phases = "
      << getNumSolve()
      << std::endl;
  out << p << "Time for solve = "
      << solTime << " (s), avg = "
      << solTime / getNumSolve() << " (s)"
      << std::endl;

  out << p << "Total time spent in Amesos2 = "
      << totTime << " (s)"
      << std::endl;
  out << p << "Total time spent in the Amesos2 interface = "
      << overhead << " (s)"
      << std::endl;
  out << p << "  (the above time does not include solver time)"
      << std::endl;
  out << p << "Amesos2 interface time / total time = "
      << overhead / totTime
      << std::endl;
  Util::printLine(out);
}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
void
Solver<ConcreteSolver,Matrix,Vector>::getTiming(
  Teuchos::ParameterList& timingParameterList) const
{}


template <template <class,class> class ConcreteSolver, class Matrix, class Vector >
std::string
Solver<ConcreteSolver,Matrix,Vector>::name() const {
  std::string solverName = solver_type::name;
  return solverName;
}


} // end namespace Amesos

#endif  // AMESOS2_SOLVER_DEF_HPP
