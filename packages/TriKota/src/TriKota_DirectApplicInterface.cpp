#include <iostream>
#include "TriKota_Driver.hpp"
#include "Epetra_Map.h"
#include "Teuchos_TestForException.hpp"
#include "Teuchos_VerboseObject.hpp"

using namespace std;
using namespace Dakota;

// Define interface class
TriKota::DirectApplicInterface::DirectApplicInterface(
  const ProblemDescDB& problem_db_,
  const Teuchos::RCP<EpetraExt::ModelEvaluator> App_
  )
  : Dakota::DirectApplicInterface(problem_db_),
    App(App_)
{

  Teuchos::RCP<Teuchos::FancyOStream>
    out = Teuchos::VerboseObjectBase::getDefaultOStream();

  if (App != Teuchos::null) {
    model_p = Teuchos::rcp(new Epetra_Vector(*(App->get_p_init(0))));
    model_g = Teuchos::rcp(new Epetra_Vector(*(App->get_g_map(0))));
    //model_g = Teuchos::rcp(new Epetra_Vector((const Epetra_BlockMap&) *(App->get_g_map(0)), true));
    model_dgdp = Teuchos::rcp(new Epetra_MultiVector(model_p->Map(), model_g->GlobalLength() ));

    numParameters = model_p->GlobalLength();
    numResponses  = model_g->GlobalLength();

    *out << "TriKota:: ModeEval has " << numParameters <<
            " parameters and " << numResponses << " responses." << endl;
  }
  else {
    *out << "Warning in TriKota::DirectApplicInterface constructor\n" 
         << "\tModelEvaluator is null. This is OK iff Dakota has assigned"
         << " MPI_COMM_NULL to this Proc " << endl;
  }
}

int TriKota::DirectApplicInterface::derived_map_ac(const Dakota::String& ac_name)
{

  if (App != Teuchos::null) {

    // test for consistency of problem definition between ModelEval and Dakota
    TEST_FOR_EXCEPTION(numVars > numParameters, logic_error,
                       "TriKota_Dakota Adapter Error: ");
    TEST_FOR_EXCEPTION(numADV != 0, logic_error,
                       "TriKota_Dakota Adapter Error: ");
    TEST_FOR_EXCEPTION(numFns > numResponses, logic_error,
                       "TriKota_Dakota Adapter Error: ");
    TEST_FOR_EXCEPTION(hessFlag, logic_error,
                       "TriKota_Dakota Adapter Error: ");

    EpetraExt::ModelEvaluator::InArgs inArgs = App->createInArgs();
    EpetraExt::ModelEvaluator::OutArgs outArgs = App->createOutArgs();

    bool supportsSensitivities =
      !outArgs.supports(EpetraExt::ModelEvaluator::OUT_ARG_DgDp, 0, 0).none();

    TEST_FOR_EXCEPTION(gradFlag && !supportsSensitivities, logic_error,
      "TriKota_Dakota Adapter Error: ");

    // Load parameters from Dakota to ModelEval data structure
    for (int i=0; i<numVars; i++) (*model_p)[i]=xC[i];

    // Evaluate model
    inArgs.set_p(0,model_p);
    outArgs.set_g(0,model_g);
    if (gradFlag) outArgs.set_DgDp(0,0,model_dgdp);
    App->evalModel(inArgs, outArgs);

    for (int j=0; j<numFns; j++) fnVals[j]= (*model_g)[j];

    if (gradFlag) 
      for (int i=0; i<numVars; i++)
        for (int j=0; j<numFns; j++)
          fnGrads[i][j]= (*model_dgdp)[j][i];
  }
  else {
    TEST_FOR_EXCEPTION(parallelLib.parallel_configuration().ea_parallel_level().server_intra_communicator()
      != MPI_COMM_NULL, logic_error,
      "\nTriKota Parallelism Error: ModelEvaluator=null, but analysis_comm != MPI_COMMM_NULL");
  }

  return 0;

}

int TriKota::DirectApplicInterface::derived_map_of(const Dakota::String& ac_name)
{
  Teuchos::RCP<Teuchos::FancyOStream>
    out = Teuchos::VerboseObjectBase::getDefaultOStream();

    // WARNING:: For some reason, this method is not being called!
    *out << "Finished Dakota NLS Fitting!: " << setprecision(5) << endl;
    model_p->Print(*out << "\nParameters!\n");
    model_g->Print(*out << "\nResponses!\n");
    model_dgdp->Print(*out << "\nSensitivities!\n");

    return 0;
}
