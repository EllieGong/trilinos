#ifndef __PB_EpetraOperatorWrapper_hpp__
#define __PB_EpetraOperatorWrapper_hpp__

#include "Thyra_LinearOpBase.hpp"
#include "Thyra_VectorImpl.hpp" // need for LinOpDecl
#include "Thyra_VectorSpaceImpl.hpp" // need for LinOpDecl
#include "Thyra_LinearOperatorImpl.hpp"
#include "Epetra_Map.h"
#include "Epetra_Comm.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Operator.h"

#include <string>


namespace PB {
namespace Epetra {
  using Teuchos::RCP;

  class EpetraOperatorWrapper;

  /// Abstract Mapping strategy for an EpetraOperatorWrapper
  class MappingStrategy {
  public:
     /** \brief Copy an Epetra_MultiVector into a Thyra::MultiVectorBase
       *
       * Copy an Epetra_MultiVector into a Thyra::MultiVectorBase. The exact
       * method for copying is specified by the concrete implementations.
       *
       * \param[in]     epetraX Vector to be copied into the Thyra object
       * \param[in,out] thyraX  Destination Thyra object
       * \param[in]     eow     EpetraOperatorWrapper using this object (may be unecessary)
       */
     virtual void copyEpetraIntoThyra(const Epetra_MultiVector& epetraX,
                                      const Teuchos::Ptr<Thyra::MultiVectorBase<double> > & thyraX,
                                      const EpetraOperatorWrapper & eow) const = 0;

     /** \brief Copy an Thyra::MultiVectorBase into a Epetra_MultiVector
       *
       * Copy an Thyra::MultiVectorBase into an Epetra_MultiVector. The exact
       * method for copying is specified by the concrete implementations.
       *
       * \param[in]     thyraX  Source Thyra object
       * \param[in,out] epetraX Destination Epetra object
       * \param[in]     eow     EpetraOperatorWrapper using this object (may be unecessary)
       */
     virtual void copyThyraIntoEpetra(const RCP<const Thyra::MultiVectorBase<double> > & thyraX,
                                      Epetra_MultiVector& epetraX,
                                      const EpetraOperatorWrapper & eow) const = 0;

     /** \brief Domain map for this strategy */
     virtual const RCP<const Epetra_Map> domainMap() const = 0; 

     /** \brief Range map for this strategy */
     virtual const RCP<const Epetra_Map> rangeMap() const = 0;

     /** \brief Identifier string */
     virtual std::string toString() const = 0;
  };

  /// default mapping strategy for the basic EpetraOperatorWrapper
  class DefaultMappingStrategy : public MappingStrategy {
  public:
     /** */
     DefaultMappingStrategy(const RCP<const Thyra::LinearOpBase<double> > & thyraOp, Epetra_Comm & comm);

     /** \brief Copy an Epetra_MultiVector into a Thyra::MultiVectorBase
       *
       * Copy an Epetra_MultiVector into a Thyra::MultiVectorBase. The exact
       * method for copying is specified by the concrete implementations.
       *
       * \param[in]     epetraX Vector to be copied into the Thyra object
       * \param[in,out] thyraX  Destination Thyra object
       * \param[in]     eow     EpetraOperatorWrapper using this object (may be unecessary)
       */
     virtual void copyEpetraIntoThyra(const Epetra_MultiVector& epetraX,
                                      const Teuchos::Ptr<Thyra::MultiVectorBase<double> > & thyraX,
                                      const EpetraOperatorWrapper & eow) const;

     /** \brief Copy an Thyra::MultiVectorBase into a Epetra_MultiVector
       *
       * Copy an Thyra::MultiVectorBase into an Epetra_MultiVector. The exact
       * method for copying is specified by the concrete implementations.
       *
       * \param[in]     thyraX  Source Thyra object
       * \param[in,out] epetraX Destination Epetra object
       * \param[in]     eow     EpetraOperatorWrapper using this object (may be unecessary)
       */
     virtual void copyThyraIntoEpetra(const RCP<const Thyra::MultiVectorBase<double> > & thyraX,
                                      Epetra_MultiVector& epetraX,
                                      const EpetraOperatorWrapper & eow) const;

     /** \brief Domain map for this strategy */
     virtual const RCP<const Epetra_Map> domainMap() const { return domainMap_; }

     /** \brief Range map for this strategy */
     virtual const RCP<const Epetra_Map> rangeMap() const { return rangeMap_; }

     /** \brief Identifier string */
     virtual std::string toString() const
     { return std::string("DefaultMappingStrategy"); } 

  protected:
     RCP<const Thyra::VectorSpaceBase<double> > domainSpace_; ///< Domain space object
     RCP<const Thyra::VectorSpaceBase<double> > rangeSpace_; ///< Range space object

     RCP<const Epetra_Map> domainMap_; ///< Pointer to the constructed domain map
     RCP<const Epetra_Map> rangeMap_; ///< Pointer to the constructed range map
  };

  /** \brief 
   * Implements the Epetra_Operator interface with a Thyra LinearOperator. This
   * enables the use of absrtact Thyra operators in AztecOO as preconditioners and 
   * operators, without being rendered into concrete Epetra matrices. This is my own
   * modified version that was originally in Thyra.
   */
  class EpetraOperatorWrapper : public Epetra_Operator
  {
  public:
    /** */
    EpetraOperatorWrapper(const RCP<const Thyra::LinearOpBase<double> > & thyraOp);
    EpetraOperatorWrapper(const RCP<const Thyra::LinearOpBase<double> > & thyraOp,
                          const RCP<const MappingStrategy> & mapStrategy);
    EpetraOperatorWrapper(const RCP<const MappingStrategy> & mapStrategy);
    
    /** */
    virtual ~EpetraOperatorWrapper() {;}

    /** */
    int SetUseTranspose(bool UseTranspose) {useTranspose_ = UseTranspose; return 0;}

    /** */
    int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const ;

    /** */
    int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const ;

    /** */
    double NormInf() const ;

    /** */
    const char* Label() const {return label_.c_str();}

    /** */
    bool UseTranspose() const {return useTranspose_;}

    /** */
    bool HasNormInf() const {return false;}
    
    /** */
    const Epetra_Comm& Comm() const {return *comm_;}

    /** */
    const Epetra_Map& OperatorDomainMap() const {return *mapStrategy_->domainMap();}

    /** */
    const Epetra_Map& OperatorRangeMap() const {return *mapStrategy_->rangeMap();}


    const RCP<const Thyra::LinearOpBase<double> > getThyraOp() const 
    { return thyraOp_; }

    const RCP<const MappingStrategy> getMapStrategy() const 
    { return mapStrategy_; }

  protected:
    /** */
    EpetraOperatorWrapper();

    /** */
    RCP<Epetra_Comm> getEpetraComm(const Thyra::ConstLinearOperator<double>& thyraOp) const;

    /** */
    void SetOperator(const RCP<const Thyra::LinearOpBase<double> > & thyraOp,bool buildMap=true);

    /** */
    void SetMapStrategy(const RCP<const MappingStrategy> & mapStrategy)
    { mapStrategy_ = mapStrategy; }

    /** */
    RCP<const MappingStrategy> mapStrategy_;


    /** */
    RCP<const Thyra::LinearOpBase<double> > thyraOp_;

    /** */
    bool useTranspose_;

    /** */
    RCP<Epetra_Comm> comm_;

    /** */
    std::string label_;
  };
} // end namespace Epetra 
} // end namespace PB

#endif 