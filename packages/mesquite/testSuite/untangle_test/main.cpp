// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-
//
//   SUMMARY: 
//     USAGE:
//
// ORIG-DATE: 19-Feb-02 at 10:57:52
//  LAST-MOD: 30-Aug-02 at 21:17:07 by Thomas Leurent
//
//
// DESCRIPTION:
// ============
/*! \file main.cpp

describe main.cpp here

 */
// DESCRIP-END.
//

#ifdef USE_STD_INCLUDES
#include <iostream>
#else
#include <iostream.h>
#endif

#ifdef USE_C_PREFIX_INCLUDES
#include <cstdlib>
#else
#include <stdlib.h>
#endif


#include "Mesquite.hpp"
#ifdef MESQUITE_USES_TSTT
#include "TSTT_Base.h"
#include "MesquiteUtilities.hpp" //  for writeShowMeMesh()
#else
#include "TSTT_C.h"
#include "CubitMesh.h" //  for writeShowMeMesh()
#endif
#include "MesquiteError.hpp"
#include "Vector3D.hpp"
#include "InstructionQueue.hpp"
#include "MeshSet.hpp"
#include "PatchData.hpp"
#include "StoppingCriterion.hpp"
#include "QualityAssessor.hpp"

// algorythms
#include "MeanRatioQualityMetric.hpp"
#include "UntangleBetaQualityMetric.hpp"
#include "LPTemplate.hpp"
#include "LInfTemplate.hpp"
#include "SteepestDescent.hpp"
#include "ConjugateGradient.hpp"
using namespace Mesquite;


#undef __FUNC__
#define __FUNC__ "main"
int main()
{     
  char file_name[128];
#ifdef MESQUITE_USES_TSTT  
    /* Reads a TSTT Mesh file */
  TSTT::Mesh_Handle mesh;
  TSTT::MeshError tstt_err;
  TSTT::Mesh_Create(&mesh, &tstt_err);
  strcpy(file_name, "../../meshFiles/3D/VTK/hex_5_tangled.vtk");
  TSTT::Mesh_Load(mesh, file_name, &tstt_err);
#else
  Mesh_Handle mesh;
  strcpy(file_name, "../MeshFiles/CUBIT/2D/bad_circle");
  TSTT_Mesh_loadFile(&mesh, file_name);
#endif
  
    // Mesquite error object
  MsqError err;
  
    // initialises a MeshSet object
  MeshSet mesh_set1;
  mesh_set1.add_mesh(mesh, err); MSQ_CHKERR(err);
#ifdef MESQUITE_USES_TSTT  
#else
  mesh_set1.set_space_dim(2);
  mesh_set1.set_element_type(Mesquite::TRIANGLE);
#endif
  
    // creates an intruction queue
  InstructionQueue queue1;

    // creates a mean ratio quality metric ...
  ShapeQualityMetric* mean_ratio = MeanRatioQualityMetric::create_new();
  UntangleQualityMetric* untangle = UntangleBetaQualityMetric::create_new(.1);
  
    // ... and builds an objective function with it
    //LInfTemplate* obj_func = new LInfTemplate(mean_ratio);
  LPTemplate* obj_func = new LPTemplate(untangle, 2, err);
    // creates the steepest descent optimization procedures
  ConjugateGradient* pass1 = new ConjugateGradient( obj_func );
  
  QualityAssessor stop_qa=QualityAssessor(mean_ratio,QualityAssessor::MINIMUM);
  stop_qa.add_quality_assessment(untangle,QualityAssessor::ALL_MEASURES,err);
  stop_qa.set_stopping_assessment(untangle,QualityAssessor::MAXIMUM,err);
    // **************Set stopping criterion**************
    //untangle beta should be 0 when untangled
  StoppingCriterion sc1(&stop_qa,-1.0,.0000001);
  StoppingCriterion sc2(StoppingCriterion::NUMBER_OF_PASSES,10);
    //either until untangled or 10 iterations
  CompositeOrStoppingCriterion sc(&sc1,&sc2);
  pass1->set_stopping_criterion(&sc);
    // sets a culling method on the first QualityImprover
  pass1->add_culling_method(QualityImprover::NO_BOUNDARY_VTX);
    // adds 1 pass of pass1 to mesh_set1
  queue1.set_master_quality_improver(pass1, err); MSQ_CHKERR(err);

#ifdef MESQUITE_USES_TSTT
  writeVtkMesh("original_mesh", mesh, err); MSQ_CHKERR(err);
#else
  writeShowMeMesh("original_mesh", mesh);
#endif
  
    // launches optimization on mesh_set1
  queue1.run_instructions(mesh_set1, err); MSQ_CHKERR(err);
  
#ifdef MESQUITE_USES_TSTT
  writeVtkMesh("smoothed_mesh", mesh, err); MSQ_CHKERR(err);
#else
  writeShowMeMesh("smoothed_mesh", mesh);
#endif

}
