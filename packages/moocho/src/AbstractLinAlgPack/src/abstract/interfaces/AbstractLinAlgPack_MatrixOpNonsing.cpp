// //////////////////////////////////////////////////////////////////////
// MatrixWithOpNonsingular.cpp
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.

#include <math.h>

#include "AbstractLinAlgPack/src/MatrixWithOpNonsingular.h"
#include "AbstractLinAlgPack/src/VectorSpace.h"
#include "AbstractLinAlgPack/src/LinAlgOpPack.h"
#include "ThrowException.h"

namespace AbstractLinAlgPack {

MatrixWithOpNonsingular::mat_mwons_mut_ptr_t
MatrixWithOpNonsingular::clone_mwons()
{
	return MemMngPack::null;
}

MatrixWithOpNonsingular::mat_mwons_ptr_t
MatrixWithOpNonsingular::clone_mwons() const
{
	return MemMngPack::null;
}

const MatrixWithOpNonsingular::MatNorm
MatrixWithOpNonsingular::calc_cond_num(
	EMatNormType  requested_norm_type
	,bool         allow_replacement
	) const
{
	using BLAS_Cpp::no_trans;
	using BLAS_Cpp::trans;
	using LinAlgOpPack::V_InvMtV;
	const VectorSpace
		&space_cols = this->space_cols(),
		&space_rows = this->space_rows();
	const index_type
		num_cols = space_rows.dim();
	THROW_EXCEPTION(
		!(requested_norm_type == MAT_NORM_1 || requested_norm_type == MAT_NORM_INF), MethodNotImplemented
		,"MatrixWithOp::calc_norm(...): Error, This default implemenation can only "
		"compute the one norm or the infinity norm!"
		);
	//
	// Here we implement Algorithm 2.5 in "Applied Numerical Linear Algebra", Demmel (1997)
	// using the momenclature in the text.  This is applied to the inverse matrix.
	//
	const MatrixWithOpNonsingular
		&B = *this;
	bool
		do_trans = requested_norm_type == MAT_NORM_INF;
	VectorSpace::vec_mut_ptr_t
		x    = (do_trans ? space_rows : space_cols).create_member(1.0/num_cols),
		w    = (do_trans ? space_cols : space_rows).create_member(),
		zeta = (do_trans ? space_cols : space_rows).create_member(),
		z    = (do_trans ? space_rows : space_cols).create_member();
	const index_type max_iter = 5;  // Recommended by Highman 1988, (see Demmel's reference)
	value_type w_nrm = 0.0;
	for( index_type k = 0; k <= max_iter; ++k ) {
		V_InvMtV( w.get(), B, !do_trans ? no_trans : trans, *x );     // w = B*x
		sign( *w, zeta.get() );                                       // zeta = sign(w)
		V_InvMtV( z.get(), B, !do_trans ? trans : no_trans, *zeta );  // z = B'*zeta
		value_type  z_j = 0.0;                                        // max |z(j)| = ||z||inf
		index_type  j   = 0;
		max_abs_ele( *z, &z_j, &j );
		const value_type zTx = dot(*z,*x);                            // z'*x
		w_nrm = w->norm_1();                                        // ||w||1
		if( ::fabs(z_j) <= zTx ) {                                    // Update
			break;
		}
		else {
			*x = 0.0;
			x->set_ele(j,1.0);
		}
	}
	const MatNorm M_nrm = this->calc_norm(requested_norm_type);
	return MatNorm( w_nrm * M_nrm.value ,requested_norm_type );
}

// Overridden from MatrixWithOp

MatrixWithOpNonsingular::mat_mut_ptr_t
MatrixWithOpNonsingular::clone()
{
	return clone_mwons();
}

MatrixWithOpNonsingular::mat_ptr_t
MatrixWithOpNonsingular::clone() const
{
	return clone_mwons();
}

// Overridden from MatrixNonsingular

MatrixWithOpNonsingular::mat_mns_mut_ptr_t
MatrixWithOpNonsingular::clone_mns()
{
	return clone_mwons();
}

MatrixWithOpNonsingular::mat_mns_ptr_t
MatrixWithOpNonsingular::clone_mns() const
{
	return clone_mwons();
}

}	// end namespace AbstractLinAlgPack
