// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
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
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
/*
 * Condif2d_test.cpp
 *
 *  Created on: Oct 10, 2011
 *      Author: wiesner
 */

#include <unistd.h>
#include <iostream>

// Teuchos
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_DefaultComm.hpp>

// Xpetra
#include <Xpetra_Map.hpp>
#include <Xpetra_CrsMatrixWrap.hpp>
#include <Xpetra_VectorFactory.hpp>
#include <Xpetra_MultiVectorFactory.hpp>
#include <Xpetra_Parameters.hpp>

// MueLu
#include "MueLu_ConfigDefs.hpp"
#include "MueLu_Memory.hpp"
#include "MueLu_Hierarchy.hpp"
#include "MueLu_PgPFactory.hpp"
#include "MueLu_GenericRFactory.hpp"
#include "MueLu_SaPFactory.hpp"
#include "MueLu_TransPFactory.hpp"
#include "MueLu_RAPFactory.hpp"
#include "MueLu_TrilinosSmoother.hpp"
#include "MueLu_Utilities.hpp"
#include "MueLu_Exceptions.hpp"

#include "EpetraExt_CrsMatrixIn.h"
#include "EpetraExt_VectorIn.h"

//
typedef double Scalar;
typedef int    LocalOrdinal;
#ifdef HAVE_TEUCHOS_LONG_LONG_INT
typedef long long int GlobalOrdinal;
#else
typedef int GlobalOrdinal;
#endif
//
typedef KokkosClassic::DefaultNode::DefaultNodeType Node;
typedef KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Node>::SparseOps LocalMatOps;
//

