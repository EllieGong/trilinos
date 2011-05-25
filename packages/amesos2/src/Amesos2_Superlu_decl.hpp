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
  \file   Amesos2_Superlu_decl.hpp
  \author Eric Bavier <etbavier@sandia.gov>
  \date   Thu Jul  8 22:52:34 2010

  \brief  Amesos2 Superlu declarations.
*/


#ifndef AMESOS2_SUPERLU_DECL_HPP
#define AMESOS2_SUPERLU_DECL_HPP

#include "Amesos2_Solver.hpp"
#include "Amesos2_Superlu_MatrixHelper.hpp"
#include "Amesos2_Superlu_FunctionMap.hpp"
#include "Amesos2_Superlu_TypeMap.hpp"


namespace Amesos {


/** \brief Amesos2 interface to the SuperLU package.
 *
 * \section slu_options Supported Options
 *
 * Currently, the following parameters/options are recognized:
 *
 * <ul>
 *   <li> \c "Trans" : { \c "NOTRANS" | \c "TRANS" |
 *     \c "CONJ" }.  Will also recognize the \c "Transpose" : { \c true
 *     | \c false } option which is equivalent to \c "TRANS" and
 *     \c "NOTRANS" , respectively.</li>
 *   <li> \c "Equil" : { \c "YES" | \c "NO" } or, equivalently, { \c true | \c false }.
 *     Specifies whether the solver to equilibrate the matrix before solving.</li>
 *   <li> \c "IterRefine" : { \c "NO" | \c "SINGLE" | \c "DOUBLE" | \c "EXTRA"
 *     }. Specifies whether to perform iterative refinement, and in
 *     what precision to compute the residual.</li>
 *   <li> \c "SymmetricMode" : { \c "Yes" | \c "NO" } or, equivalently,
 *     { \c true | \c false }.
 *   <li> \c "DiagPivotThresh" : \c double value. Specifies the threshold
 *     used for a diagonal to be considered an acceptable pivot.</li>
 *   <li> \c "ColPerm" which takes one of the following:
 *     <ul>
 *     <li> \c "NATURAL" : natural ordering.</li>
 *     <li> \c "MMD_AT_PLUS_A" : minimum degree ordering on the structure of
 *       \f$ A^T + A\f$ .</li>
 *     <li> \c "MMD_ATA" : minimum degree ordering on the structure of
 *       \f$ A T A \f$ .</li>
 *     <li> \c "COLAMD" : approximate minimum degree column ordering.
 *       (default)</li>
 *     <li> \c "MY_PERMC" : use the ordering given in the "perm_c" parameter
 *       given by the user.  The value of the "perm_c" parameter should be a
 *       Teuchos::Array<int> with length equal to the number of global columns.
 *       </li>
 *     </ul>
 *   <li> \c "perm_c" : a Teuchos::Array<int> with length equal to the number
 *     of global columns.  Will assume <tt> ColPerm = MY_PERMC</tt>.</li>
 * </ul>
 */
template <class Matrix,
          class Vector>
class Superlu : public Solver<Amesos::Superlu, Matrix, Vector>
{
  friend class Solver<Amesos::Superlu,Matrix,Vector>; // Give our base access
                                                      // to our private
                                                      // implementation funcs
public:

  /// Name of this solver interface.
  static const char* name;	// declaration. Initialization outside.

  typedef Superlu<Matrix,Vector>                                   solver_type;
  typedef Matrix                                                   matrix_type;
  typedef Vector                                                   vector_type;
  typedef typename MatrixAdapter<matrix_type>::scalar_type         scalar_type;
  typedef typename MatrixAdapter<matrix_type>::local_ordinal_type  local_ordinal_type;
  typedef typename MatrixAdapter<matrix_type>::global_ordinal_type global_ordinal_type;
  typedef typename MatrixAdapter<matrix_type>::global_size_type    global_size_type;
  typedef typename MatrixAdapter<matrix_type>::node_type           node_type;

  /// \name Constructor/Destructor methods
  //@{

