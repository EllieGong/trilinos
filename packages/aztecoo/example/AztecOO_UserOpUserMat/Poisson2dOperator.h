
//@HEADER
// ***********************************************************************
// 
//        AztecOO: An Object-Oriented Aztec Linear Solver Package 
//                 Copyright (2002) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
//@HEADER

#ifndef _POISSON2DOPERATOR_H_
#define _POISSON2DOPERATOR_H_

class Epetra_MultiVector;
class Epetra_CrsMatrix;
class Epetra_Map;
class Epetra_Import;
class Epetra_BlockMap;
class Epetra_Comm;
#include "Epetra_Operator.h"

//! Poisson2dOperator: A sample implementation of the Epetra_Operator class.
/*! The Poisson2dOperator class is a class that implements Epetra_Operator for a 5-point Poisson stencil
    operator.
*/    

class Poisson2dOperator: public virtual Epetra_Operator {
      
 public:

  //@{ \name Constructors.
    //! Builds a 2 dimensional Poisson operator for a nx by ny grid, assuming zero Dirichlet BCs.
  /*! Build a 2 D Poisson operator.  Split the y-dimension across the processor space.
    \param In nx - Number of gridpoints in the x direction.
    \param In ny - Number of gridpoints in the y direction.
  */
  Poisson2dOperator(int nx, int ny, const Epetra_Comm & comm);
  //@{ \name Destructor.
    //! Destructor
  ~Poisson2dOperator();
  //@}
  
  //@{ \name Atribute set methods.

    //! If set true, transpose of this operator will be applied.
    /*! This flag allows the transpose of the given operator to be used implicitly.  Setting this flag
        affects only the Apply() and ApplyInverse() methods.  If the implementation of this interface 
	does not support transpose use, this method should return a value of -1.
      
    \param In
	   UseTranspose -If true, multiply by the transpose of operator, otherwise just use operator.

    \return Integer error code, set to 0 if successful.  Set to -1 if this implementation does not support transpose.
  */
  int SetUseTranspose(bool UseTranspose){useTranspose_ = UseTranspose; return(0);};
  //@}
  
  //@{ \name Mathematical functions.

    //! Returns the result of a Poisson2dOperator applied to a Epetra_MultiVector X in Y.
    /*! 
    \param In
	   X - A Epetra_MultiVector of dimension NumVectors to multiply with matrix.
    \param Out
	   Y -A Epetra_MultiVector of dimension NumVectors containing result.

    \return Integer error code, set to 0 if successful.
  */
  int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

  //! Returns the result of a Poisson2dOperator inverse applied to an Epetra_MultiVector X in Y.
  /*! 
    \param In
    X - A Epetra_MultiVector of dimension NumVectors to solve for.
    \param Out
    Y -A Epetra_MultiVector of dimension NumVectors containing result.
    
    \return Integer error code, set to 0 if successful.
    
    \warning In order to work with AztecOO, any implementation of this method must 
    support the case where X and Y are the same object.
  */
  int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const {return(Apply(X,Y));};
  
  //! Returns the infinity norm of the global matrix.
  /* Returns the quantity \f$ \| A \|_\infty\f$ such that
     \f[\| A \|_\infty = \max_{1\lei\lem} \sum_{j=1}^n |a_{ij}| \f].
     
     \warning This method must not be called unless HasNormInf() returns true.
  */ 
  double NormInf() const {return(8.0);};
  //@}
  
  //@{ \name Atribute access functions

  //! Returns a character string describing the operator
  const char * Label() const{return(Label_);};
  
  //! Returns the current UseTranspose setting.
  bool UseTranspose() const {return(useTranspose_);};
  
  //! Returns true if the \e this object can provide an approximate Inf-norm, false otherwise.
  bool HasNormInf() const{return(true);};
  
  //! Returns a pointer to the Epetra_Comm communicator associated with this operator.
  const Epetra_Comm & Comm() const{return(comm_);};
  
  //! Returns the Epetra_Map object associated with the domain of this operator.
  const Epetra_Map & OperatorDomainMap() const {return(*map_);};
  
  //! Returns the Epetra_Map object associated with the range of this operator.
  const Epetra_Map & OperatorRangeMap() const {return(*map_);};
  //@}
  
  //@{ \name Approximate matrix generators
  //! Generate a tridiagonal approximation to the 5-point Poisson as an Epetra_CrsMatrix

  Epetra_CrsMatrix * GeneratePrecMatrix() const;
  //@}


  int nx_, ny_, myny_;
  bool useTranspose_;
  const Epetra_Comm & comm_;
  Epetra_Map * map_;
  int numImports_;
  int * importIDs_;
  Epetra_Map * importMap_;
  Epetra_Import * importer_;
  mutable Epetra_MultiVector * importX_;
  char * Label_;
};

#endif /* _POISSON2DOPERATOR_H_ */