int main(int argc, char *argv[]) {
#include "MueLu_UseShortNames.hpp"

  using Teuchos::RCP;

  Teuchos::oblackholestream blackhole;
  Teuchos::GlobalMPISession mpiSession(&argc,&argv,&blackhole);

  RCP<const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();
  RCP<Teuchos::FancyOStream> out = Teuchos::fancyOStream(Teuchos::rcpFromRef(std::cout));
  //out->setOutputToRootOnly(0);
  *out << MueLu::MemUtils::PrintMemoryUsage() << std::endl;

  // Timing
  Teuchos::Time myTime("global");
  Teuchos::TimeMonitor M(myTime);

  //out->setOutputToRootOnly(-1);
  //out->precision(12);

#ifndef HAVE_TEUCHOS_LONG_LONG_INT
  *out << "Warning: scaling test was not compiled with long long int support" << std::endl;
#endif

  // custom parameters
  LO maxLevels = 10;
  LO its=100;
  std::string smooType="sgs";
  int pauseForDebugger=0;
  int amgAsSolver=1;
  int amgAsPrecond=1;
  int useExplicitR=0;
  int sweeps=1;
  int maxCoarseSize=1;  //FIXME clp doesn't like long long int
  Scalar SADampingFactor=4./3;
  double tol = 1e-7;
  std::string aggOrdering = "natural";
  int minPerAgg=2;
  int maxNbrAlreadySelected=0;

  // read in problem
  Epetra_Map emap(10201,0,*Xpetra::toEpetra(comm));
  Epetra_CrsMatrix * ptrA = 0;
  Epetra_Vector * ptrf = 0;

  std::cout << "Reading matrix market file" << std::endl;
  //EpetraExt::MatrixMarketFileToCrsMatrix("/home/wiesner/trilinos/Trilinos_dev/fc8_openmpi_dbg2/preCopyrightTrilinos/muelu/test/transferOps/Condif2Mat.mat",emap,emap,emap,ptrA);
  //EpetraExt::MatrixMarketFileToVector("/home/wiesner/trilinos/Trilinos_dev/fc8_openmpi_dbg2/preCopyrightTrilinos/muelu/test/transferOps/Condif2Rhs.mat",emap,ptrf);
  EpetraExt::MatrixMarketFileToCrsMatrix("Condif2Mat.mat",emap,emap,emap,ptrA);
  EpetraExt::MatrixMarketFileToVector("Condif2Rhs.mat",emap,ptrf);
  RCP<Epetra_CrsMatrix> epA = Teuchos::rcp(ptrA);
  RCP<Epetra_Vector> epv = Teuchos::rcp(ptrf);

  // Epetra_CrsMatrix -> Xpetra::Matrix
  RCP<Xpetra::CrsMatrix<double, int, int> > exA = Teuchos::rcp(new Xpetra::EpetraCrsMatrix(epA));
  RCP<Xpetra::CrsMatrixWrap<double, int, int> > crsOp = Teuchos::rcp(new Xpetra::CrsMatrixWrap<double, int, int>(exA));
  RCP<Xpetra::Matrix<double, int, int> > Op = Teuchos::rcp_dynamic_cast<Xpetra::Matrix<double, int, int> >(crsOp);

  // Epetra_Vector -> Xpetra::Vector
  RCP<Xpetra::Vector<double,int,int> > xRhs = Teuchos::rcp(new Xpetra::EpetraVector(epv));

  // Epetra_Map -> Xpetra::Map
  const RCP< const Xpetra::Map<int, int> > map = Xpetra::toXpetra(emap);

  // build nullspace
  RCP<MultiVector> nullSpace = MultiVectorFactory::Build(map,1);
  nullSpace->putScalar( (SC) 1.0);
  /*for (size_t i=0; i<nullSpace->getLocalLength(); i++) {
    Teuchos::ArrayRCP< Scalar > data0 = nullSpace->getDataNonConst(0);
    Teuchos::ArrayRCP< Scalar > data1 = nullSpace->getDataNonConst(1);

    GlobalOrdinal gid = map->getGlobalElement(Teuchos::as<LocalOrdinal>(i));
    if(gid % 2 == 0) {
      data0[i] = 1.0; data1[i] = 0.0;
    }
    else {
      data0[i] = 0.0; data1[i] = 1.0;
    }
  }*/

  RCP<MueLu::Hierarchy<SC,LO,GO,NO,LMO> > H = rcp ( new Hierarchy() );
  H->setDefaultVerbLevel(Teuchos::VERB_HIGH);
  H->SetMaxCoarseSize((GO) maxCoarseSize);;

  // build finest Level
  RCP<MueLu::Level> Finest = H->GetLevel();
  Finest->setDefaultVerbLevel(Teuchos::VERB_HIGH);
  Finest->Set("A",Op);
  Finest->Set("Nullspace",nullSpace);

  RCP<CoupledAggregationFactory> CoupledAggFact = rcp(new CoupledAggregationFactory());
  *out << "========================= Aggregate option summary  =========================" << std::endl;
  *out << "min DOFs per aggregate :                " << minPerAgg << std::endl;
  *out << "min # of root nbrs already aggregated : " << maxNbrAlreadySelected << std::endl;
  CoupledAggFact->SetMinNodesPerAggregate(minPerAgg);  //TODO should increase if run anything other than 1D
  CoupledAggFact->SetMaxNeighAlreadySelected(maxNbrAlreadySelected);
  std::transform(aggOrdering.begin(), aggOrdering.end(), aggOrdering.begin(), ::tolower);
  if (aggOrdering == "natural") {
       *out << "aggregate ordering :                    NATURAL" << std::endl;
       CoupledAggFact->SetOrdering(MueLu::AggOptions::NATURAL);
  } else if (aggOrdering == "random") {
       *out << "aggregate ordering :                    RANDOM" << std::endl;
       CoupledAggFact->SetOrdering(MueLu::AggOptions::RANDOM);
  } else if (aggOrdering == "graph") {
       *out << "aggregate ordering :                    GRAPH" << std::endl;
       CoupledAggFact->SetOrdering(MueLu::AggOptions::GRAPH);
  } else {
    std::string msg = "main: bad aggregation option """ + aggOrdering + """.";
    throw(MueLu::Exceptions::RuntimeError(msg));
  }
  CoupledAggFact->SetPhase3AggCreation(0.5);
  *out << "=============================================================================" << std::endl;

  // build transfer operators
  RCP<TentativePFactory> TentPFact = rcp(new TentativePFactory(CoupledAggFact));

  *out << " afer TentativePFactory " << std::endl;
  //RCP<TentativePFactory> Pfact = rcp(new TentativePFactory(CoupledAggFact));
  //RCP<Factory>          Rfact = rcp( new TransPFactory(Pfact));
  //RCP<SaPFactory>       Pfact = rcp( new SaPFactory(TentPFact) );
  //RCP<Factory>         Rfact = rcp( new TransPFactory(Pfact));
  RCP<PgPFactory>       Pfact = rcp( new PgPFactory(TentPFact) );
  RCP<Factory>         Rfact = rcp( new GenericRFactory(Pfact));
  RCP<RAPFactory>       Acfact = rcp( new RAPFactory(Pfact, Rfact) );
  Acfact->setVerbLevel(Teuchos::VERB_HIGH);

  *out << " after ACFactory " << std::endl;

  // build level smoothers

  RCP<SmootherPrototype> smooProto;
  std::string ifpackType;
  Teuchos::ParameterList ifpackList;
  ifpackList.set("relaxation: sweeps", (LO) sweeps);
  ifpackList.set("relaxation: damping factor", (SC) 0.9); // 0.7
  /*if (smooType == "sgs") {
    ifpackType = "RELAXATION";
    ifpackList.set("relaxation: type", "Symmetric Gauss-Seidel");
  } else if (smooType == "gs") {*/
    ifpackType = "RELAXATION";
    ifpackList.set("relaxation: type", "Gauss-Seidel");
  /*}
  else if (smooType == "cheby") {
    ifpackType = "CHEBYSHEV";
    ifpackList.set("chebyshev: degree", (LO) sweeps);
    ifpackList.set("chebyshev: ratio eigenvalue", (SC) 20);
    ifpackList.set("chebyshev: max eigenvalue", (double) -1.0);
    ifpackList.set("chebyshev: min eigenvalue", (double) 1.0);
    ifpackList.set("chebyshev: zero starting solution", true);
  }*/

  smooProto = Teuchos::rcp( new TrilinosSmoother(Xpetra::UseEpetra, ifpackType, ifpackList) );
  RCP<SmootherFactory> SmooFact;
  if (maxLevels > 1)
    SmooFact = rcp( new SmootherFactory(smooProto) );

  Teuchos::ParameterList status;
  status = H->FullPopulate(*Pfact,*Rfact,*Acfact,*SmooFact,0,maxLevels);

  H->SetCoarsestSolver(*SmooFact,MueLu::PRE);


  *out  << "======================\n Multigrid statistics \n======================" << std::endl;
  status.print(*out,Teuchos::ParameterList::PrintOptions().indent(2));

  /**********************************************************************************/
  /* SOLVE PROBLEM                                                                  */
  /**********************************************************************************/

  RCP<MultiVector> x = MultiVectorFactory::Build(map,1);
  RCP<Xpetra::MultiVector<double,int,int> > rhs = Teuchos::rcp_dynamic_cast<Xpetra::MultiVector<double,int,int> >(xRhs);//(new Xpetra::EpetraVector(epv));

  // Use AMG directly as an iterative method
  {
    x->putScalar( (SC) 0.0);

    H->Iterate(*rhs,*x,its);

    //x->describe(*out,Teuchos::VERB_EXTREME);
  }

  return EXIT_SUCCESS;
}