  /**
   * \brief Initialize from Teuchos::RCP.
   *
   * \warning Should not be called directly!  Use instead
   * Amesos::Factory::create() to initialize a Superlu interface.
   */
  Superlu(
    Teuchos::RCP<Matrix> A,
    Teuchos::RCP<Vector> X,
    Teuchos::RCP<Vector> B);


  /// Destructor
  ~Superlu( );

  //@}

private:

  /**
   * \brief Performs pre-ordering on the matrix to increase efficiency.
   *
   * Superlu does not support pre-ordering, so this method does nothing.
   */
  int preOrdering_impl();


  /**
   * \brief Perform symbolic factorization of the matrix using Superlu.
   *
   * Called first in the sequence before numericFactorization.
   *
   * \throw std::runtime_error Superlu is not able to factor the matrix.
   */
  int symbolicFactorization_impl();


  /**
   * \brief Superlu specific numeric factorization
   *
   * \throw std::runtime_error Superlu is not able to factor the matrix
   */
  int numericFactorization_impl();


  /**
   * \brief Superlu specific solve.
   *
   * Uses the symbolic and numeric factorizations, along with the RHS vector
   * \c B to solve the sparse system of equations.
   *
   * \throw std::runtime_error Superlu is not able to solve the system.
   *
   * \callgraph
   */
  int solve_impl();


  /**
   * \brief Determines whether the shape of the matrix is OK for this solver.
   */
  bool matrixShapeOK_impl() const;


  /**
   * This method is hooked in by our Amesos::Solver parent class, which
   * handles the status and control methods, and this method handles
   * solver-specific parameters.
   *
   * See also: \ref slu_options
   */
  void setParameters_impl(
    const Teuchos::RCP<Teuchos::ParameterList> & parameterList );


  /**
   * Hooked in by Amesos::Solver parent class.
   *
   * \return a const Teuchos::ParameterList of all valid parameters for this
   * solver.
   */
  Teuchos::RCP<const Teuchos::ParameterList> getValidParameters_impl() const;


  typedef typename TypeMap<Amesos::Superlu,scalar_type>::magnitude_type magnitude_type;

  // struct holds all data necessary to make a superlu factorization or solve call
  struct SLUData {
    SLU::SuperMatrix A, B, X, L, U;
#ifdef USE_DGSTRF
    SLU::SuperMatrix AC;
#endif
    SLU::superlu_options_t options;
    SLU::mem_usage_t mem_usage;
    SLU::SuperLUStat_t stat;

    Teuchos::Array<magnitude_type> berr; ///< backward error bounds
    Teuchos::Array<magnitude_type> ferr; ///<  forward error bounds
    Teuchos::Array<int> perm_r;
    Teuchos::Array<int> perm_c;
    Teuchos::Array<int> etree;
    Teuchos::Array<magnitude_type> R;
    Teuchos::Array<magnitude_type> C;

    char equed;                  // Flags whether to equilibrate the matrix
                                 // before solve
    int relax;
    int panel_size;
  } data_;

  // The following Arrays are persisting storage arrays for A, X, and B
  /// Stores the values of the nonzero entries for SuperLU
  Teuchos::Array<typename TypeMap<Amesos::Superlu,scalar_type>::type> nzvals_;
  /// Stores the location in \c Ai_ and Aval_ that starts row j
  Teuchos::Array<int> rowind_;
  /// Stores the row indices of the nonzero entries
  Teuchos::Array<int> colptr_;
  /// Persisting 1D store for X
  Teuchos::Array<typename TypeMap<Amesos::Superlu,scalar_type>::type> xvals_;  int ldx_;
  /// Persisting 1D store for B
  Teuchos::Array<typename TypeMap<Amesos::Superlu,scalar_type>::type> bvals_;  int ldb_;

  /* Note: In the above, must use "Amesos::Superlu" rather than "Superlu"
   * because otherwise the compiler references the specialized type of the
   * class, and not the templated type that is required for Amesos::TypeMap
   */

  /* SuperLU can accept input in either compressed-row or compressed-column
   * storage.  We will store and pass matrices in *compressed-row* format
   * because that is the format Amesos used.
   */

  /// Has factorization been performed yet?
  bool factorizationDone_;

};				// End class Superlu


} // end namespace Amesos

#endif	// AMESOS2_SUPERLU_DECL_HPP
