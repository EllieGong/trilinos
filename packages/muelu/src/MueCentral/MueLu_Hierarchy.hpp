#ifndef MUELU_HIERARCHY_HPP
#define MUELU_HIERARCHY_HPP

#include "Teuchos_RefCountPtr.hpp"
#include "Teuchos_VerboseObject.hpp"

#include "MueLu_Level.hpp"
#include "MueLu_SaPFactory.hpp"
#include "MueLu_GenericPRFactory.hpp"
#include "MueLu_TransPFactory.hpp"
#include "MueLu_RAPFactory.hpp"
#include "MueLu_SmootherFactory.hpp"
#include "MueLu_Types.hpp"
#include "MueLu_IfpackSmoother.hpp"

namespace MueLu {
/*!
  @class Hierarchy
  @brief Provides methods to build a multigrid hierarchy and apply multigrid cycles.

  Allows users to manually populate operators at different levels within 
  a multigrid method and push them into the hierarchy via SetLevel() 
  and/or to supply factories for automatically generating prolongators, 
  restrictors, and coarse level discretizations.  Additionally contains 
  a V-cycle apply method.
*/
template<class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
class Hierarchy : public Teuchos::VerboseObject<Hierarchy<Scalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps> > {

#include "MueLu_UseShortNames.hpp"

  template<class AA, class BB, class CC, class DD, class EE>
  inline friend std::ostream& operator<<(std::ostream& os, Hierarchy<AA,BB,CC,DD,EE> &hierarchy);

  private:

    //! vector of Level objects
    std::vector<Teuchos::RCP<Level> > Levels_;
    //! print residual history during iteration
    bool printResidualHistory_;

  protected:
    Teuchos::RCP<Teuchos::FancyOStream> out_;

  public:

  //! @name Constructors/Destructors
  //@{

    //! Default constructor.
    Hierarchy() : printResidualHistory_(false), out_(this->getOStream()) {}

    //! Copy constructor.
    Hierarchy(Hierarchy const &inHierarchy) {
      std::cerr << "Not implemented yet." << std::endl;
    }

    //! Destructor.
    virtual ~Hierarchy() {}

   //@}

   //! @name Set/Get Methods.
   //@{

     //! Assign a level to hierarchy.
     void SetLevel(Teuchos::RCP<Level> const& level) {
       Levels_.push_back(level);
       level->SetLevelID(Levels_.size());
     }

     //! Retrieve a certain level from hierarchy.
     Teuchos::RCP<Level>& GetLevel(int const levelID) {
       return Levels_[levelID];
     }

     LO GetNumberOfLevels() {
       return Levels_.size();
     }

     //! Indicate whether to print residual history
     void PrintResidualHistory(bool value) {
       printResidualHistory_ = value;
     }

     //! Retrieve whether to print residual history
     bool PrintResidualHistory() {
       return printResidualHistory_;
     }

   //@}

   //! @name Populate Methods.
   //@{

     /*!
       @brief Constructs components of the hierarchy.

       Invoke a set of factories to populate (construct prolongation,
       restriction, coarse level discretizations, and smoothers in this
       order) a multigrid Hierarchy starting with information on 'startLevel'
       and continuing for at most 'numDesiredLevels'.

       Note: Empty factories are simply skipped.
     */
     Teuchos::ParameterList FullPopulate(Teuchos::RCP<PRFactory> PRFact=Teuchos::null,
                       Teuchos::RCP<OperatorFactory> AcFact=Teuchos::null,
                       Teuchos::RCP<SmootherFactory> SmooFact=Teuchos::null,
                       int const &startLevel=0, int const &numDesiredLevels=10 )
     {
       Teuchos::OSTab tab(out_);
       MueLu_cout(Teuchos::VERB_HIGH) << "Hierarchy::FullPopulate()" << std::endl;

       if (PRFact == Teuchos::null) {
         RCP<SaPFactory> SaPFact = rcp( new SaPFactory() );
         PRFact = rcp( new GenericPRFactory(SaPFact));
       }
       if (AcFact == Teuchos::null) AcFact = rcp( new RAPFactory());
       if (SmooFact == Teuchos::null) {
//FIXME #ifdef we're using tpetra
//FIXME    throw(Exceptions::NotImplemented("No default smoother is defined"));
//FIXME #else we're using epetra
         Teuchos::ParameterList  ifpackList;
         ifpackList.set("relaxation: type", "Gauss-Seidel");
         ifpackList.set("relaxation: sweeps", (int) 1);
         ifpackList.set("relaxation: damping factor", (double) 1.0);
         ifpackList.set("relaxation: zero starting solution", false);
         RCP<IfpackSmoother>  smoother = rcp( new IfpackSmoother("point relaxation stand-alone",ifpackList) );
         SmooFact = rcp( new SmootherFactory(smoother) );
//FIXME #endif
       }

       Teuchos::ParameterList status;
       status = FillHierarchy(*PRFact,*AcFact,startLevel,numDesiredLevels /*,status*/);
       SetSmoothers(*SmooFact,startLevel,numDesiredLevels);
       return status;

     } //FullPopulate()

