// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
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
// @HEADER

#ifndef THYRA_VECTOR_BASE_DECL_HPP
#define THYRA_VECTOR_BASE_DECL_HPP

#include "Thyra_OperatorVectorTypes.hpp"
#include "Thyra_MultiVectorBaseDecl.hpp"
#include "RTOpPack_RTOpT.hpp"

namespace Thyra {

/** \brief Abstract interface for finite-dimensional dense vectors.
 *
 * This interface contains a minimal set of operations.  The main
 * feature of this interface is the operation <tt>applyOp()</tt>.
 * Every standard (i.e. BLAS) and nearly every non-standard
 * element-wise operation that can be performed on a set of vectors
 * can be performed efficiently through reduction/transformation
 * operators.  More standard vector operations could be included in
 * this interface and allow for specialized implementations but in
 * general, assuming the sub-vectors are large enough, such
 * implementations would not be significantly faster than those
 * implemented through reduction/transformation operators.  There are
 * some operations however that can not always be efficiently
 * implemented with reduction/transformation operators and a few of
 * these important operations are included in this interface.  The
 * <tt>applyOp()</tt> function allows to client to specify a sub-set
 * of the vector elements to include in reduction/transformation
 * operation.  This greatly increases the generality of this vector
 * interface as vector objects can be used as sub objects in larger
 * composite vectors and sub-views of a vector can be created.
 *
 * <b>Heads Up!</b>
 * There already exists reduction/transformation operator-based
 * implementations of several standard vector operations and
 * some convenience functions that wrap these operators and call
 * <tt>applyOp()</tt> can be found \ref Thyra_VectorStdOps_grp "here".
 *
 * This interface also allows a client to extract a sub-set of
 * elements in an explicit form as non-mutable
 * <tt>RTOpPack::SubVectorT<Scalar></tt> or mutable
 * <tt>RTOpPack::MutableSubVectorT<Scalar></tt> objects using the
 * operations <tt>getSubVector()</tt>.  In general, this is a very bad
 * thing to do and should be avoided at all costs.  However, there are
 * some situations where this is needed and therefore it is supported.
 * The default implementation of these operations use
 * reduction/transformation operators with <tt>applyOp()</tt> in order
 * to extract and set the needed elements and therefore all
 * <tt>%VectorBase</tt> subclasses automatically support these operations
 * (even if it is a bad idea to use them).  <b>Heads Up!</b> Note that
 * client code in general should not directly call the above explicit
 * sub-vector access functions but should use the utility classes
 * <tt>ExplicitVectorView</tt> and <tt>ExplicitMutableVectorView</tt>
 * instead since these are easier an safer in the event that an
 * exception is thrown.
 *
 * In addition to being able to extract an explicit non-mutable and
 * mutable views of some (small?) sub-set of elements, this interface
 * allows a client to set sub-vectors using <tt>setSubVector()</tt>.
 *
 * It is also worth mentioning that that this <tt>%VectorBase</tt>
 * interface class also inherits from <tt>MultiVectorBase</tt> so that
 * every <tt>%VectorBase</tt> object is also a <tt>%MultiVectorBase</tt>
 * object.  This allows any piece of code that accepts
 * <tt>%MultiVectorBase</tt> objects to automatically accept
 * <tt>%VectorBase</tt> objects as well.
 *
 * <b>Notes for subclass developers</b>
 *
 * In order to create a concrete subclass of this interface, only two
 * operations must be overridden: <tt>space()</tt> and
 * <tt>applyOp()</tt>.  Overriding the <tt>space()</tt> operation
 * requires defining a concrete <tt>VectorSpaceBase</tt> class (which has
 * only three pure virtual operations).
 *
 * A subclass should only bother overriding the explicit sub-vector
 * access operations <tt>getSubVector()</tt>,
 * <tt>freeSubVector()</tt>, <tt>commitSubVector()</tt> and
 * <tt>setSubVector()</tt> if profiling shows that these operations
 * are taking too much time.  In most use cases, these default
 * implementations will not impact the overall runtime for a numerical
 * algorithm in any significant way and in most algorithms these
 * functions will not even be called.
 *
 * Note that all of the inherited <tt>OpBase</tt>, <tt>LinearOpBase</tt>
 * and <tt>MultiVectorBase</tt> functions are overridden in this subclass
 * and are given perfectly good implementations.  Therefore, a
 * concrete subclass of <tt>%VectorBase</tt> should not have to override
 * any of these functions.
 *
 * \ingroup Thyra_Op_Vec_fundamental_interfaces_code_grp
 */
template<class Scalar>
class VectorBase : virtual public MultiVectorBase<Scalar>
{
public:

