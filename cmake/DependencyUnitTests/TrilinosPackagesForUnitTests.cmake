
# This list is just used for unit testing the dependency handling
# CMake code.  The reason that we have a separate list is so that we
# can keep very stable unit tests.


SET( Trilinos_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS
  Teuchos          ../cmake/DependencyUnitTests/packages/teuchos                PS
  RTOp             ../cmake/DependencyUnitTests/packages/rtop                   PS
  Epetra           ../cmake/DependencyUnitTests/packages/epetra                 PS 
  Zoltan           ../cmake/DependencyUnitTests/packages/zoltan                 PS
  Shards           ../cmake/DependencyUnitTests/packages/shards                 PS
  Triutils         ../cmake/DependencyUnitTests/packages/triutils               PS
  Tpetra           ../cmake/DependencyUnitTests/packages/tpetra                 PS
  EpetraExt        ../cmake/DependencyUnitTests/packages/epetraext              PS
  Stokhos          ../cmake/DependencyUnitTests/packages/stokhos                EX
  Sacado           ../cmake/DependencyUnitTests/packages/sacado                 PS
  Thyra            ../cmake/DependencyUnitTests/packages/thyra                  PS
  Isorropia        ../cmake/DependencyUnitTests/packages/isorropia              PS
  AztecOO          ../cmake/DependencyUnitTests/packages/aztecoo                PS
  Galeri           ../cmake/DependencyUnitTests/packages/galeri                 PS
  Amesos           ../cmake/DependencyUnitTests/packages/amesos                 PS
  Intrepid         ../cmake/DependencyUnitTests/packages/intrepid               PS
  Ifpack           ../cmake/DependencyUnitTests/packages/ifpack                 PS
  ML               ../cmake/DependencyUnitTests/packages/ml                     PS
  Belos            ../cmake/DependencyUnitTests/packages/belos                  PS
  Stratimikos      ../cmake/DependencyUnitTests/packages/stratimikos            PS
  RBGen            ../cmake/DependencyUnitTests/packages/rbgen                  PS
  Phalanx          ../cmake/DependencyUnitTests/packages/phalanx                SS
  )

# NOTE: Sacado is really PS but for testing purpose it is make SS
