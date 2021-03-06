// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef ROL_PRIMALDUALPRECONDITIONER_H
#define ROL_PRIMALDUALPRECONDITIONER_H

#include "ROL_LinearOperator.hpp"
#include "ROL_Objective.hpp"
#include "ROL_BoundConstraint.hpp"
#include "ROL_Types.hpp"

namespace ROL {

template <class Real>
class PrimalDualPreconditioner : public LinearOperator<Real> {
private:

  const Teuchos::RCP<Objective<Real> > obj_;
  const Teuchos::RCP<BoundConstraint<Real> > con_;
  const Teuchos::RCP<Vector<Real> > x_;
  const Teuchos::RCP<Vector<Real> > xlam_;
  Teuchos::RCP<Vector<Real> > v_;

  const Teuchos::RCP<Secant<Real> > secant_;

  bool useSecant_;

  Real eps_;

public:

  PrimalDualPreconditioner(const Teuchos::RCP<Objective<Real> > &obj, 
                           const Teuchos::RCP<BoundConstraint<Real> > &con, 
                           const Teuchos::RCP<Vector<Real> > &x, 
                           const Teuchos::RCP<Vector<Real> > &xlam ) 
    : obj_(obj), con_(con), x_(x), xlam_(xlam), secant_(Teuchos::null), eps_(0.0) {
    v_ = x_->clone();
    useSecant_ = false;
  }

  PrimalDualPreconditioner(const Teuchos::RCP<Secant<Real> > &secant, 
                           const Teuchos::RCP<Objective<Real> > &obj, 
                           const Teuchos::RCP<BoundConstraint<Real> > &con, 
                           const Teuchos::RCP<Vector<Real> > &x,
                           const Teuchos::RCP<Vector<Real> > &xlam, bool useSecant = true ) 
    : obj_(obj), con_(con), x_(x), xlam_(xlam), secant_(secant), eps_(0.0) {
    v_ = x_->clone();
    useSecant_ = useSecant;
  }

  /** \brief Apply preconditioner.

      This function applies the preconditioner to a vector.
      @param[out]         Hv  is the output vector.
      @param[in]          v   is the input vector.
      @param[in]          tol is a tolerance for inexact Hessian application.
  */
  void apply( Vector<Real> &Hv, const Vector<Real> &v, Real &tol ) const {
    v_->set(v);
    con_->pruneActive(*v_,*xlam_,eps_);
    if ( useSecant_ && secant_ != Teuchos::null ) {
      secant_->applyH(Hv,*v_,*x_);
    }
    else {
      obj_->precond(Hv,*v_,*x_,tol);
    }
    con_->pruneActive(Hv,*xlam_,eps_);
  }

}; // class Preconditioner

} // namespace ROL

#endif
