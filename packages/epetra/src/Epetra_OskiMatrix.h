
//@HEADER
/*
************************************************************************

              Epetra: Linear Algebra Services Package 
                Copyright (2001) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.
 
This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact Michael A. Heroux (maherou@sandia.gov) 

************************************************************************
*/
//@HEADER

// Author: Ian Karlin ikarlin@sandia.gov 05-28-2008

#ifndef EPETRA_OSKIMATRIX_H
#define EPETRA_OSKIMATRIX_H

#include "Epetra_OskiMultiVector.h"
#include "Epetra_OskiVector.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_OskiPermutation.h"
#include "Teuchos_ParameterList.hpp"
extern "C" {
  #include <oski/oski.h>
}

class Epetra_OskiVector;
class Epetra_OskiMultiVector;
class Teuchos_ParameterList;

//! Epetra_OskiMatrix: A class for constructing and using OSKI Matrices within Epetra.

/*! The Epetra_OskiMatrix class is a utility for wrapping OSKI matrix calls into Epetra.
    It can convert Epetra matrices to OSKI matrices for use by OSKI functions.  It also
    calls all OSKI functions that access OSKI data and perform calculations on OSKI matrices.

    The calculation kernels to perform matrix-vector and matrix multi-vector calculations are
    provided along with runtime tuning function calls.
*/
 
class Epetra_OskiMatrix: public Epetra_CrsMatrix{
 public:
	//! @name Constructors/Destructor
	//@{
        //! Copy constructor.
        Epetra_OskiMatrix (const Epetra_OskiMatrix& Source); //not in use for now
 
	//! Constructor creates an Epetra_OskiMatrix from an Epetra_CrsMatrix.
	/*! \param Source (In) An Epetra_CrsMatrix that is to be wrapped as an Epetra_OskiMatrix.
	    \param List (In) Any options or data wanted or needed for the conversion.  The
		   actual values that can go here will be listed later depending on what
		   can be gotten from Epetra and what is applicable to our implementation.
		   Examples of parameters will be matrix storage format and whether the
		   indices are zero or one based.
            \return Pointer to an Epetra_OskiMatrix.
	*/
	Epetra_OskiMatrix (const Epetra_CrsMatrix& Source, const Teuchos::ParameterList& List);
	
	//! Destructor	
	virtual ~Epetra_OskiMatrix ();
	//@}

	//! @name Extract/Replace Values
	//@{
	//! Replace current values with this list of entries for a given local row of the matrix.  Warning this could be expensive.
	/*! The reason this function could be expensive is its underlying implementation.  
	    Both the OSKI and Epetra versions of the Matrix must be changed when the matrix 
	    has been permuted.  When this is the case a call must be made to the Epetra
	    ReplaceMyValues and NumEntries calls must be made to a function that changes the
	    OSKI matrix's values one at a time.
	    \param MyRow (In) Row number (in local coordinates) to put elements.
	    \param NumEntries (In) Number of entries.
	    \param Values (In) Values to enter.
	    \param Indices (In) Local column indices corresponding to the values.
       	    \return Integer error code, set to 0 if successful. Note that if the
    		    allocated length of the row has to be expanded, a positive warning code
    		    will be returned.
    	    \pre IndicesAreLocal()==true
    	    \post The given Values at the given Indices have been summed into the
    	   	  entries of MyRow.

	*/
	int ReplaceMyValues(int MyRow, 
			    int NumEntries, 
			    double* Values, 
			    int* Indices);

   	//! Add this list of entries to existing values for a given local row of the matrix.  Warning this could be expensive.
  	/*! The reason this function could be expensive is its underlying implementation.
            Both the OSKI and Epetra versions of the Matrix must be changed when the matrix
            has been permuted.  When this is the case a call must be made to the Epetra
            SumIntoMyValues and NumEntries calls must be made to a function that changes the
            OSKI matrix's values one at a time.
	    \param MyRow - (In) Row number (in local coordinates) to put elements.
    	    \param NumEntries - (In) Number of entries.
    	    \param Values - (In) Values to enter.
    	    \param Indices - (In) Local column indices corresponding to values. 
    	    \return Integer error code, set to 0 if successful. Note that if the
    		    allocated length of the row has to be expanded, a positive warning code
    		    will be returned.
    	    \pre IndicesAreLocal()==true
    	    \post The given Values at the given Indices have been summed into the
    		  entries of MyRow.
	*/
	int SumIntoMyValues(int MyRow, 
			    int NumEntries, 
			    double* Values, 
			    int* Indices);