     /*! @brief Populate hierarchy with A's, R's, and P's.

         Prolongator factory defaults to SaPFactory.
     */

     Teuchos::ParameterList FillHierarchy() {
       RCP<SaPFactory> PFact = rcp(new SaPFactory());
       RCP<GenericPRFactory>  PRFact = rcp(new GenericPRFactory(PFact));
       RCP<RAPFactory> AcFact = rcp(new RAPFactory());
       Teuchos::ParameterList status;
       status = FillHierarchy(*PRFact,*AcFact);
       return status;
     } //FillHierarchy()

     Teuchos::ParameterList FillHierarchy(PRFactory const &PRFact) {
       RAPFactory AcFact;
       Teuchos::ParameterList status;
       status = FillHierarchy(PRFact,AcFact);
       return status;
     }

     //TODO should there be another version of FillHierarchy:
     //TODO   Teuchos::ParameterList FillHierarchy(OperatorFactory const &AcFact)
     //TODO where the PRFactory is automatically generated?
     
     /*! @brief Populate hierarchy with A's, R's, and P's.

         Invoke a set of factories to populate (construct prolongation,
         restriction, and coarse level discretizations in this
         order) a multigrid Hierarchy starting with information on 'startLevel' 
         and continuing for at most 'numDesiredLevels'. 
     */
     Teuchos::ParameterList FillHierarchy(PRFactory const &PRFact,
                        OperatorFactory const &AcFact,
                        int startLevel=0, int numDesiredLevels=10 )
     {
       Teuchos::OSTab tab(out_);
       MueLu_cout(Teuchos::VERB_HIGH) << "Hierarchy::FillHierachy()" << std::endl;

       RCP<Operator> A = Levels_[startLevel]->GetA();
       Cthulhu::global_size_t fineNnz = A->getGlobalNumEntries();
       Cthulhu::global_size_t totalNnz = fineNnz;

       bool goodBuild=true;
       int i = startLevel;
       while (i < startLevel + numDesiredLevels - 1)
       {
         if ( (i+1) >= (int) Levels_.size() || Levels_[i+1] == Teuchos::null ) {
           Teuchos::OSTab tab2(out_);
           Levels_.push_back( Levels_[i]->Build(*out_) );
         }
         Levels_[i+1]->SetLevelID(i+1);
         goodBuild = PRFact.Build(*(Levels_[i]),*(Levels_[i+1]));
         if ((int)Levels_.size() <= i) goodBuild=false; //TODO is this the right way to cast?
         if (!goodBuild) {
           Levels_.resize(i+1); //keep only entries 0..i
           break;
         }
         if ( !AcFact.Build(*(Levels_[i]),*(Levels_[i+1])) ) {
           Levels_.resize(i+1); //keep only entries 0..i
           break;
         }
         //RCP<Operator> A = Levels_[i+1]->GetA();
         totalNnz += Levels_[i+1]->GetA()->getGlobalNumEntries();
         ++i;
       } //while

       Teuchos::ParameterList status;
       status.set("fine nnz",fineNnz);
       status.set("total nnz",totalNnz);
       status.set("start level",startLevel);
       status.set("end level",i);
       status.set("operator complexity",((SC)totalNnz) / fineNnz);
       return status;
     } //FillHierarchy