  /** \brief . */
  using MultiVectorBase<Scalar>::describe;
  /** \brief . */
  using MultiVectorBase<Scalar>::applyOp;
  /** \brief . */
  using MultiVectorBase<Scalar>::col;

  /** @name Public pure virtual operations (must be overridden by subclass) */
  //@{

  /** \brief Return a smart pointer to the vector space that this vector belongs to.
   *
   * A return value of <tt>space().get()==NULL</tt> is a flag that
   * <tt>*this</tt> is not fully initialized.
   *
   * If <tt>return.get()!=NULL</tt>, it is required that the object
   * referenced by <tt>*return.get()</tt> must have lifetime that
   * extends past the lifetime of the returned smart pointer object.
   * However, the object referenced by <tt>*return.get()</tt> may
   * change if <tt>*this</tt> modified so this reference should not be
   * maintained for too long.
   *
   * Once more, the client should not expect the <tt>%VectorSpaceBase</tt>
   * object embedded in <tt>return</tt> to be valid past the lifetime
   * of <tt>*this</tt>.
   */
  virtual Teuchos::RefCountPtr< const VectorSpaceBase<Scalar> > space() const = 0;

  /** \brief Apply a reduction/transformation,operation over a set of vectors:
   * <tt>op(op(v[0]...v[nv-1],z[0]...z[nz-1]),(*reduct_obj)) -> z[0]...z[nz-1],(*reduct_obj)</tt>.
   *
   * Preconditions:<ul>
   * <li> <tt>this->space().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * </ul>
   *
   * The vector <tt>this</tt> that this operation is called on is
   * assumed to be one of the vectors in
   * <tt>v[0]...v[nv-1],z[0]...z[nz-1]</tt>.  This operation is generally should
   * not called directly by a client but instead the client should call the
   * nonmember function <tt>Thyra::applyOp()</tt>.
   *
   * See the documentation for the nonmember function <tt>Thyra::applyOp()</tt>
   * for a description of what this operation does.
   */
  virtual void applyOp(
    const RTOpPack::RTOpT<Scalar>    &op
    ,const int                       num_vecs
    ,const VectorBase<Scalar>*       vecs[]
    ,const int                       num_targ_vecs
    ,VectorBase<Scalar>*             targ_vecs[]
    ,RTOpPack::ReductTarget          *reduct_obj
    ,const Index                     first_ele
    ,const Index                     sub_dim
    ,const Index                     global_offset
    ) const = 0;

  //@}

  /** @name Vector cloning */
  //@{

  /** \brief Returns a copy of <tt>*this</tt> vector.
   *
   * This function exists to be consistent with the clone functions
   * <tt>clone()</tt> which creates a <tt>LinearOpBase</tt> object and
   * <tt>clone_mv()</tt> which creates a <tt>MultiVectorBase</tt> objects.
   * However, this function is not really necessary because this capability is
   * already present by using the <tt>VectorSpaceBase</tt> returned from
   * <tt>this->space()</tt>.  The default implementation of this function
   * simply creates a new vector using <tt>createMember(this->space())</tt>
   * and then assigns the contents from <tt>*this</tt>.
   *
   * Subclasses should only consider overriding this function if there they
   * want to be very sophisticated and implement some form of lazy evaluation
   * in case the created object might not actually be modified before it is
   * destroyed.
   */
  Teuchos::RefCountPtr<VectorBase<Scalar> > clone_v() const;

  //@}

  /** @name Explicit sub-vector access */
  //@{

