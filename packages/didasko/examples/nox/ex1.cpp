
// @HEADER
// ***********************************************************************
// 
//                      Didasko Tutorial Package
//                 Copyright (2005) Sandia Corporation
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

// Trilinos Tutorial
// -----------------
// Simple nonlinear problem.
// This file shows how to solve the nonlinear problem
//
// x(0)^2 + x(1)^2 -1 = 0 
//      x(1) - x(0)^2 = 0
//
// using NOX. Due to the very small dimension of the problem,
// it should be run with one process.
 
#include "Didasko_ConfigDefs.h"
#if defined(HAVE_DIDASKO_EPETRA) && defined(HAVE_DIDASKO_NOX)

#include <iostream>
#include "Epetra_ConfigDefs.h"
#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_RowMatrix.h"
#include "Epetra_CrsMatrix.h"
#include "NOX.H"
#include "NOX_Epetra_Interface_Required.H"
#include "NOX_Epetra_Interface_Jacobian.H"
#include "NOX_Epetra_LinearSystem_AztecOO.H"
#include "NOX_Epetra_Group.H"

class SimpleProblemInterface : public NOX::Epetra::Interface::Required,
                               public NOX::Epetra::Interface::Jacobian
{

public:
 
  //! Constructor
  SimpleProblemInterface( Epetra_Vector & InitialGuess, 
                          Epetra_Vector & ExactSolution )
  {
    InitialGuess_ = new Epetra_Vector(InitialGuess);
    ExactSolution_ = new Epetra_Vector(ExactSolution);
  };

  //! Destructor
  ~SimpleProblemInterface() 
  {
  };

  bool computeF(const Epetra_Vector & x, Epetra_Vector & f,
                NOX::Epetra::Interface::Required::FillType F )
  {
    f[0] = x[0]*x[0] + x[1]*x[1] - 1.0;
    f[1] = x[1] - x[0]*x[0];
    return true;
  };
  
  bool computeJacobian(const Epetra_Vector & x, Epetra_Operator & Jac)
  {

    Epetra_CrsMatrix * J;
    J = dynamic_cast<Epetra_CrsMatrix*>(&Jac);
    if (J == NULL) {
      cout << "*ERR* Problem_Interface::computeJacobian() - The supplied" << endl;
      cout << "*ERR* Epetra_Operator is NOT an Epetra_CrsMatrix!" << endl;
      throw;
    }
  
    int NumMyElements = J->Map().NumMyElements();
    int * MyGlobalElements = J->Map().MyGlobalElements();
    int indices[2];
    double values[2];
    indices[0] = 0; indices[1] = 1;
    
    for( int i=0 ; i<NumMyElements ; ++i ) {
      switch( MyGlobalElements[i] ) {
      case 0:
        values[0] = 2.0 * x[0];
	values[1] = 2.0 * x[1];
        J->ReplaceGlobalValues(0, 2, values, indices );
	break;
      case 1:
        values[0] = - 2.0 * x[0];
	values[1] = 1.0;
        J->ReplaceGlobalValues(1, 2, values, indices );
	break;
      default:
        cerr << "*ERR*" << endl;
	exit( EXIT_FAILURE );	
      }
    }

    return true;
  }

  bool computePrecMatrix(const Epetra_Vector & x, Epetra_RowMatrix & M) 
  {
    cout << "*ERR* SimpleProblem::preconditionVector()\n";
    cout << "*ERR* don't use explicit preconditioning" << endl;
    exit( 0 );
    throw 1;
  }  
  
  bool computePreconditioner(const Epetra_Vector & x, Epetra_Operator & O)
  {
    cout << "*ERR* SimpleProblem::preconditionVector()\n";
    cout << "*ERR* don't use explicit preconditioning" << endl;
    exit( 0 );
        throw 1;
  }  

private:
  Epetra_Vector * InitialGuess_;
  Epetra_Vector * ExactSolution_;
  
};

// =========== //
// main driver //
// =========== //

