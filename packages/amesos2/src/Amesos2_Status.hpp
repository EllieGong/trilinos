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
  \file   Amesos2_Status.hpp
  \author Eric T Bavier <etbavier@sandia.gov>
  \date   Thu Jan 14 08:52:04 2010

  \brief  Container class for status variables.
*/

#ifndef AMESOS2_STATUS_HPP
#define AMESOS2_STATUS_HPP

#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_Comm.hpp>

namespace Amesos2 {


class Status {
public:
  /// Default constructor.
  Status()
    // , computeVectorNorms_(false)
    // , computeTrueResidual_(false)
      // Will use the Teuchos::VerboseObject for verbosity settings
    : verbose_(0)
    , debug_(0)

    , numPreOrder_(0)
    , numSymbolicFact_(0)
    , numNumericFact_(0)
    , numSolve_(0)

    , preOrderingDone_(false)
    , symbolicFactorizationDone_(false)
    , numericFactorizationDone_(false)

    , myPID_(0)
    , root_(true)
    , numProcs_(1)
    { }


  Status(const Teuchos::RCP<const Teuchos::Comm<int> > comm)
    // , computeVectorNorms_(false)
    // , computeTrueResidual_(false)
      // Will use the Teuchos::VerboseObject for verbosity settings
    : verbose_(0)
    , debug_(0)

    , numPreOrder_(0)
    , numSymbolicFact_(0)
    , numNumericFact_(0)
    , numSolve_(0)

    , preOrderingDone_(false)
    , symbolicFactorizationDone_(false)
    , numericFactorizationDone_(false)

    , myPID_(comm->getRank())
    , root_( myPID_ == 0 )
    , numProcs_(comm->getSize())
    { }


  /// Default destructor.
  ~Status() { };


  /**
   * \brief Set various status parameters.
   *
   * Updates internal variables from the given parameterList.
   */
  void setStatusParameters(
    const Teuchos::RCP<Teuchos::ParameterList> & parameterList );

  // /// If \c true, prints the norms of X and B in solve().
  // bool computeVectorNorms_;

  // /// If \c true, computes the true residual in solve().
  // bool computeTrueResidual_;


  /// Toggles the output level.
  int verbose_;

  /// Sets the level of debug_ output
  int debug_;

  /// Number of pre-ordering phases
  mutable int numPreOrder_;

  /// Number of symbolic factorization phases.
  mutable int numSymbolicFact_;

  /// Number of numeric factorization phases.
  mutable int numNumericFact_;

  /// Number of solves.
  mutable int numSolve_;

  /// If \c true, preOrdering() has been successfully called.
  mutable bool preOrderingDone_;

  /// If \c true, symbolicFactorization() has been successfully called.
  mutable bool symbolicFactorizationDone_;

  /// If \c true, numericFactorization() has been successfully called.
  mutable bool numericFactorizationDone_;


  /// My process ID in this MPI communicator
  int myPID_;

  /// Indicates whether this process is the root process
  bool root_;

  /// The number of processors in this MPI communicator
  int numProcs_;

};                              // end class Amesos2::Status


} // end namespace Amesos2

#endif  // AMESOS2_STATUS_HPP