  /** \brief Get a non-mutable explicit view of a sub-vector.
   *
   * @param  rng      [in] The range of the elements to extract the sub-vector view.
   * @param  sub_vec  [in/out] View of the sub-vector.  Prior to the
   *                  first call <tt>sub_vec->set_uninitialized()</tt> must be called first.
   *                  Technically <tt>*sub_vec</tt> owns the memory but this memory can be freed
   *                  only by calling <tt>this->freeSubVector(sub_vec)</tt>.
   *
   * Preconditions:<ul>
   * <li> <tt>this->space().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> [<tt>!rng.full_range()</tt>] <tt>(rng.ubound() <= this->dim()) == true</tt>
   *      (<tt>throw std::out_of_range</tt>)
   * </ul>
   *
   * Postconditions:<ul>
   * <li> <tt>*sub_vec</tt> contains an explicit non-mutable view to the elements
   *      in the range <tt>full_range(rng,1,this->dim())</tt>
   * </ul>
   *
   * This is only a transient view of a sub-vector that is to be
   * immediately used and then released with a call to
   * <tt>freeSubVector()</tt>.
   *
   * Note that calling this operation might require some dynamic
   * memory allocations and temporary memory.  Therefore, it is
   * critical that <tt>this->freeSubVector(sub_vec)</tt> is called
   * to clean up memory and avoid memory leaks after the sub-vector
   * is used.
   *
   * <b>Heads Up!</b> Note that client code in general should not
   * directly call this function but should use the utility class
   * <tt>ExplicitVectorView</tt> which will also take care
   * of calling <tt>freeSubVector()</tt>.
   *
   * If <tt>this->getSubVector(...,sub_vec)</tt> was previously
   * called on <tt>sub_vec</tt> then it may be possible to reuse
   * this memory if it is sufficiently sized.  The user is
   * encouraged to make multiple calls to
   * <tt>this->getSubVector(...,sub_vec)</tt> before
   * <tt>this->freeSubVector(sub_vec)</tt> to finally clean up all
   * of the memory.  Of course, the same <tt>sub_vec</tt> object must
   * be passed to the same vector object for this to work correctly.
   *
   * This operation has a default implementation based on a vector
   * reduction operator class (see <tt>RTOpPack::ROpGetSubVector</tt>)
   * and calls <tt>applyOp()</tt>.  Note that the footprint of the reduction
   * object (both internal and external state) will be
   * O(<tt>rng.size()</tt>).  For serial applications this is fairly
   * reasonable and will not be a major performance penalty.  For
   * parallel applications, however, this is a terrible
   * implementation and must be overridden if <tt>rng.size()</tt> is
   * large at all.  Although, this operation should not even be used in
   * case where the vector is very large.  If a subclass does
   * override this operation, it must also override
   * <tt>freeSubVector()</tt> which has a default implementation
   * which is a companion to this operation's default implementation.
   */
  virtual void getSubVector( const Range1D& rng, RTOpPack::SubVectorT<Scalar>* sub_vec ) const;

  /** \brief Free an explicit view of a sub-vector.
   *
   * @param  sub_vec
   *				[in/out] The memory referred to by <tt>sub_vec->values</tt>
   *				will be released if it was allocated and <tt>*sub_vec</tt>
   *              will be zeroed out using <tt>sub_vec->set_uninitialized()</tt>.
   *
   * Preconditions:<ul>
   * <li> <tt>this->space().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>sub_vec</tt> must have been passed through a call to 
   *      <tt>this->getSubVector(...,sub_vec)</tt>
   * </ul>
    *
   * Postconditions:<ul>
   * <li> See <tt>RTOpPack::SubVectorT::set_uninitialized()</tt> for <tt>sub_vec</tt>
   * </ul>
   *
   * The sub-vector view must have been allocated by <tt>this->getSubVector()</tt> first.
   *
   * This operation has a default implementation which is a companion
   * to the default implementation for the non-<tt>const</tt>
   * version of <tt>getSubVector()</tt>.  If <tt>getSubVector()</tt>
   * is overridden by a subclass then this operation must be overridden
   * also!
   */
  virtual void freeSubVector( RTOpPack::SubVectorT<Scalar>* sub_vec ) const;

