

#
# This file contains global-level macros that are specific to Trilinos
#



#
# Macro that defines Trilinos testing support
#

MACRO(TRILINOS_SETUP_TESTING_SUPPORT)

  CONFIGURE_FILE(
    ${Trilinos_SOURCE_DIR}/cmake/ctest/CTestCustom.ctest.in
    ${Trilinos_BINARY_DIR}/CTestCustom.ctest
    )

ENDMACRO()


#
#  Function for helping set up exclude files only for the packages
#  that will not be supporting autotools.
#  Returns a list of the given file name with a path for packages
#  that are not supporting autotools anymore.
#
#  example: PACKAGE_APPLY_TO_NO_AUTOTOOLS_PACKAGES("configure.ac" list)
#    assuming that the packages epetra and teuchos are not supporting 
#    autotools anymore then the return value would be:
#    "epetra/configure.ac;teuchos/configure.ac"
#
#

FUNCTION(APPLY_TO_NO_AUTOTOOLS_PACKAGES FILE_NAME LIST_RETURN)
  SET(NON_AUTOTOOLS_PACKAGES
    /packages/amesos
    /packages/anasazi
    /packages/aztecoo
    /packages/belos
    /packages/didasko
    /packages/epetra
    /packages/epetraext
    /packages/fei
    /packages/galeri
    /packages/ifpack
    /packages/intrepid
    /packages/isorropia
    /packages/kokkos
    /packages/komplex
    /packages/meros
    /packages/ml
    /packages/moertel
    /packages/moocho
    /packages/nox
    /packages/pamgen
    /packages/phalanx
    /packages/phdmesh
    /packages/pliris
    /packages/PyTrilinos
    /packages/rtop
    /packages/rythmos
    /packages/sacado
    /packages/shards
    /packages/stratimikos
    /packages/stk
    /packages/Sundance
    /packages/teuchos
    /packages/ThreadPool
    /packages/thyra
    /packages/tpetra
    /packages/tifpack
    /packages/trilinoscouplings
    /packages/triutils
  )
  
  FOREACH(PACKAGE ${NON_AUTOTOOLS_PACKAGES})
    SET(LIST_RETURN_TMP ${LIST_RETURN_TMP} ${PACKAGE}/${FILE_NAME} ${PACKAGE}/\(.*/\)*${FILE_NAME})
  ENDFOREACH()
  
  SET(${LIST_RETURN} ${LIST_RETURN_TMP} PARENT_SCOPE)
ENDFUNCTION()

#
# Macro that defines Trilinos packaging options:
#