	//! Returns a copy of the main diagonal in a user-provided vector.
  	/*! \param Diagonal - (Out) Extracted main diagonal.
    	    \return Integer error code, set to 0 if successful and non-zero if not.
    	    \pre Filled()==true
    	    \post Unchanged.
  	*/
	int ExtractDiagonalCopy(Epetra_OskiVector& Diagonal) const;
	
	//! Replaces diagonal values of the matrix with those in the user-provided vector.
  	/*! This routine is meant to allow replacement of {\bf existing} diagonal values.
     	    If a diagonal value does not exist for a given row, the corresponding value in
            the input Epetra_OskiVector will be ignored and the return code will be set to 1.
    	    \param Diagonal - (In) New values to be placed in the main diagonal.
    	    \return Integer error code, set to 0 if successful, set to 1 on the calling processor 
	       	    if one or more diagonal entries not present in matrix.  Other error codes can
		    be returned as well indicating improperly constructed matrices or vectors.
    	    \pre Filled()==true
    	    \post Diagonal values have been replaced with the values of Diagonal.
	*/
	int ReplaceDiagonalValues(const Epetra_OskiVector& Diagonal);
	//@}	

	//! @name Computational methods
	//@{
	//! Performs a matrix vector multiply of y = Alpha*this^TransA*x + Beta*y
	/*! The vectors x and y can be either Epetra_Vectors or Epetra_OskiVectors.
	//! Gets diagonal DiagNum from the matrix such that j - i = DiagNum for all this(i, j) entries.
	//! Gets diagonal DiagNum from the matrix such that j - i = DiagNum for all this(i, j) entries.
	    \param TransA (In) If TransA = TRUE then use the transpose of the matrix in
	           computing the product.
	    \param x (In) The vector the matrix is multiplied by.
	    \param y (In/Out) The vector where the calculation result is stored.
	    \param Alpha (In) A scalar constant used to scale x.
	    \param Beta  (In) A scalar constant used to scale y.
	    \return Integer error code, set to 0 if successful.
            \pre Filled()==true
            \post Unchanged
	*/
  	int Multiply(bool TransA,
	    	     const Epetra_Vector& x, 
		     Epetra_Vector& y,
		     double Alpha = 1.0,
		     double Beta = 0.0) const;
	
	//! Performs a matrix multi-vector multiply of Y = Alpha*this^TransA*X + Beta*Y
	/*! The multi-vectors X and Y can be either Epetra_MultiVectors or Epetra_OskiMultiVectors.
	    \param TransA (In) If Trans = TRUE then use the transpose of the matrix in
	           computing the product.
	    \param X (In) The multi-vector the matrix is multiplied by.
	    \param Y (In/Out) The multi-vector where the calculation result is stored.
	    \param Alpha (In) A scalar constant used to scale X.
	    \param Beta  (In) A scalar constant used to scale Y.
	    \return Integer error code, set to 0 if successful.
	    \pre Filled()==true
	    \post Unchanged
	*/
  	int Multiply(bool TransA,
   	    	     const Epetra_MultiVector& X, 
	    	     Epetra_MultiVector& Y,
		     double Alpha = 1.0,
	    	     double Beta = 0.0) const;
	//@}

