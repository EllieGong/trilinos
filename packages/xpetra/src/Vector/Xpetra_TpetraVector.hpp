#ifndef XPETRA_TPETRAVECTOR_HPP
#define XPETRA_TPETRAVECTOR_HPP

/* this file is automatically generated - do not edit (see script/tpetra.py) */

#include "Xpetra_TpetraConfigDefs.hpp"

#include "Xpetra_Vector.hpp"
#include "Xpetra_MultiVector.hpp"
#include "Xpetra_TpetraMultiVector.hpp"

#include "Xpetra_TpetraMap.hpp" //TMP
#include "Xpetra_Utils.hpp"
#include "Xpetra_TpetraImport.hpp"
#include "Xpetra_TpetraExport.hpp"

#include "Tpetra_Vector.hpp"

namespace Xpetra {

  // TODO: move that elsewhere
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  Tpetra::Vector<Scalar,LocalOrdinal, GlobalOrdinal, Node> & toTpetra(Vector<Scalar,LocalOrdinal, GlobalOrdinal, Node> &);

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  const Tpetra::Vector<Scalar,LocalOrdinal, GlobalOrdinal, Node> & toTpetra(const Vector<Scalar,LocalOrdinal, GlobalOrdinal, Node> &);
  //

  template <class Scalar, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType>
  class TpetraVector
    : public virtual Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node>, public TpetraMultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node>
  {

  public:

    using TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::dot;          // overloading, not hiding
    using TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::norm1;        // overloading, not hiding
    using TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::norm2;        // overloading, not hiding
    using TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::normInf;      // overloading, not hiding
    using TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::normWeighted; // overloading, not hiding
    using TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::meanValue;    // overloading, not hiding

    //! @name Constructor/Destructor Methods
    //@{ 

    //! Sets all vector entries to zero.
    TpetraVector(const Teuchos::RCP< const Map<LocalOrdinal,GlobalOrdinal,Node> > &map, bool zeroOut=true)
      : TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >(map,1,zeroOut) { }
    
    //! Set multi-vector values from an array using Teuchos memory management classes. (copy)
    TpetraVector(const Teuchos::RCP< const Map<LocalOrdinal,GlobalOrdinal,Node> > &map, const Teuchos::ArrayView< const Scalar > &A)
      : TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >(map,A,map->getNodeNumElements(),1) { }

    //! Destructor.
    virtual ~TpetraVector() { }

    //@}

    //! @name Mathematical methods
    //@{

    //! Computes dot product of this Vector against input Vector x.
    Scalar dot(const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &a) const { return getTpetra_Vector()->dot(toTpetra(a)); }

    //! Return 1-norm of this Vector.
    typename Teuchos::ScalarTraits< Scalar >::magnitudeType norm1() const { return getTpetra_Vector()->norm1(); }

    //! Compute 2-norm of this Vector.
    typename Teuchos::ScalarTraits< Scalar >::magnitudeType norm2() const { return getTpetra_Vector()->norm2(); }

    //! Compute Inf-norm of this Vector.
    typename Teuchos::ScalarTraits< Scalar >::magnitudeType normInf() const { return getTpetra_Vector()->normInf(); }

    //! Compute Weighted 2-norm (RMS Norm) of this Vector.
    typename Teuchos::ScalarTraits< Scalar >::magnitudeType normWeighted(const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &weights) const { return getTpetra_Vector()->normWeighted(toTpetra(weights)); }

    //! Compute mean (average) value of this Vector.
    Scalar meanValue() const { return getTpetra_Vector()->meanValue(); }

    //@}

    //! @name Overridden from Teuchos::Describable
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const { return getTpetra_Vector()->description(); }

    //! Print the object with some verbosity level to an FancyOStream object.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const { getTpetra_Vector()->describe(out, verbLevel); }

    //@}

    //! @name Xpetra specific
    //@{

    //! TpetraMultiVector constructor to wrap a Tpetra::MultiVector object
    TpetraVector(const Teuchos::RCP<Tpetra::Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node> > &vec) : TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >(vec) { } // TODO: removed const of Tpetra::Vector

    //! Get the underlying Tpetra multivector
    RCP< Tpetra::Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node> > getTpetra_Vector() const { return this->TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::getTpetra_MultiVector()->getVectorNonConst(0); }

    //@}
    
  }; // TpetraVector class

  // TODO: move that elsewhere
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  Tpetra::Vector< Scalar,LocalOrdinal, GlobalOrdinal, Node> & toTpetra(Vector< Scalar,LocalOrdinal, GlobalOrdinal, Node> &x) {
    typedef TpetraVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > TpetraVectorClass;
    XPETRA_DYNAMIC_CAST(      TpetraVectorClass, x, tX, "toTpetra");
    return *tX.getTpetra_Vector();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  const Tpetra::Vector< Scalar,LocalOrdinal, GlobalOrdinal, Node> & toTpetra(const Vector< Scalar,LocalOrdinal, GlobalOrdinal, Node> &x) {
    typedef TpetraVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > TpetraVectorClass;
    XPETRA_DYNAMIC_CAST(const TpetraVectorClass, x, tX, "toTpetra");
    return *tX.getTpetra_Vector();
  }
  //

} // Xpetra namespace

#define XPETRA_TPETRAVECTOR_SHORT
#endif // XPETRA_TPETRAVECTOR_HPP