     /*! @brief Set solve method for coarsest method.

         @param smooFact  fully constructed SmootherFactory 
         @param pop       whether to use pre,post, or both pre and post smoothing 

         Note: Whether the SmootherFactory builds both a pre- and post-smoother can be also be
         controlled by SmootherFactory::SetSmootherPrototypes. This approach is a bit cumbersome,
         however.
     */
     void SetCoarsestSolver(SmootherFactory const &smooFact, PreOrPost const &pop = BOTH) {
       RCP<SmootherPrototype> preSmoo;
       RCP<SmootherPrototype> postSmoo;
       LO clevel = GetNumberOfLevels()-1;

       smooFact.Build(Levels_[clevel],preSmoo,postSmoo);
       if (pop == PRE)       postSmoo = Teuchos::null;
       else if (pop == POST) preSmoo  = Teuchos::null;
       Levels_[clevel]->SetPreSmoother(preSmoo);
       Levels_[clevel]->SetPostSmoother(postSmoo);
     }

     /*! @brief Construct smoothers on all levels but the coarsest.

       Defaults to using smoother factory that generates Gauss-Seidel smoothers.
     */
     void SetSmoothers()
     {
       Teuchos::ParameterList  ifpackList;
//FIXME #ifdef we're using tpetra
//FIXME    throw(Exceptions::NotImplemented("No default smoother is defined"));
//FIXME #else we're using epetra
       ifpackList.set("relaxation: type", "Gauss-Seidel");
       ifpackList.set("relaxation: sweeps", (int) 1);
       ifpackList.set("relaxation: damping factor", (double) 1.0);
       ifpackList.set("relaxation: zero starting solution", false);
       RCP<IfpackSmoother>  smoother = rcp( new IfpackSmoother("point relaxation stand-alone",ifpackList) );
       SmootherFactory smooFact(smoother);
//FIXME #endif
       SetSmoothers(smooFact);
     }

     /*! @brief Construct smoothers on all levels but the coarsest.
       TODO need to check whether using Tpetra or Epetra

        Invoke a set of factories to construct smoothers within 
        a multigrid Hierarchy starting with information on 'startLevel' 
        and continuing for at most 'numDesiredLevels'. 

        Note: last level smoother will not be set here. Use SetCoarsestSolver()
        to define a smoother for the last level. Otherwise, a direct solve is
        assumed
     */
     void SetSmoothers(SmootherFactory const &smooFact, LO const &startLevel=0, LO numDesiredLevels=-1)
     {
       Teuchos::OSTab tab(out_);
       MueLu_cout(Teuchos::VERB_HIGH) << "Hierarchy::SetSmoothers()" << std::endl;

       if (numDesiredLevels == -1)
         numDesiredLevels = GetNumberOfLevels()-startLevel;
       LO lastLevel = startLevel + numDesiredLevels - 1;

       //checks
       if (startLevel >= GetNumberOfLevels())
         throw(Exceptions::RuntimeError("startLevel >= actual number of levels"));

       if (lastLevel >= GetNumberOfLevels()) {
         lastLevel = GetNumberOfLevels() - 1;
         MueLu_cout(Teuchos::VERB_HIGH)
           << "Warning: Coarsest Level will have a direct solve!" << std::endl;
       }

       for (int i=startLevel; i<=lastLevel; i++) {
         Teuchos::RCP<SmootherPrototype> preSm, postSm;
         smooFact.Build(Levels_[i], preSm, postSm);
         if (preSm != Teuchos::null) Levels_[i]->SetPreSmoother(preSm);
         if (postSm != Teuchos::null) Levels_[i]->SetPostSmoother(postSm);
       }

     } //SetSmoothers()

//         typedef typename Teuchos::ScalarTraits<SC>::magnitudeType Magnitude;
//FIXME delete this macro
#define GimmeNorm(someVec,someLabel) {(someVec)->norm2(norms); \
         *out_ << someLabel << " = " << norms<< std::endl;}

