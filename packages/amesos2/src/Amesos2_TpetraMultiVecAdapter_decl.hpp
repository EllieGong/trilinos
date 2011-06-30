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
  \file   Amesos2_TpetraMultiVecAdapter_decl.hpp
  \author Eric T Bavier <etbavier@sandia.gov>
  \date   Wed May 26 19:49:10 CDT 2010

  \brief  Amesos2::MultiVecAdapter specialization for the
	  Tpetra::MultiVector class.
*/

#ifndef AMESOS2_TPETRA_MULTIVEC_ADAPTER_DECL_HPP
#define AMESOS2_TPETRA_MULTIVEC_ADAPTER_DECL_HPP

#include <Teuchos_RCP.hpp>
#include <Teuchos_Array.hpp>
#include <Teuchos_as.hpp>
#include <Tpetra_MultiVector.hpp>

#include "Amesos2_MultiVecAdapter_decl.hpp"

namespace Amesos {


  /**
   * \brief Amesos2 adapter for the Tpetra::MultiVector class.
   *
   * \ingroup amesos2_multivec_adapters
   */
  template< typename Scalar,
	    typename LocalOrdinal,
	    typename GlobalOrdinal,
	    class    Node >
  class MultiVecAdapter<Tpetra::MultiVector<Scalar,
					    LocalOrdinal,
					    GlobalOrdinal,
					    Node> >
  {
  public:

    // public type definitions
    typedef Tpetra::MultiVector<Scalar,
				LocalOrdinal,
				GlobalOrdinal,
				Node>       multivec_type;
    typedef Scalar                          scalar_type;
    typedef LocalOrdinal                    local_ordinal_type;
    typedef GlobalOrdinal                   global_ordinal_type;
    typedef Node                            node_type;
    typedef typename Tpetra::global_size_t  global_size_type;

    static const char* name;


    /// Default Constructor
    MultiVecAdapter()
      : mv_(Teuchos::null)
      , l_mv_(Teuchos::null)
      , l_l_mv_(Teuchos::null)
    { }


    /// Copy constructor
    MultiVecAdapter( const MultiVecAdapter<multivec_type>& adapter )
      : mv_(adapter.mv_)
      , l_mv_(adapter.l_mv_)
      , l_l_mv_(adapter.l_l_mv_)
    { }

    /**
     * \brief Initialize an adapter from a multi-vector RCP.
     *
     * \param m An RCP pointing to the multi-vector which is to be wrapped.
     */
    MultiVecAdapter( const Teuchos::RCP<multivec_type>& m )
      : mv_(m)
      , l_mv_(Teuchos::null)
      , l_l_mv_(Teuchos::null)
    { }


    ~MultiVecAdapter()
    {
      // TODO: Should the destructor export changes to the serial
      // version before it destroys itself, or should we leave the
      // user responsible for calling the updateValues method?
    }

    /**
     * \return An RCP to the matrix that is being adapted by this adapter
     */
    const Teuchos::RCP<multivec_type> getAdaptee(){ return mv_; }

    /**
     * \brief Scales values of \c this by a factor of \c alpha
     *
     * \f$ this = alpha * this\f$
     *
     * \param [in] alpha scalar factor
     *
     * \return A reference to \c this
     */
    MultiVecAdapter<multivec_type>& scale( const scalar_type alpha )
    {
      mv_->scale(alpha);

      return *this;
    }



    /**
     * \brief Updates the values of \c this to \f$this = alpha*this + beta*B\f$
     *
     * \param [in] beta scalar coefficient of \c B
     * \param [in] B additive MultiVector
     * \param [in] alpha scalar coefficient of \c this
     *
     * \return A reference to \c this
     */
    MultiVecAdapter<multivec_type>& update(
					   const scalar_type beta,
					   const MultiVecAdapter<multivec_type>& B,
					   const scalar_type alpha )
    {
      mv_->update(beta, B.mv_, alpha);

      return *this;
    }


    /// Checks whether this multivector is local to the calling node.
    bool isLocal() const
    {
      if(getComm()->getSize() == 1){
	return true;
      } // There may be other conditions to check
    }


    const Teuchos::RCP<const Tpetra::Map<
			 local_ordinal_type,
			 global_ordinal_type,
			 node_type > >&
    getMap() const
    {
      return mv_->getMap();
    }

    /// Returns the Teuchos::Comm object associated with this multi-vector
    const Teuchos::RCP<const Teuchos::Comm<int> >& getComm() const
    {
      return getMap()->getComm();
    }

    /// Get the length of vectors local to the calling node
    size_t getLocalLength() const
    {
      return Teuchos::as<size_t>(mv_->getLocalLength());
    }


    /// Get the number of vectors on this node
    size_t getLocalNumVectors() const
    {
      return mv_->getNumVectors();
    }


    /// Get the length of vectors in the global space
    global_size_type getGlobalLength() const
    {
      // return mv_->getGlobalLength();
      return getMap()->getMaxAllGlobalIndex() + 1;
    }


    /// Get the number of global vectors
    size_t getGlobalNumVectors() const
    {
      return mv_->getNumVectors();
    }


    /// Return the stride between vectors on this node
    size_t getStride() const
    {
      return mv_->getStride();
    }


    /// Return \c true if this MV has constant stride between vectors on this node
    bool isConstantStride() const
    {
      return mv_->isConstantStride();
    }


    /// Const vector access
    Teuchos::RCP<const Tpetra::Vector<scalar_type,local_ordinal_type,global_ordinal_type,node_type> >
    getVector( size_t j ) const
    {
      return mv_->getVector(j);
    }


    /// Nonconst vector access
    Teuchos::RCP<Tpetra::Vector<scalar_type,local_ordinal_type,global_ordinal_type,node_type> >
    getVectorNonConst( size_t j )
    {
      return mv_->getVectorNonConst(j);
    }


    /*
     * TODO: The declaration and implementation of this function needs
     * to eventually be redefined to be more general.  An enum
     * parameter might be sufficient to describe the possible
     * use-cases we would expect to have to support in the long run
     * for all types of solvers.  This must encompase the different
     * ways in which a solver would like the multivector data
     * distributed across processes:
     *
     * - Root only has access to all MV data
     * - All processes have access to all MV data
     * - Processes have access only to its local data
     */
    /**
     * \brief Copies the multivector's data into the user-provided vector.
     *
     *  Each multivector is \c lda apart in memory.
     *
     *  \param             A user-supplied storage for multi-vector data
     *  \param lda         user-supplied spacing for consecutive vectors
     *                     in \c A
     *  \param global_copy Whether a copy of the full global multivector
     *                     is copied into A, or just the local portion.
     *                     If \c true, then A must be large enough to
     *                     fit all global multivector entries; if \c
     *                     false, then it must be large enough to fit
     *                     all local entries.
     *
     *  \throw std::runtime_error Thrown if the space available in \c A
     *  is not large enough given \c lda , the value of \c global_copy ,
     *  and the number of vectors in \c this.
     */
    void get1dCopy( const Teuchos::ArrayView<scalar_type>& A,
		    size_t lda,
		    bool global_copy ) const;


    /**
     * \brief Extracts a 1 dimensional view of this MultiVector's data
     *
     * Guarantees that the view returned will reside in contiguous storage.
     *
     * \warning
     * It is recommended to use the \c get1dCopy function, from a
     * data-hiding perspective. Use if you know what you are doing.
     *
     * \param local if \c true , each node will get a view of the vectors it is
     * in possession of.  The default, \c false , will give each calling node a
     * view of the global multivector.
     */
    Teuchos::ArrayRCP<scalar_type> get1dViewNonConst( bool local = false );

    /**
     * \brief Get a 2-D copy of the multi-vector.
     *
     * Copies a 2-D representation of the multi-vector into the user-supplied
     * array-of-arrays.
     *
     * \param A user-supplied storage for the 2-D copy.
     *
     * \throw std::length_error Thrown if the size of \c A is not equal to the
     * number of vectors in the underlying multi-vector.
     */
    void get2dCopy( Teuchos::ArrayView<const Teuchos::ArrayView<scalar_type> > A ) const;

    /**
     * \brief Extracts a 2 dimensional view of this multi-vector's data.
     *
     * Guarantees that the view returned will reside in contiguous storage.  The
     * data is not \c const , so it may be altered if desired.
     *
     * \warning
     * It is recommended to use the \c get2dCopy function, from a
     * data-hiding perspective. Use if you know what you are doing.
     *
     * \param local if \c true , each node will get a view of the vectors it is
     * in possession of.  The default, \c false , will give each calling node a
     * view of the global multivector.
     *
     * \return An array-of-arrays view of this multi-vector's data
     *
     * \note This function is not declared \c const as it normally would be,
     * since it must modify local copies of the vector data before returning the
     * result.
     */
    Teuchos::ArrayRCP<Teuchos::ArrayRCP<scalar_type> >
    get2dViewNonConst( bool local = false );


    /**
     * \brief Exports any local changes to this MultiVector to the global space.
     *
     * \post The values in \c l_mv_ will be equal to the values in \c mv_
     *
     * \param root gives the rank whose local multivector will be distributed
     */
    void globalize(int root = 0);


    /**
     * \brief Export \c newVals into the global MultiVector space.
     *
     * \note We assume that the leading dimension of the data in
     * newVals is equal to the global length of a vector in \c this.
     *
     * \tparam Value_t The type of the data values that are being put into \c mv_
     *
     * \param newVals The values to be exported into the global space.
     * \param root gives the rank whose values will be distributed
     */
    template<typename Value_t>
    void globalize( const Teuchos::ArrayView<Value_t>& newVals, int root = 0 );


    /// Get a short description of this adapter class
    std::string description() const;


    /// Print a description of this adapter to the Fancy Output Stream.
    void describe( Teuchos::FancyOStream& os,
		   const Teuchos::EVerbosityLevel verbLevel ) const;


  private:

    /**
     * \brief "Localizes" the wrapped \c mv_ .
     *
     * It defines the private maps \c o_map_ and \c l_map_ and imports
     * global data into the root node (given by the function parameter).
     * If \c mv_ is not distributed (which includes locally replicated),
     * this method does nothing.
     *
     * It is intended to set things up properly for calls to
     * \c get1dCopy() and \c get1dView().
     *
     * \param root gives which processor is to have the local copy of
     *             the entire multivector.
     *
     * \sa get1dCopy(), get1dView()
     */
    void localize( bool root ) const;


    /// The multivector this adapter wraps
    Teuchos::RCP<Tpetra::MultiVector<scalar_type,
				     local_ordinal_type,
				     global_ordinal_type,
				     node_type > > mv_;

    /**
     * \brief local multivector.
     *
     * Contains a local view of the entire multivector.
     */
    mutable Teuchos::RCP<Tpetra::MultiVector<scalar_type,
					     local_ordinal_type,
					     global_ordinal_type,
					     node_type > > l_mv_;

    /**
     * \brief local-local multivector.
     *
     * Holds only a representation of the vectors local to the calling processor.
     */
    mutable Teuchos::RCP<Tpetra::MultiVector<scalar_type,
					     local_ordinal_type,
					     global_ordinal_type,
					     node_type > > l_l_mv_;

    /// Used for transferring between local and global multivectors
    mutable Teuchos::RCP<Tpetra::Import<local_ordinal_type,
					global_ordinal_type,
					node_type> > importer_;
    mutable Teuchos::RCP<Tpetra::Export<local_ordinal_type,
					global_ordinal_type,
					node_type> > exporter_;

    /**
     * \brief Local map.
     *
     * If \c mv_ is not distributed, then this should be equivalent to \c o_map_
     */
    mutable Teuchos::RCP<const Tpetra::Map<local_ordinal_type,
					   global_ordinal_type,
					   node_type > > l_map_;

    /// original map
    mutable Teuchos::RCP<const Tpetra::Map<local_ordinal_type,
					   global_ordinal_type,
					   node_type > > o_map_;

  };                              // end class MultiVecAdapter<Tpetra::MultiVector>

} // end namespace Amesos


#endif // AMESOS2_TPETRA_MULTIVEC_ADAPTER_DECL_HPP