  /** \brief Get a mutable explicit view of a sub-vector.
   *
   * @param  rng      [in] The range of the elements to extract the sub-vector view.
   * @param  sub_vec  [in/out] Mutable view of the sub-vector.  Prior to the
   *                  first call <tt>sub_vec->set_uninitialized()</tt> must
   *                  have been called for the correct behavior.  Technically
   *                  <tt>*sub_vec</tt> owns the memory but this memory must be committed
   *                  and freed only by calling <tt>this->commitSubVector(sub_vec)</tt>.
   *
   * Preconditions:<ul>
   * <li> <tt>this->space().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> [<tt>!rng.full_range()</tt>] <tt>(rng.ubound() <= this->dim()) == true</tt>
   *      (throw <tt>std::out_of_range</tt>)
   * </ul>
    *
   * Postconditions:<ul>
   * <li> <tt>*sub_vec</tt> contains an explicit mutable view to the elements
   *      in the range <tt>full_range(rng,1,this->dim())</tt>
   * </ul>
   *
   * This is only a transient view of a sub-vector that is to be
   * immediately used and then committed back with a call to
   * <tt>commitSubVector()</tt>.
   *
   * Note that calling this operation might require some internal
   * allocations and temporary memory.  Therefore, it is critical
   * that <tt>this->commitSubVector(sub_vec)</tt> is called to
   * commit the changed entries and clean up memory and avoid memory
   * leaks after the sub-vector is modified.
   *
   * <b>Heads Up!</b> Note that client code in general should not
   * directly call this function but should use the utility class
   * <tt>ExplicitMutableVectorView</tt> which will also take care
   * of calling <tt>commitSubVector</tt>.
   *
   * If <tt>this->getSubVector(...,sub_vec)</tt> was previously
   * called on <tt>sub_vec</tt> then it may be possible to reuse
   * this memory if it is sufficiently sized.  The user is
   * encouraged to make multiple calls to
   * <tt>this->getSubVector(...,sub_vec)</tt> before
   * <tt>this->commitSubVector(sub_vec)</tt> to finally clean up all
   * of the memory.  Of course the same <tt>sub_vec</tt> object must
   * be passed to the same vector object for this to work correctly.
   *
   * Changes to the underlying sub-vector are not guaranteed to
   * become permanent until <tt>this->getSubVector(...,sub_vec)</tt>
   * is called again, or <tt>this->commitSubVector(sub_vec)</tt> is
   * called.
   *
   * This operation has a default implementation based on a vector
   * reduction operator class (see <tt>RTOpPack::ROpGetSubVector</tt>) and calls
   * <tt>applyOp()</tt>.  Note that the footprint of the reduction
   * object (both internal and external state) will be
   * O(<tt>rng.size()</tt>).  For serial applications this is fairly
   * reasonable and will not be a major performance penalty.  For
   * parallel applications, this will be a terrible thing to do and
   * must be overridden if <tt>rng.size()</tt> is large at all.  If
   * a subclass does override this operation, it must also override
   * <tt>commitSubVector()</tt> which has a default implementation
   * which is a companion to this operation's default implementation.
   */
  virtual void getSubVector( const Range1D& rng, RTOpPack::MutableSubVectorT<Scalar>* sub_vec );

  /** \brief Commit changes for a mutable explicit view of a sub-vector.
   *
   * @param sub_vec
   *				[in/out] The data in <tt>sub_vec->values()</tt> will be written
   *              back internal storage and the memory referred to by
   *              <tt>sub_vec->values()</tt> will be released if it was allocated
   *				and <tt>*sub_vec</tt> will be zeroed out using
   *				<tt>sub_vec->set_uninitialized()</tt>.
   *
   * Preconditions:<ul>
   * <li> <tt>this->space().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>sub_vec</tt> must have been passed through a call to 
   *      <tt>this->getSubVector(...,sub_vec)</tt>
   * </ul>
    *
   * Postconditions:<ul>
   * <li> See <tt>RTOpPack::MutableSubVectorT::set_uninitialized()</tt> for <tt>sub_vec</tt>
   * <li> <tt>*this</tt> will be updated according the the changes made to <tt>sub_vec</tt>
   * </ul>
   *
   * The sub-vector view must have been allocated by
   * <tt>this->getSubVector()</tt> first.
   *
   * This operation has a default implementation which is a companion
   * to the default implementation for <tt>getSubVector()</tt>.  If
   * <tt>getSubVector()</tt> is overridden by a subclass then this
   * operation must be overridden also!
   */
  virtual void commitSubVector( RTOpPack::MutableSubVectorT<Scalar>* sub_vec );

  /** \brief Set a specific sub-vector.
   *
   * After this function returns, the corresponding elements in
   * <tt>this</tt> vector object will be set equal to those in the
   * input vector.
   *
   * Preconditions:<ul>
   * <li> <tt>this->space().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>sub_vec.global_offset + sub_vec.sub_dim <= this->dim()</tt>
   *      (<tt>throw std::out_of_range</tt>)
   * </ul>
    *
   * Postconditions:<ul>
   * <li> All of the elements in the range
   *      <tt>[sub_vec.global_offset+1,sub_vec.global_offset+sub_vec.sub_dim]</tt>
   *      in <tt>*this</tt> are set to 0.0 except for those that have that
   *      have entries in <tt>sub_vec</tt> which are set to the values specified
   *      by <tt>(*this)(sub_vec.global_offset+vec.local_offset+sub_vec.indices[sub_vec.indices_stride*(k-1)])
   *      = vec.values[vec.value_stride*(k-1)]</tt>, for <tt>k = 1..sub_vec.sub_nz</tt>
   * </ul>
   *
   * The default implementation of this operation uses a
   * transformation operator class (see
   * <tt>RTOpPack::TOpSetSubVector</tt>) and calls <tt>applyOp()</tt>.
   * Be forewarned however, that the operator objects state data (both
   * internal and external) will be order O(<tt>sub_vec.subNz</tt>).
   * For serial applications, this is entirely adequate.  For parallel
   * applications this may be bad!
   *
   * @param  sub_vec  [in] Represents the elements in the subvector to be set.
   */
  virtual void setSubVector( const RTOpPack::SparseSubVectorT<Scalar>& sub_vec );