     /*!
       @brief Apply the multigrid preconditioner.
     */
     void Iterate(RCP<MultiVector> const &rhs, LO nIts, RCP<MultiVector> &X,
                  bool const &InitialGuessIsZero=false, CycleType const &Cycle=VCYCLE, LO const &startLevel=0)
     {
       //Teuchos::Array<Magnitude> norms(1);
       //Teuchos::OSTab tab(*out_);

       for (LO i=0; i<nIts; i++) {

         RCP<Level> Fine = Levels_[startLevel];

         if (printResidualHistory_ && startLevel==0) {
           *out_ << "iter:    "
                 << std::setiosflags(ios::left)
                 << std::setprecision(3) << i;
           *out_ << "           residual = "
                 << std::setprecision(10) << Utils::ResidualNorm(*(Fine->GetA()),*X,*rhs)
                 << std::endl;
         }
         RCP<SmootherPrototype> preSmoo = Fine->GetPreSmoother();
         RCP<SmootherPrototype> postSmoo = Fine->GetPostSmoother();

         //X->norm2(norms);

         //If on the coarse level, do either smoothing (if defined) or a direct solve.
         if (startLevel == Levels_.size()-1) //FIXME is this right?
         {
           bool emptySolve=true;
           if (preSmoo != Teuchos::null) {preSmoo->Apply(X, rhs, false); emptySolve=false;}
           if (postSmoo != Teuchos::null) {postSmoo->Apply(X, rhs, false); emptySolve=false;}
           if (emptySolve==true)
             *out_ << "WARNING:  no coarse grid solve!!" << std::endl;
         } else {
           //on an intermediate level
           RCP<Level> Coarse = Levels_[startLevel+1];
           if (preSmoo != Teuchos::null)
             preSmoo->Apply(X, rhs, false);

           RCP<MultiVector> residual = Utils::Residual(*(Fine->GetA()),*X,*rhs);
           RCP<Operator> R = Coarse->GetR();
           RCP<MultiVector> coarseRhs = MultiVectorFactory::Build(R->getRowMap(),X->getNumVectors());
           R->multiply(*residual,*coarseRhs,Teuchos::NO_TRANS,1.0,0.0);

           RCP<MultiVector> coarseX = MultiVectorFactory::Build(R->getRowMap(),X->getNumVectors());
           coarseX->putScalar(0.);

           Iterate(coarseRhs,1,coarseX,true,Cycle,startLevel+1);
                                     // ^^ zero initial guess
           if (Cycle>1)
             Iterate(coarseRhs,1,coarseX,false,Cycle,startLevel+1);
                                     // ^^ nonzero initial guess
     
           RCP<Operator> P = Coarse->GetP();
           RCP<MultiVector> correction = MultiVectorFactory::Build(P->getRowMap(),X->getNumVectors());
           P->multiply(*coarseX,*correction,Teuchos::NO_TRANS,1.0,0.0);
           //correction->norm2(norms);

           X->update(1.0,*correction,1.0);

           if (postSmoo != Teuchos::null) {
             //X->norm2(norms);
             postSmoo->Apply(X, rhs, false);
           }
         }

       } //for (LO i=0; i<nIts; i++)

     } //Iterate()

   //@}
    
}; //class Hierarchy

template<class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
std::ostream& operator<<(std::ostream& os, Hierarchy<Scalar,LocalOrdinal,GlobalOrdinal,Node, LocalMatOps> &hierarchy) {
  os << "Printing Hierarchy object" << std::endl;
  typename std::vector< Teuchos::RCP<Level<Scalar,LocalOrdinal,GlobalOrdinal,Node, LocalMatOps> > >::const_iterator i;
  for (i=hierarchy.Levels_.begin(); i != hierarchy.Levels_.end(); ++i)
    os << *(*i) << std::endl;
  return os;
}

} //namespace MueLu

#define MUELU_HIERARCHY_SHORT

#endif //ifndef MUELU_HIERARCHY_HPP