MACRO(TRILINOS_DEFINE_PACKAGING)
  APPLY_TO_NO_AUTOTOOLS_PACKAGES("configure.ac" CONFIGURE_AC_LIST)
  APPLY_TO_NO_AUTOTOOLS_PACKAGES("configure"    CONFIGURE_LIST)
  APPLY_TO_NO_AUTOTOOLS_PACKAGES("Makefile.am"  MAKEFILE_AM_LIST)
  APPLY_TO_NO_AUTOTOOLS_PACKAGES("Makefile.in"  MAKEFILE_AC_LIST)
  APPLY_TO_NO_AUTOTOOLS_PACKAGES(".*.m4"        M4_LIST)
  APPLY_TO_NO_AUTOTOOLS_PACKAGES("bootstrap"    BOOTSTRAP_LIST)
  APPLY_TO_NO_AUTOTOOLS_PACKAGES("config/"      CONFIG_LIST)
 
    
  SET(CPACK_SOURCE_IGNORE_FILES
    /.git/
    ".gitignore"
    classicMakefile
    /cmake/CMakeKitwareBacklog.txt
    /cmake/TODO
    /packages/CTrilinos
    /packages/ForTrilinos
    /packages/ITAPS
    /packages/globipack
    /packages/optipack
    /packages/aristos
    /packages/claps
    /packages/external
    /packages/jpetra
    /packages/new_package
    /packages/rbgen
    /packages/WebTrilinos
    /packages/cmmlib
    /packages/lyno
    /packages/stalix
    /packages/teko
    ${CONFIGURE_AC_LIST}
    ${CONFIGURE_LIST}
    ${MAKEFILE_AM_LIST}
    ${MAKEFILE_AC_LIST}
    ${M4_LIST}
    ${BOOTSTRAP_LIST}
    ${CONFIG_LIST}
    /packages/configure.ac
    /packages/configure
    /packages/Makefile.am
    /packages/Makefile.in
    /Trilinos/configure.ac
    /Trilinos/configure
    /Trilinos/Makefile.am
    /Trilinos/Makefile.in
    /Trilinos/bootstrap
    /Trilinos/config
    /Trilinos/doc/[^b]
    /Trilinos/README_old
    /sampleScripts/old_autotools
    /sampleScripts/git-profiles
    ".*.pyc"
    /SIERRA/
    /commonTools/test/coverage
    /commonTools/test/harness
    /commonTools/test/utilities/README
    /commonTools/test/utilities/dependencies
    /commonTools/test/utilities/packages
    /commonTools/test/utilities/r.*
    /commonTools/scripts
    /commonTools/git
    /packages/PyTrilinos/Notes.txt
    /packages/PyTrilinos/aclocal.m4
    /packages/PyTrilinos/bootstrap
    /packages/PyTrilinos/config
    /packages/PyTrilinos/lib
    /packages/PyTrilinos/macdist
    /packages/PyTrilinos/shared
    /packages/PyTrilinos/src/PyTrilinos_config.h.in
    /packages/PyTrilinos/src/depend
    /packages/PyTrilinos/src/setup.py
    /packages/PyTrilinos/src-boost
    /packages/zoltan/test/ch_brack2_3
    /packages/zoltan/test/ch_bug
    /packages/zoltan/test/ch_degenerate
    /packages/zoltan/test/ch_degenerateAA
    /packages/zoltan/test/ch_drake
    /packages/zoltan/test/ch_ewgt
    /packages/zoltan/test/ch_grid20x19
    /packages/zoltan/test/ch_hammond
    /packages/zoltan/test/ch_hammond2
    /packages/zoltan/test/ch_nograph
    /packages/zoltan/test/ch_onedbug
    /packages/zoltan/test/ch_random
    /packages/zoltan/test/ch_serial
    /packages/zoltan/test/ch_slac
    /packages/zoltan/test/ch_vwgt
    /packages/zoltan/test/ch_vwgt2
    /packages/zoltan/test/hg_cage10
    /packages/zoltan/test/hg_diag500_4
    /packages/zoltan/test/hg_ewgt
    /packages/zoltan/test/hg_felix
    /packages/zoltan/test/hg_ibm03
    /packages/zoltan/test/hg_ml27
    /packages/zoltan/test/hg_nograph
    /packages/zoltan/test/hg_vwgt
    /packages/zoltan/test/nem_ti_20k
    /packages/zoltan/test/nem_ti_4k
    /packages/zoltan/test/misc_siefert
    /packages/zoltan/test/th
    /packages/zoltan/test/bin
    /packages/zoltan/doc/Zoltan_html/tu_html
    /packages/zoltan/src/ZoltanComponent
    /packages/zoltan/src/driver_old
    /packages/zoltan/src/fdriver_old
    /packages/amesos/doc/AmesosOverview
    /packages/amesos/doc/PARA06
    /packages/anasazi/doc/TOMS
    /packages/anasazi/doc/OrthoStudy
    /packages/anasazi/doc/ThyraPerf
    /packages/aztecoo/doc/AZ_capture_matrix_howto.txt
    /packages/aztecoo/doc/Aztec2.0
    /packages/aztecoo/doc/Aztec2.1
    /packages/aztecoo/doc/Managing_conditioning_howto.txt
    /packages/aztecoo/doc/UserGuide
    /packages/aztecoo/doc/azteclogo.gif
    /packages/aztecoo/doc/read_captured_matrix.c
    /packages/aztecoo/example/AztecOO_RecursiveCall
    /packages/aztecoo/example/Epetra_MsrMatrix_AztecOO
    /packages/aztecoo/example/Epetra_MsrMatrix_PowerMethod
    /packages/aztecoo/example/IfpackIctAztecOO
    /packages/aztecoo/example/IfpackAztecOO
    /packages/aztecoo/example/IfpackVbrAztecOO
    /packages/aztecoo/example/MLAztecOO
    /packages/aztecoo/example/azoo_iterate_hb
    /packages/aztecoo/example/aztec_app
    /packages/aztecoo/example/aztec_hb
    /packages/galeri/src-pfem
    /packages/galeri/example-pfem
    /packages/tpetra/doc/CodingGuidelines
    /packages/tpetra/doc/TpetraDesign
    /packages/kokkos/doc
    /packages/aztecoo/example/AztecOO/adapt_main.mk
    /packages/aztecoo/example/AztecOO/vbr_main.mk
    /packages/aztecoo/example/AztecOO_MatlabInput/A.dat
    /packages/aztecoo/example/AztecOO_MatlabInput/Ainv.dat
    /packages/aztecoo/src/AztecOO_string_maps.txt
    /packages/aztecoo/src/AztecOO_string_maps_builder.pl
    /packages/aztecoo/src/az_comm_.*
    /packages/aztecoo/src/md_timer_intel.c
    /packages/aztecoo/src/md_timer_ncube.c
    /packages/aztecoo/src/md_timer_sol.c
    /packages/aztecoo/src/md_timer_sp2.c
    /packages/aztecoo/src/md_timer_sun.c
    /packages/aztecoo/src/md_timer_win2000.c
    /packages/aztecoo/src/md_wrap_intel_c.c
    /packages/aztecoo/src/md_wrap_ncube_c.c
    /packages/aztecoo/src/md_wrap_puma_c.c
    /packages/aztecoo/src/md_wrap_sp2_c.c
    /packages/aztecoo/src/stamp-h.in
    /packages/aztecoo/test/scripts/daily/serial/Ex_AztecOO_UserOpUserMat
    /packages/common/DoxyfilePackageTemplate
    /packages/didasko/examples/teuchos/xml-data
    /packages/didasko/src/.*.eps
    /packages/didasko/src/.*.tex
    /packages/didasko/src/.*.ps
    /packages/didasko/src/DOEbwlogo.pdf
    /packages/didasko/src/Makefile
    /packages/didasko/src/SANDreport.cls
    /packages/didasko/src/Trilinos60Tutorial.pdf
    /packages/didasko/src/Trilinos70Tutorial.pdf
    /packages/didasko/src/Trilinos80Tutorial.pdf
    /packages/didasko/src/TrilinosTutorial_ReviewAndApproval.doc
    /packages/didasko/src/chapterbox.pdf
    /packages/didasko/src/colabarticle.cls
    /packages/didasko/src/snllineblk.pdf
    /packages/didasko/src/tutorial_biblio.bib
    /packages/epetra/doc
    /packages/epetra/example/C_wrappers
    /packages/epetra/example/Fortran
    /packages/epetra/example/ImportExport
    /packages/epetra/example/InverseIteration
    /packages/epetra/example/MapColoring
    /packages/epetra/example/ReducedLinearProblem
    /packages/epetra/example/petra_howle
    /packages/epetra/example/petra_nonlinear
    /packages/epetra/example/petra_transpose
    /packages/epetra/src/Epetra_FastCrsMatrix.cpp
    /packages/epetra/src/Epetra_FastCrsMatrix.h
    /packages/epetra/src/Epetra_InvOperator.cpp
    /packages/epetra/src/Epetra_LinearProblemRedistor.cpp
    /packages/epetra/src/Epetra_LinearProblemRedistor.h
    /packages/epetra/src/Epetra_MpiSmpComm.*
    /packages/epetra/src/stamp-h.in
    /packages/epetra/src/xxemacs
    /packages/epetra/test/BasicPerfTest/runSummary
    /packages/epetra/test/Comm/simple_mpi.cpp
    /packages/epetra/test/Comm/threaded_Makefile
    /packages/epetra/test/Comm/threaded_main.cpp
    /packages/epetra/test/EpetraBenchmarkTest
    /packages/epetra/test/LinearProblemRedistor
    /packages/epetra/test/Makefile.template
    /packages/epetra/test/Map/c_main.c
    /packages/epetra/test/MultiVector/Makefile.purify
    /packages/epetra/test/OSKI
    /packages/epetra/test/VbrMatrix/Suppressions.in
    /packages/epetra/test/Vector/Makefile.purify
    /packages/epetra/test/testAll.*
    /packages/epetraext/doc/UserGuide
    /packages/epetraext/doc/inout
    /packages/epetraext/doc/matlab.README
    /packages/epetraext/example/MapColoring/sample_map
    /packages/epetraext/example/MapColoring/sample_matrix
    /packages/epetraext/example/inout/build
    /packages/epetraext/example/model_evaluator/GLpApp/Parallel2DMeshGeneratorFormat.pdf
    /packages/epetraext/example/model_evaluator/GLpApp/README
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/generate-serial-meshes-1-2.sh
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/generate-serial-meshes-1-2.sh.out
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.000
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.001
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.edge
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.ele
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.epart.2
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.node
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.npart.2
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.2.poly
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.edge
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.ele
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.node
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.1.poly
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.2.edge
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.2.ele
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.2.node
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.2.poly
    /packages/epetraext/example/model_evaluator/GLpApp/data/square/square.poly
    /packages/epetraext/example/model_evaluator/GLpApp/from-triangle-to-serial-input-mesh.pl
    /packages/epetraext/src/btf/pothen/btf_notes
    /packages/epetraext/src/btf/pothen/main.f
    /packages/epetraext/src/distdir
    /packages/epetraext/src/transform/EpetraExt_Dirichlet_.*
    /packages/epetraext/src/transform/EpetraExt_StaticCondensation_LinearProblem..*
    /packages/epetraext/src/transform/EpetraExt_SubCopy_CrsMatrix..*
    /packages/epetraext/src/zoltan/EpetraExt_ZoltanMpi.*
    /packages/epetraext/test/Copy
    /packages/epetraext/test/Makefile.template
    /packages/epetraext/test/Zoltan/Dummy
    /packages/epetraext/test/inout/build
    /packages/epetraext/test/testAll.*
    /packages/triutils/src/stamp-h.in
    /stamp-h.in
    /packages/galeri/doc/AdvDiffSquare.png
    /packages/galeri/doc/L.*.png
    /packages/galeri/example-fem/TwoSquares.cpp
    /packages/galeri/src-fem/Galeri_FileGrid.h
    /packages/ifpack/doc/UsersGuide
    /packages/ifpack/example/Ifpack_ex_ScalarLaplacian_FEM.cpp
    /packages/ifpack/example/Ifpack_ex_VectorLaplacian_FEM.cpp
    /packages/ifpack/example/ifpack_hb
    /packages/ifpack/example/ifpack_threaded_hb
    /packages/ifpack/src/Ifpack_CrsGraph.h
    /packages/ifpack/src/Ifpack_CrsIlut.cpp
    /packages/ifpack/src/Ifpack_CrsIlut.h
    /packages/ifpack/src/Ifpack_CrsRick.cpp
    /packages/ifpack/src/Ifpack_CrsRick.h
    /packages/ifpack/src/Ifpack_HashTable.cpp
    /packages/ifpack/src/Ifpack_OverlapFactor.*
    /packages/ifpack/src/Ifpack_OverlapSolveObject..*
    /packages/ifpack/src/Ifpack_PerturbedMatrix.h
    /packages/ifpack/src/Ifpack_SPARSKIT.cpp
    /packages/ifpack/src/az_ifpack.*
    /packages/ifpack/src/ifp_Block
    /packages/ifpack/src/ifp_DenseMat.*
    /packages/ifpack/src/ifp_GlobalPrecon.h
    /packages/ifpack/src/ifp_Local.*
    /packages/ifpack/src/ifp_Matrix.h
    /packages/ifpack/src/ifp_Precon..*
    /packages/ifpack/src/ifp_SparseUtil..*
    /packages/ifpack/src/ifp_arch.h
    /packages/ifpack/src/ifp_b.*
    /packages/ifpack/src/ifp_c_wrappers..*
    /packages/ifpack/src/ifp_ifpack.h
    /packages/ifpack/src/ifp_lapackd.h
    /packages/ifpack/src/ifp_sp.*
    /packages/ifpack/src/old.Makefile
    /packages/ifpack/src/stamp-h.in
    /packages/ifpack/src/xxemacs
    /packages/ifpack/test/PointPreconditioner
    /packages/ifpack/test/scripts
    /packages/ifpack/test/scripts/run-tests.sh
    /packages/komplex/doc/Komplex.*.vsd
    /packages/komplex/doc/header.tex
    /packages/komplex/doc/komplex.eps
    /packages/komplex/doc/komplex.gif
    /packages/komplex/doc/komplex_user_guide.ps
    /packages/komplex/example/komplex_hb/README
    /packages/komplex/example/komplex_hb/blassm.f
    /packages/komplex/example/komplex_hb/create_vbr.c
    /packages/komplex/example/komplex_hb/distrib_.*_matrix.c
    /packages/komplex/example/komplex_hb/formats.f
    /packages/komplex/example/komplex_hb/iohb.*
    /packages/komplex/example/komplex_hb/main.c
    /packages/komplex/example/komplex_hb/prototypes.h
    /packages/komplex/example/komplex_hb/read_.*
    /packages/komplex/example/komplex_hb/sc.*
    /packages/komplex/example/komplex_hb/smsrres.c
    /packages/komplex/example/komplex_hb/svbrres.c
    /packages/komplex/example/komplex_hb/unary.f
    /packages/komplex/example/komplex_hb/write_vec.c
    /packages/komplex/src/new
    /packages/komplex/src/stamp-h.in
    /packages/komplex/test/definition
    /packages/amesos/example/RunParaklete.cpp
    /packages/amesos/example/Thyra_AmesosLinearOpWithSolveFactory.cpp
    /packages/amesos/example/pk.h
    /packages/amesos/example/run_pk.c
    /packages/amesos/example/simpleStratimikosSolve.cpp
    /packages/amesos/example/simpleStratimikosSolve.hpp
    /packages/amesos/example/stratimikos_example.cpp
    /packages/amesos/src/Amesos_BTF.h
    /packages/amesos/src/Amesos_Component.h
    /packages/amesos/src/Amesos_Merikos.h
    /packages/amesos/src/Amesos_BTF.h
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_1.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_aat.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_control.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_defaults.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_dump.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_info.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_order.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_preprocess.c
    /packages/amesos/src/SuiteSparse/CAMD/Source/amesos_camd_valid.c
    /packages/amesos/src/SuiteSparse/CCOLAMD/Source/amesos_ccolamd.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_amd.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_analyze.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_colamd.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_etree.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_factorize.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_postorder.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_rcond.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_resymbol.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_rowcolcounts.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_rowfac.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_solve.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_cholmod_spsolve.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Cholesky/amesos_t_cholmod_rowfac.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Core/amesos_cholmod_aat.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Core/amesos_cholmod_add.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Core/amesos_cholmod_band.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Core/amesos_cholmod_change_factor.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Core/amesos_cholmod_copy.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Core/amesos_cholmod_dense.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Core/amesos_cholmod_factor.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Partition/amesos_cholmod_camd.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Partition/amesos_cholmod_ccolamd.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Partition/amesos_cholmod_csymamd.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Partition/amesos_cholmod_metis.c
    /packages/amesos/src/SuiteSparse/CHOLMOD/Partition/amesos_cholmod_nesdis.c
    /packages/amesos/src/src-repository
    /packages/amesos/src/stamp-h.in
    /packages/amesos/test/TestOptions/Dummy
    /packages/amesos/test/Test_Basic/NotQuiteDense.triU
    /packages/amesos/test/Test_Performance/In_Test_UmfpackPerformance.csh
    /packages/amesos/test/scripts/daily/mpi/TestBasic.sh
    /packages/amesos/test/scripts/daily/serial/TestAmesos.sh
    /packages/pliris/doc/matrix_.*.gif
    /packages/pliris/src/Make..*
    /packages/pliris/src/clean_code.h
    /packages/pliris/src/init..*
    /packages/pliris/src/malloc.c
    /packages/pliris/src/my_srand48.c
    /packages/meros/README-MEROS
    /packages/meros/doc/UsersGuide
    /packages/meros/example/data/mac
    /packages/meros/example/data/mac-vbr
    /packages/meros/example/data/q1p0
    /packages/meros/example/data/salsa
    /packages/meros/example/data/tmac-vbr
    /package/phdmesh/Make.in
    /packages/phdmesh/\(.*/\)*Make.in
    /packages/phdmesh/build_examples
    /packages/teuchos/config.h.in
    /packages/teuchos/doc/images
    /packages/teuchos/example/config.h.in
    /packages/nox/src-loca/python
    /packages/nox/test/lapack/LOCA_python
    /packages/anasazi/src/ModalAnalysisSolvers
    /packages/ml/util
    /packages/ml/etc
    /packages/ml/test/tmp
    /packages/ml/doc/UsersGuide
    /packages/ml/doc/DevelopersGuide
    /packages/ml/doc/MLAPI
    /packages/ml/python
    /packages/ml/doc/DoxyfileWeb
    /packages/ml/doc/build_docs
    /packages/ml/doc/ml-logo.eps
    /packages/ml/doc/ml-logo.jpg
    /packages/ml/doc/sc2000.ps.gz
    /packages/ml/examples/Makefile-common.include
    /packages/ml/examples/Maxwell/ml_periodic_max.c
    /packages/ml/examples/Other/ml_read_complexmaxwell.c
    /packages/ml/examples/Other/ml_read_maxwell.c
    /packages/ml/examples/Other/ml_star2d.c
    /packages/ml/examples/Other/new_readex.c
    /packages/ml/examples/Other/oldml_readex.c
    /packages/ml/examples/Other/seg_readex.c
    /packages/ml/examples/README.AddingExamples
    /packages/ml/examples/RefMaxwell
    /packages/ml/examples/RefMaxwell/rpc.cpp
    /packages/ml/src/Coarsen/README
    /packages/ml/src/Main/ml_v_cycle.c
    /packages/ml/src/Smoother/README
    /packages/ml/src/Utils/jmpilib.c
    /packages/ml/src/Utils/jostle.h
    /packages/ml/src/Utils/ml_vampir.c
    /packages/ml/src/Utils/tumi.c
    /packages/ml/test/README.runtests
    /packages/ml/test/Zoltan/cxx_main_simple.cpp
    /packages/ml/test/scripts
    /packages/ml/test/scripts/run-tests.sh
    /packages/sacado/example/FEApp/experimental
    /packages/tifpack/src/Tifpack_AMDReordering.cpp
    /packages/tifpack/src/Tifpack_AMDReordering.hpp
    /packages/tifpack/src/Tifpack_AdditiveSchwarz.hpp
    /packages/tifpack/src/Tifpack_Amesos.cpp
    /packages/tifpack/src/Tifpack_Amesos.hpp
    /packages/tifpack/src/Tifpack_BlockRelaxation.hpp
    /packages/tifpack/src/Tifpack_ConstructLevelFillGraph.hpp
    /packages/tifpack/src/Tifpack_Container.hpp
    /packages/tifpack/src/Tifpack_CrsGraph.hpp
    /packages/tifpack/src/Tifpack_DenseContainer.cpp
    /packages/tifpack/src/Tifpack_DenseContainer.hpp
    /packages/tifpack/src/Tifpack_DiagPreconditioner.cpp
    /packages/tifpack/src/Tifpack_DiagPreconditioner.hpp
    /packages/tifpack/src/Tifpack_DiagonalFilter.cpp
    /packages/tifpack/src/Tifpack_DiagonalFilter.hpp
    /packages/tifpack/src/Tifpack_DropFilter.cpp
    /packages/tifpack/src/Tifpack_DropFilter.hpp
    /packages/tifpack/src/Tifpack_EquationPartitioner.cpp
    /packages/tifpack/src/Tifpack_EquationPartitioner.hpp
    /packages/tifpack/src/Tifpack_Graph.hpp
    /packages/tifpack/src/Tifpack_Graph_Tpetra_CrsGraph.cpp
    /packages/tifpack/src/Tifpack_Graph_Tpetra_CrsGraph.hpp
    /packages/tifpack/src/Tifpack_Graph_Tpetra_RowMatrix.cpp
    /packages/tifpack/src/Tifpack_Graph_Tpetra_RowMatrix.hpp
    /packages/tifpack/src/Tifpack_GreedyPartitioner.cpp
    /packages/tifpack/src/Tifpack_GreedyPartitioner.hpp
    /packages/tifpack/src/Tifpack_HashTable.cpp
    /packages/tifpack/src/Tifpack_HashTable.hpp
    /packages/tifpack/src/Tifpack_IC.cpp
    /packages/tifpack/src/Tifpack_IC.hpp
    /packages/tifpack/src/Tifpack_ICT.cpp
    /packages/tifpack/src/Tifpack_ICT.hpp
    /packages/tifpack/src/Tifpack_IC_Utils.cpp
    /packages/tifpack/src/Tifpack_IC_Utils.hpp
    /packages/tifpack/src/Tifpack_IKLU.cpp
    /packages/tifpack/src/Tifpack_IKLU.hpp
    /packages/tifpack/src/Tifpack_IKLU_Utils.cpp
    /packages/tifpack/src/Tifpack_IKLU_Utils.hpp
    /packages/tifpack/src/Tifpack_ILU.cpp
    /packages/tifpack/src/Tifpack_ILU.hpp
    /packages/tifpack/src/Tifpack_LinearPartitioner.cpp
    /packages/tifpack/src/Tifpack_LinearPartitioner.hpp
    /packages/tifpack/src/Tifpack_LocalFilter.cpp
    /packages/tifpack/src/Tifpack_LocalFilter.hpp
    /packages/tifpack/src/Tifpack_METISPartitioner.cpp
    /packages/tifpack/src/Tifpack_METISPartitioner.hpp
    /packages/tifpack/src/Tifpack_METISReordering.cpp
    /packages/tifpack/src/Tifpack_METISReordering.hpp
    /packages/tifpack/src/Tifpack_NodeFilter.cpp
    /packages/tifpack/src/Tifpack_NodeFilter.hpp
    /packages/tifpack/src/Tifpack_OverlapFactor.cpp
    /packages/tifpack/src/Tifpack_OverlapFactorObject.hpp
    /packages/tifpack/src/Tifpack_OverlapSolveObject.cpp
    /packages/tifpack/src/Tifpack_OverlapSolveObject.hpp
    /packages/tifpack/src/Tifpack_OverlappingPartitioner.cpp
    /packages/tifpack/src/Tifpack_OverlappingPartitioner.hpp
    /packages/tifpack/src/Tifpack_OverlappingRowMatrix.cpp
    /packages/tifpack/src/Tifpack_OverlappingRowMatrix.hpp
    /packages/tifpack/src/Tifpack_Partitioner.hpp
    /packages/tifpack/src/Tifpack_PerturbedMatrix.hpp
    /packages/tifpack/src/Tifpack_RCMReordering.cpp
    /packages/tifpack/src/Tifpack_RCMReordering.hpp
    /packages/tifpack/src/Tifpack_ReorderFilter.cpp
    /packages/tifpack/src/Tifpack_ReorderFilter.hpp
    /packages/tifpack/src/Tifpack_Reordering.hpp
    /packages/tifpack/src/Tifpack_SPARSKIT.cpp
    /packages/tifpack/src/Tifpack_SPARSKIT.hpp
    /packages/tifpack/src/Tifpack_SingletonFilter.cpp
    /packages/tifpack/src/Tifpack_SingletonFilter.hpp
    /packages/tifpack/src/Tifpack_SparseContainer.hpp
    /packages/tifpack/src/Tifpack_SparsityFilter.cpp
    /packages/tifpack/src/Tifpack_SparsityFilter.hpp
    /packages/tifpack/src/Tifpack_UserPartitioner.cpp
    /packages/tifpack/src/Tifpack_UserPartitioner.hpp
    /packages/tifpack/src/Tifpack_Utils.cpp
    /packages/tifpack/src/Tifpack_Utils.hpp
  )
  
  IF(${PROJECT_NAME}_VERBOSE_CONFIGURE)
    MESSAGE("Exclude files when building source packages")
    FOREACH(item ${CPACK_SOURCE_IGNORE_FILES})
      MESSAGE(${item})
    ENDFOREACH()
  ENDIF()
  

  SET(CPACK_PACKAGE_DESCRIPTION "Trilinos provides algorithms and technologies for the solution of large-scale, complex multi-physics engineering and scientific problems.")
  SET(CPACK_PACKAGE_FILE_NAME "trilinos-setup-${Trilinos_VERSION}")
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Trilinos ${Trilinos_VERSION}")
  SET(CPACK_PACKAGE_REGISTRY_KEY "Trilinos ${Trilinos_VERSION}")
  SET(CPACK_PACKAGE_NAME "trilinos")
  SET(CPACK_PACKAGE_VENDOR "Sandia National Laboratories")
  SET(CPACK_PACKAGE_VERSION "${Trilinos_VERSION}")
  SET(CPACK_RESOURCE_FILE_README "${Trilinos_SOURCE_DIR}/README")
  SET(CPACK_RESOURCE_FILE_LICENSE "${Trilinos_SOURCE_DIR}/README")
  SET(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
  SET(CPACK_SOURCE_FILE_NAME "trilinos-source-${Trilinos_VERSION}")
  SET(CPACK_COMPONENTS_ALL ${Trilinos_PACKAGES})
  
  PACKAGE_ARCH_GET_ENABLED_LIST( Trilinos_PACKAGES Trilinos ON
    FALSE ENABLED_PACKAGES NUM_ENABLED)
  string(REPLACE " " ";" ENABLED_PACKAGES "${ENABLED_PACKAGES}")
  
  #message("ENABLED PACKAGES: ${ENABLED_PACKAGES} ${NUM_ENABLED}")
  FOREACH(PKG ${ENABLED_PACKAGES})
    IF(NOT "${${PKG}_LIB_REQUIRED_DEP_PACKAGES}" STREQUAL "")
        string(TOUPPER ${PKG} UPPER_PKG)
        #message("${UPPER_PKG} depends on : ${${PKG}_LIB_REQUIRED_DEP_PACKAGES}")
        SET(CPACK_COMPONENT_${UPPER_PKG}_DEPENDS ${${PKG}_LIB_REQUIRED_DEP_PACKAGES})
    ENDIF()
    #message("${PKG} depends on : ${${PKG}_LIB_REQUIRED_DEP_PACKAGES}")
  ENDFOREACH()

  
  IF(WIN32)
    SET(CPACK_GENERATOR "NSIS")
    SET(CPACK_NSIS_MODIFY_PATH OFF)
  ENDIF()
  
  INCLUDE(CPack)

ENDMACRO()