int main( int argc, char **argv )
{

#ifdef HAVE_MPI
  MPI_Init(&argc, &argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif
 
  if (Comm.NumProc() != 1) {
    if (Comm.MyPID() == 0)
      cerr << "Please run this test with one process only!" << endl;
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    exit(EXIT_SUCCESS);
  }
      
  // linear map for the 2 global elements
  Epetra_Map Map(2,0,Comm);
  
  // build up initial guess and exact solution
  Epetra_Vector ExactSolution(Map);
  ExactSolution[0] = sqrt(0.5*(sqrt(5.0)-1));
  ExactSolution[1] = 0.5*(sqrt(5.0)-1);
  
  Epetra_Vector InitialGuess(Map);
  InitialGuess[0] = 0.5;
  InitialGuess[1] = 0.5;
    
  // Set up the problem interface
  Teuchos::RefCountPtr<SimpleProblemInterface> interface = 
    Teuchos::rcp(new SimpleProblemInterface(InitialGuess,ExactSolution) );
  
  // Create the top level parameter list
  Teuchos::RefCountPtr<NOX::Parameter::List> nlParamsPtr =
    Teuchos::rcp(new NOX::Parameter::List);
  NOX::Parameter::List& nlParams = *(nlParamsPtr.get());

  // Set the nonlinear solver method
  nlParams.setParameter("Nonlinear Solver", "Line Search Based");

  // Set the printing parameters in the "Printing" sublist
  NOX::Parameter::List& printParams = nlParams.sublist("Printing");
  printParams.setParameter("MyPID", Comm.MyPID()); 
  printParams.setParameter("Output Precision", 3);
  printParams.setParameter("Output Processor", 0);
  printParams.setParameter("Output Information", 
			NOX::Utils::OuterIteration + 
			NOX::Utils::OuterIterationStatusTest + 
			NOX::Utils::InnerIteration +
			NOX::Utils::Parameters + 
			NOX::Utils::Details + 
			NOX::Utils::Warning);

  // start definition of nonlinear solver parameters
  // Sublist for line search 
  NOX::Parameter::List& searchParams = nlParams.sublist("Line Search");
  searchParams.setParameter("Method", "Full Step");

  // Sublist for direction
  NOX::Parameter::List& dirParams = nlParams.sublist("Direction");
  dirParams.setParameter("Method", "Newton");

  NOX::Parameter::List& newtonParams = dirParams.sublist("Newton");
  newtonParams.setParameter("Forcing Term Method", "Constant");

  // Sublist for linear solver for the Newton method
  NOX::Parameter::List& lsParams = newtonParams.sublist("Linear Solver");
  lsParams.setParameter("Aztec Solver", "GMRES");  
  lsParams.setParameter("Max Iterations", 800);  
  lsParams.setParameter("Tolerance", 1e-4);
  lsParams.setParameter("Output Frequency", 50);    
  lsParams.setParameter("Aztec Preconditioner", "ilu"); 

  // define the Jacobian matrix
  Teuchos::RefCountPtr<Epetra_CrsMatrix> A = 
    Teuchos::rcp( new Epetra_CrsMatrix(Copy,Map,2));
  int NumMyElements = A.get()->Map().NumMyElements();
  int * MyGlobalElements = A.get()->Map().MyGlobalElements();
  int indices[2];
  double values[2];
    
  indices[0]=0; indices[1]=1;
  
  for( int i=0 ; i<NumMyElements ; ++i ) {
      switch( MyGlobalElements[i] ) {
      case 0:
        values[0] = 2.0 * InitialGuess[0];
	values[1] = 2.0 * InitialGuess[1];
        A.get()->InsertGlobalValues(0, 2, values, indices );
	break;
      case 1:
        values[0] = - 2.0 * InitialGuess[0];
	values[1] = 1.0;
        A.get()->InsertGlobalValues(1, 2, values, indices );
	break;
    }
  }

  A.get()->FillComplete();  

  Teuchos::RefCountPtr<NOX::Epetra::Interface::Required> iReq = interface;
  Teuchos::RefCountPtr<NOX::Epetra::Interface::Jacobian> iJac = interface;
  Teuchos::RefCountPtr<NOX::Epetra::LinearSystemAztecOO> linSys = 
    Teuchos::rcp(new NOX::Epetra::LinearSystemAztecOO(printParams, lsParams,
						      iReq,
						      iJac, A, 
						      InitialGuess));

  // Need a NOX::Epetra::Vector for constructor
  NOX::Epetra::Vector noxInitGuess(InitialGuess, NOX::DeepCopy);
  Teuchos::RefCountPtr<NOX::Epetra::Group> grpPtr = 
    Teuchos::rcp(new NOX::Epetra::Group(printParams, 
					iReq, 
					noxInitGuess, 
					linSys)); 

  // Set up the status tests
  Teuchos::RefCountPtr<NOX::StatusTest::NormF> testNormF = 
    Teuchos::rcp(new NOX::StatusTest::NormF(1.0e-4));
  Teuchos::RefCountPtr<NOX::StatusTest::MaxIters> testMaxIters = 
    Teuchos::rcp(new NOX::StatusTest::MaxIters(20));
  // this will be the convergence test to be used
  Teuchos::RefCountPtr<NOX::StatusTest::Combo> combo = 
    Teuchos::rcp(new NOX::StatusTest::Combo(NOX::StatusTest::Combo::OR, 
					    testNormF, testMaxIters));

  // Create the solver
  NOX::Solver::Manager solver(grpPtr, combo, nlParamsPtr);

  // Solve the nonlinesar system
  NOX::StatusTest::StatusType status = solver.solve();

  if( NOX::StatusTest::Converged  != status )
    cout << "\n" << "-- NOX solver converged --" << "\n";
  else
    cout << "\n" << "-- NOX solver did not converge --" << "\n";

  // Print the answer
  cout << "\n" << "-- Parameter List From Solver --" << "\n";
  solver.getParameterList().print(cout);

  // Get the Epetra_Vector with the final solution from the solver
  const NOX::Epetra::Group & finalGroup = 
    dynamic_cast<const NOX::Epetra::Group&>(solver.getSolutionGroup());
  const Epetra_Vector & finalSolution = 
      (dynamic_cast<const NOX::Epetra::Vector&>(finalGroup.getX())).getEpetraVector();

  if( Comm.MyPID() == 0 ) cout << "Computed solution : " << endl;
  cout << finalSolution;

  if( Comm.MyPID() == 0 ) cout << "Exact solution : " << endl;
  cout << ExactSolution;
  
#ifdef HAVE_MPI
  MPI_Finalize();
#endif
  return(EXIT_SUCCESS);
}

#else

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_MPI
#include "mpi.h"
#endif

int main(int argc, char *argv[])
{
#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
#endif
  puts("Please configure Didasko with:");
  puts("--enable-epetra");
  puts("--enable-nox");

#ifdef HAVE_MPI
  MPI_Finalize();
#endif
  return(EXIT_SUCCESS);
}
#endif
