#ifndef __PB_BlockingEpetra_hpp__
#define __PB_BlockingEpetra_hpp__

#include "Teuchos_RCP.hpp"

// include basic Epetra information
#include "Epetra_Map.h"
#include "Epetra_Comm.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_Vector.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Export.h"
#include "Epetra_Import.h"
#include "Epetra_IntVector.h"

#include <vector>

namespace PB {
namespace Epetra {
namespace Blocking {

typedef std::pair<Teuchos::RCP<Epetra_Map>,Teuchos::RCP<Epetra_Map> > MapPair;
typedef std::pair<Teuchos::RCP<Epetra_Import>,Teuchos::RCP<Epetra_Export> > ImExPair;

/** Build maps to make other conversions. This function builds a map 
  * using a vector of global ids local to this processor.  It also builds
  * a seperate map that (globally) starts from zero. For instance if the following
  * GIDs are passed in PID = 0, GID = [0 2 4 6 8] and PID = 1, GID = [10 12 14]
  * the the two maps created are
  *    Global Map = [(PID=0,GID=[0 2 4 6 8]), (PID=1,GID=[10 12 14])]
  *    Contiguous Map = [(PID=0,GID=[0 1 2 3 4]), (PID=1,GID=[5 6 7])]
  *
  * \param[in] gid Local global IDs to use
  * \param[in] comm Communicator to use in construction of the maps
  *
  * \returns A pair of maps: (Global Map, Contiguous Map)
  */
const MapPair buildSubMap(const std::vector< int > & gid, const Epetra_Comm &comm);

/** Build the Export/Import objects that take the single vector global map and 
  * build individual sub maps.
  *
  * \param[in] baseMap Single global vector map.
  * \param[in] maps Pair of maps containing the global sub map and the contiguous sub map.
  *                 These come directly from <code>buildSubMap</code>.
  *
  * \returns A pair containing pointers to the Import/Export objects.
  */
const ImExPair buildExportImport(const Epetra_Map & baseMap,const MapPair & maps);

/** Copy the contents of many sub vectors (created from a contigous sub maps) to a single global
  * vector. This should have the map used to create the Export/Import objects in the <code>buildExportImport</code>
  * function. If more then one sub vector contains values for a particular GID in the single vector
  * then the value in the final vector will be that of the last sub vector (in the list <code>many</code>).
  *
  * \param[in,out] one The single vector to be filled by this operation.
  * \param[in] many Sub-vectors created by <code>buildSubVectors</code> used to fill <code>one</code>.
  * \param[in] subExport A list of export objects to use in copying.
  */
void many2one(Epetra_MultiVector & one, const std::vector<Teuchos::RCP<const Epetra_MultiVector> > & many,
                                        const std::vector<Teuchos::RCP<Epetra_Export> > & subExport);

/** Copy the contents of a global vector into many sub-vectors created by <code>buildSubVectors</code>.
  *
  * \param[in,out] many Sub-vector to be filled by this operation created by <code>buildSubVectors</code>.
  * \param[in] single The source vector.
  * \param[in] subImport A list of import objects to use in copying.
  */
void one2many(std::vector<Teuchos::RCP<Epetra_MultiVector> > & many, const Epetra_MultiVector & single,
                                                            const std::vector<Teuchos::RCP<Epetra_Import> > & subImport);

/** Using a list of map pairs created by <code>buildSubMap</code>, buidl the corresponding
  * multi-vector objects.
  *
  * \param[in] maps Map pairs created by <code>buildSubMap</code>
  * \param[in,out] vectors Vector objects created using the Contiguous maps
  * \param[in] count Number of multivectors to build.
  */
void buildSubVectors(const std::vector<MapPair> & maps,
                     std::vector<Teuchos::RCP<Epetra_MultiVector> > & vectors,int count);

/** This function will return an IntVector that is constructed with a column map.
  * The vector will be filled with -1 if there is not a corresponding entry in the
  * sub-block row map. The other columns will be filled with the contiguous row map
  * values.
  */
Teuchos::RCP<Epetra_IntVector> getSubBlockColumnGIDs(const Epetra_CrsMatrix & A,const MapPair & mapPair);

/** Extract the (i,j) sub block described by a vector of map pair objects from
  * a CRS matrix. The first of map in the ith pair describes the rows to be extracted,
  * while the second pair defines GID numbering of the new block. Similarly with the
  * jth pair (except for columns).
  *
  * \param[in] i Index of the map to use to define the row space
  * \param[in] j Index of the map to use to define the columns space
  * \param[in] A Source CRS matrix
  * \param[in] subMaps Vector of maps created by the <code>buildSubMap</code> routine.
  *
  * \returns A CRS matrix containing a subset of the rows and columns as described
  *          by the vector of maps.
  */
Teuchos::RCP<Epetra_CrsMatrix> buildSubBlock(int i,int j,const Epetra_CrsMatrix & A,const std::vector<MapPair> & subMaps);

/** Extract the (i,j) sub block described by a vector of map pair objects from
  * a CRS matrix. The first of map in the ith pair describes the rows to be extracted,
  * while the second pair defines GID numbering of the new block. Similarly with the
  * jth pair (except for columns). This "refills" an operator already created by 
  * <code>buildSubBlock</code>. This only replaces the sparsity pattern of the matrix.
  *
  * \param[in] i Index of the map to use to define the row space
  * \param[in] j Index of the map to use to define the columns space
  * \param[in] A Source CRS matrix
  * \param[in] subMaps Vector of maps created by the <code>buildSubMap</code> routine.
  * \param[in,out] mat Destination matrix with a fixed nonzero pattern. Most likely
  *                    this operator would come from the <code>buildSubBlock</code> routine.
  */
void rebuildSubBlock(int i,int j,const Epetra_CrsMatrix & A,const std::vector<MapPair> & subMaps,Epetra_CrsMatrix & mat);

} // end Blocking
} // end Epetra
} // end PB

#endif