  //@}

  /** @name Overridden from Teuchos::Describable */
  //@{

  /** \brief Generates a default outputting for all vectors.
   *
   * Calls on the <tt>this->describe(void)</tt> function for the name
   * of the class (and possibly its instance name) and then if
   * <tt>verbLevel >= VERB_HIGH</tt>, then the vector elements
   * themselves are printed as well.  The format of the output is
   * as follows:
   *
   \verbatim

   type = 'this->description()', size = n
     1:x1
     2:x2
     .
     .
     .
     n:xn
   \endverbatim
   *
   * Before <tt>type = 'this->description()'</tt> is printed and after
   * each newline, <tt>leadingIndent</tt> is output.  The
   * <tt>index:value</tt> lines are offset an additional
   * <tt>indentSpacer</tt> amount.  A newline is printed after the
   * last <tt>n:xn</tt> entry.
   */
  std::ostream& describe(
    std::ostream                         &out
    ,const Teuchos::EVerbosityLevel      verbLevel
    ,const std::string                   leadingIndent
    ,const std::string                   indentSpacer
    ) const;

  //@}

}; // end class VectorBase

/** \brief Apply a reduction/transformation,operation over a set of vectors:
 * <tt>op(op(v[0]...v[nv-1],z[0]...z[nz-1]),(*reduct_obj)) -> z[0]...z[nz-1],(*reduct_obj)</tt>.
 *
 * @param  op	[in] Reduction/transformation operator to apply over each sub-vector
 *				and assemble the intermediate targets into <tt>reduct_obj</tt> (if
 *              <tt>reduct_obj != RTOp_REDUCT_OBJ_NULL</tt>).
 * @param  num_vecs
 *				[in] Number of nonmutable vectors in <tt>vecs[]</tt>.
 *              If <tt>vecs==NULL</tt> then this argument is ignored but should be set to zero.
 * @param  vecs
 *				[in] Array (length <tt>num_vecs</tt>) of a set of pointers to
 *				nonmutable vectors to include in the operation.
 *				The order of these vectors is significant to <tt>op</tt>.
 * @param  num_targ_vecs
 *				[in] Number of mutable vectors in <tt>targ_vecs[]</tt>.
 *              If <tt>targ_vecs==NULL</tt>	then this argument is ignored but should be set to zero.
 * @param  targ_vecs
 *				[in] Array (length <tt>num_targ_vecs</tt>) of a set of pointers to
 *				mutable vectors to include in the operation.
 *				The order of these vectors is significant to <tt>op</tt>.
 *				If <tt>targ_vecs==NULL</tt> then <tt>op</tt> is called with no mutable vectors.
 * @param  reduct_obj
 *				[in/out] Target object of the reduction operation.
 *				This object must have been created by the <tt>op.reduct_obj_create_raw(&reduct_obj)</tt>
 *              function first.  The reduction operation will be added to <tt>(*reduct_obj)</tt> if
 *              <tt>(*reduct_obj)</tt> has already been through a reduction.  By allowing the info in
 *              <tt>(*reduct_obj)</tt> to be added to the reduction over all of these vectors, the reduction
 *              operation can be accumulated over a set of abstract vectors	which can be useful for implementing
 *              composite vectors for instance.  If <tt>op.get_reduct_type_num_entries(...)</tt> returns
 *              <tt>num_values == 0</tt>, <tt>num_indexes == 0</tt> and <tt>num_chars == 0</tt> then
 *              <tt>reduct_obj</tt> should be set to <tt>RTOp_REDUCT_OBJ_NULL</tt> and no reduction will be performed.
 * @param  first_ele
 *				[in] (default = 1) The index of the first element in <tt>this</tt> to be included.
 * @param  sub_dim
 *              [in] (default = 0) The number of elements in these vectors to include in the reduction/transformation
 *              operation.  The value of <tt>sub_dim == 0</tt> means to include all available elements.
 * @param  global_offset
 *				[in] (default = 0) The offset applied to the included vector elements.
 *
 *
 * Preconditions:<ul>
 * <li> [<tt>num_vecs > 0</tt>] <tt>vecs[k]->space()->isCompatible(*this->space()) == true</tt>,
 *      for <tt>k = 0...num_vecs-1</tt> (throw <tt>Exceptions::IncompatibleVectorSpaces</tt>)
 * <li> [<tt>num_targ_vecs > 0</tt>] <tt>targ_vecs[k]->space()->isCompatible(*this->space()) == true</tt>,
 *      for <tt>k = 0...num_targ_vecs-1</tt> (throw <tt>Exceptions::IncompatibleVectorSpaces</tt>)
 * <li> [<tt>num_targ_vecs > 0</tt>] The vectors pointed to by <tt>targ_vecs[k]</tt>, for
 *      <tt>k = 0...num_targ_vecs-1</tt> must not alias each other or any of the vectors
 *      <tt>vecs[k]</tt>, for <tt>k = 0...num_vecs-1</tt>.  <b>You have be warned!!!!</b>
 * <li> <tt>1 <= first_ele <= this->dim()</tt> (throw <tt>std::out_of_range</tt>)
 * <li> <tt>global_offset >= 0</tt> (throw <tt>std::invalid_argument</tt>)
 * <li> <tt>sub_dim - (first_ele - 1) <= this->dim()</tt> (throw <tt>std::length_error</tt>).
 * </ul>
 *
 * Postconditions:<ul>
 * <li> The vectors in <tt>targ_vecs[]</tt> may be modified as determined by the definition of <tt>op</tt>.
 * <li> [<tt>reduct_obj!=RTOp_REDUCT_OBJ_NULL</tt>] The reduction object <tt>reduct_obj</tt> contains
 *      the combined reduction from the input state of <tt>reduct_obj</tt> and the reductions that
 *      where accumulated during this this operation invocation.
 * </ul>
 *
 * The logical vector passed to the
 * <tt>op\ref RTOpPack::RTOpT::apply_op ".apply_op(...)"</tt>
 * operation is: \verbatim

 v(k + global_offset) = this->get_ele(first_ele + k - 1)
 , for k = 1 ... sub_dim
 \endverbatim
 *
 * where <tt>v</tt> represents any one of the input or input/output
 * vectors.  The situation where <tt>first_ele == 1</tt> and
 * <tt>global_offset > 1</tt> corresponds to the case where the
 * vectors represent constituent vectors in a larger aggregate
 * vector.  The situation where <tt>first_ele > 1</tt> and
 * <tt>global_offset == 0</tt> is for when a sub-view of the vectors
 * are being treated as full vectors.  Other combinations of these
 * arguments are also possible.
 *
 * \ingroup Thyra_Op_Vec_fundamental_interfaces_code_grp
 */
template<class Scalar>
inline
void applyOp(
  const RTOpPack::RTOpT<Scalar>   &op
  ,const int                      num_vecs
  ,const VectorBase<Scalar>*      vecs[]
  ,const int                      num_targ_vecs
  ,VectorBase<Scalar>*            targ_vecs[]
  ,RTOpPack::ReductTarget         *reduct_obj
#ifdef __sun
  ,const Index                    first_ele
  ,const Index                    sub_dim
  ,const Index                    global_offset
#else
  ,const Index                    first_ele     = 1
  ,const Index                    sub_dim       = 0
  ,const Index                    global_offset = 0
#endif
  )
{
  if(num_vecs)
    vecs[0]->applyOp(op,num_vecs,vecs,num_targ_vecs,targ_vecs,reduct_obj,first_ele,sub_dim,global_offset);
  else if (num_targ_vecs)
    targ_vecs[0]->applyOp(op,num_vecs,vecs,num_targ_vecs,targ_vecs,reduct_obj,first_ele,sub_dim,global_offset);
}

#ifdef __sun
template<class Scalar>
inline
void applyOp(
  const RTOpPack::RTOpT<Scalar>   &op
  ,const int                      num_vecs
  ,const VectorBase<Scalar>*      vecs[]
  ,const int                      num_targ_vecs
  ,VectorBase<Scalar>*            targ_vecs[]
  ,RTOpPack::ReductTarget         *reduct_obj
  )
{
  applyOp(op,num_vecs,vecs,num_targ_vecs,targ_vecs,reduct_obj,1,0,0);
}
#endif

} // end namespace Thyra

#endif  // THYRA_VECTOR_BASE_DECL_HPP