	//! @name Tuning
	//@{
	//! Stores the hints in List in the matrix structure.
	/*! \param List (In) A list of hints and options to register along with the matrix
		   used for tuning purposes.  The full list is below for now and will either
		   stay there and/or be moved to the user guide in the future.
	    \return On successful storage of the hint 0 is returned.  On failure an error code
		    is returned.

	    The available hints are as follows within each section only one hint may be set at
	    a time.

	    NO_BLOCKS,
	    SINGLE_BLOCKSIZE    [int r, c],
	    MULTIPLE_BLOCKSIZES [int k, r1, c1, ..., rk, ck]

	    ALIGNED_BLOCKS,
	    UNALIGNED_BLOCKS
	    
	    SYMM_PATTERN,
	    NONSYMM_PATTERN
	    
	    RANDOM_PATTERN,
	    CORRELATED_PATTERN

	    NO_DIAGS,
	    DIAGS
 	*/
	int SetHint(const Teuchos::ParameterList& List);

	
	//! Workload hints for computing a matrix-vector multiply used by OskiTuneMat to optimize the data structure storage and the routine to compute the calculation.
	/*! \param Trans (In) If Trans = 'T' or 't' then the transpose of the matrix will be used in
	           computing the product.  Otherwise this should be set to 'N' or 'n' to denote
		   the transpose is not taken.
	    \param Alpha (In) A scalar constant used to scale InVec.
	    \param InVec (In) The vector the matrix is multiplied by or whether it is a single vector or multi-vector.
	    \param Beta  (In) A scalar constant used to scale OutVec.
	    \param OutVec (In) The vector where the calculation result is stored or whether it is a single vector or multi-vector.
	    \param NumCalls (In) The number of times the operation is called or the tuning level wanted.
	    \param List (In) Used for denoting the use of symbolic vectors for both InVec
		   and OutVec as well as for level of aggressive tuning if either NumCalls not
		   known or to be overridden.  Options are shown below it should be noted that
		   by using these options the associated vector or NumCalls becomes invalid.
	    \return Stores the workload hint in the matrix if the operation is valid.  If the
	       	    operation is not valid an error code is returned.
	    
	    SYMBOLIC_IN_VEC,
	    SYMBOLIC_IN_MULTIVEC
	
	    SYMBOLIC_OUT_VEC,
	    SYMBOLIC_OUT_MULTIVEC

	    ALWAYS_TUNE,
	    ALWAYS_TUNE_AGGRESSIVELY
	*/
	int SetHintMatrixMultiply(bool TransA,
			          double Alpha,
			   	  const Epetra_OskiMultiVector InVec,
			   	  double Beta,
			   	  const Epetra_OskiMultiVector OutVec,
			   	  int NumCalls,
			   	  const Teuchos::ParameterList& List);

	//! Tunes the matrix multiply if its deemed profitable.
	/*! The routine tunes based upon user provided hints if given.  If hints are not given the
	    tuning is performed based on expected future workload for the calculation.
	    \return On success returns a non-negative status code of the transformations 
		    performed.  On failure an error code is returned.
	*/
	int TuneMatrix();
	//@}	

	//! @name Data Structure Transformation Methods
	//@{
	//! Returns 1 if the matrix has been reordered by tuning and 0 if it has not been.
	int IsMatrixTransformed() const;

	//! Returns the transformed version of InMat if InMat has been transformed.  If InMat has not been transformed then the return will equal InMat.
	const Epetra_OskiMatrix& ViewTransformedMat() const;

	//! Returns a read only row/left permutation of the Matrix.
	const Epetra_OskiPermutation& ViewRowPermutation() const;
	
	//! Returns a read only column/right permutation of the Matrix.
	const Epetra_OskiPermutation& ViewColumnPermutation() const;
	
	//! Returns a string holding the transformations performed on the matrix when it was tuned.
	/*! \return Upon success returns a newly-allocated string that stores the 
		    transformations applied to the matrix during tuning.  NULL is returned
		    upon an error.  It is the users responsibility to deallocate the returned
                    string.
	*/
	char* GetMatrixTransforms() const;

	//! Replaces the current data structure of the matrix with the one specified in Transforms.
	/*! If a previously tuned copy of the Matrix existed it is now replaced by one
	    specified in Transforms.
	    \param Transforms (In) A string that holds the transformations to be applied
		   to the matrix.  If Transforms is NULL or the empty string then no
	           changes to the data structure are made.
	    \return If the transformation was successful 0 is returned.  Otherwise an error
		    code is returned.
	*/
	int ApplyMatrixTransforms(const char* Transforms);
	//@}

 protected:

 private:
	const Epetra_CrsMatrix* Epetra_View_;
	oski_matrix_t A_tunable_;
	bool Copy_Created_; 
};
#endif /* EPETRA_OSKIMATRIX_H */
