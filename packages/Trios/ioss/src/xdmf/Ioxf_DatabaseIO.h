 /*--------------------------------------------------------------------*/
/*    Copyright 2000 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/

// -*- Mode: c++ -*-
#ifndef IOSS_Ioxf_DatabaseIO_h
#define IOSS_Ioxf_DatabaseIO_h

#include <Ioss_DatabaseIO.h>
#include <Ioss_Field.h>
#include <Ioss_DBUsage.h>
#include <Ioss_Map.h>
#include <Ioss_Utils.h>
#include <Ioss_FileInfo.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <XdmfArray.h>
#include <XdmfHDF.h>
#include <xdmf/Ioxf_Internals.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


namespace Ioss {
   class GroupingEntity;
   class Region;
   class EntityBlock;
   class NodeBlock;
   class FaceBlock;
   class ElementBlock;
   class NodeSet;
   class EdgeSet;
   class FaceSet;
   class CommSet;
}

namespace Ioxf {
  struct CommunicationMetaData;

  // Used for variable name index mapping
  typedef std::map<std::string, int, std::less<std::string> > VariableNameMap;
  typedef VariableNameMap::value_type VNMValuePair;

  // Used to store reduction variables
  typedef std::vector<double> ValueContainer;

  // Use to store element block info for XML
  typedef std::vector<std::ostringstream *> XmlContainer;
  typedef std::map<std::string, int, std::less<std::string> > IndexNameMap;
  typedef IndexNameMap::value_type BINMValuePair;

  // Used for persistent entity IDs
  // The set contains a pair of <char, int>. The 'char' denotes the entity type:
  // 'E' element block, 'N' nodeset, 'S' sideset, 'C' commset, 'B' nodeblock.
  // The 'int' is the entity id.
  // The set is used for output databases to ensure that there are no id collisions.
  typedef std::set<std::pair<char, int> > EntityIdSet;

  class DatabaseIO : public Ioss::DatabaseIO
    {
    public:
      DatabaseIO(Ioss::Region *region, const std::string& filename,
		 Ioss::DatabaseUsage db_usage, MPI_Comm communicator);
      ~DatabaseIO();
      static void finalize();

      void InitXML(std::ostringstream *XML);
      void WriteMetaXdmfNodesets(std::ostringstream *XML, const std::vector<Ioxf::NodeSet> &nodesets);
      void WriteMetaXdmfSidesets(std::ostringstream *XML, const std::vector<Ioxf::SideSet> &sidesets);
      void WriteMetaXdmfElementBlock(const std::vector<Ioxf::Block> &blocks);
      void WriteXmlFile(int niterations);
      void WriteHdfDataset(const std::string &FinalName, XdmfArray *ScalarArray, int lineno) const;
      std::string decode_proc_filename(const std::string &filename,  int processor);
      void MergeXmlFiles();

      // Check capabilities of input/output database...
      bool supports_nodal_fields()    const {return false;}
      bool supports_edge_fields()     const {return false;}
      bool supports_face_fields()     const {return false;}
      bool supports_element_fields()  const {return true;}
      bool supports_nodelist_fields() const {return false;}

      int    node_local_to_global(int local)  const;

      /*!
       * Determine the local position of the node with the global id
       * 'global'.  If 'must_exist' is false, then the global id possibly
       * does not exist in the map; otherwise, it must exist and will
       * throw an exception if not found.
       */
      int    node_global_to_local(int global, bool must_exist) const;
      int element_local_to_global(int local)  const;
      int element_global_to_local(int global) const;

      bool begin(Ioss::State state);
      bool   end(Ioss::State state);

      bool begin_state(Ioss::Region *region, int state, double time);
      bool   end_state(Ioss::Region *region, int state, double time);

      std::string title()               const     {return databaseTitle;}
      int    spatial_dimension()   const     {return spatialDimension;}
      int    node_count()          const     {return nodeCount;}
      int    edge_count()          const     {return 0;}
      int    face_count()          const     {return 0;}
      int    element_count()       const     {return elementCount;}
      int    node_block_count()    const     {return nodeBlockCount;}
      int    element_block_count() const     {return elementBlockCount;}
      int    faceset_count()       const     {return sidesetCount;}
      int    edgeset_count()       const     {return 0;}
      int    nodeset_count()       const     {return nodesetCount;}
      int    maximum_symbol_length() const {return 32;}

    private:
      int get_field_internal(const Ioss::Region* reg, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int get_field_internal(const Ioss::ElementBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::FaceBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::EdgeBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::NodeBlock* nb, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int get_field_internal(const Ioss::NodeSet* ns, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::EdgeSet* es, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::FaceSet* fs, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::CommSet* cs, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int put_field_internal(const Ioss::Region* reg, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int put_field_internal(const Ioss::ElementBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::FaceBlock* fb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::EdgeBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::NodeBlock* nb, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int put_field_internal(const Ioss::NodeSet* ns, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::EdgeSet* es, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::FaceSet* fs, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::CommSet* cs, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      // Private member functions
      DatabaseIO(const DatabaseIO& from); // do not implement
      DatabaseIO& operator=(const DatabaseIO& from); // do not implement

      virtual void openDatabase() const {}

      virtual void closeDatabase() const {}

      void put_qa();
      void put_info();
      int read_nodal_coordinates();
      void read_elements(const Ioss::ElementBlock& block);

      // Metadata-related functions.
      void read_meta_data();
      void read_communication_metadata();

      int read_transient_field(const char *type, const Ioss::Field& field,
			       const Ioss::GroupingEntity *ge,
			       void *variables) const;
      // Should be made more generic again so can rejoin with write_element_transient field
      void write_nodal_transient_field(const char *type, const Ioss::Field& field,
				       const Ioss::NodeBlock *ge,
				       int count, void *data) const;
      // Should be made more generic again so can rejoin with write_nodal_transient field
      void write_entity_transient_field(const char *type, const Ioss::Field& field,
					const Ioss::GroupingEntity *ge,
					int count, void *data) const;
      void write_meta_data();
      void gather_communication_metadata(Ioxf::CommunicationMetaData *meta);
      void write_results_metadata();

      int  get_xml_stream(const std::string &block_name) const;

      void generate_var_xmltable(const char *type);
      void generate_element_var_xmltable();
      void generate_nodeset_var_xmltable();
      void generate_sideset_var_xmltable();

      int  gather_names(const char *type, const Ioss::GroupingEntity *ge,
			int index, bool reduction);

      // Read related metadata and store it in the region...
      void read_region();
      void get_step_times();
      void get_nodeblocks() const;
      void get_elemblocks() const;
      void get_facesets()   const;
      void get_edgesets()   const;
      void get_edge_face_sets(int topology_dimension)   const;
      void get_nodesets()   const;
      void get_commsets()   const;

      // ID Mapping functions.
      const Ioss::MapContainer& get_node_map()            const;
      const Ioss::MapContainer& get_element_map()         const;

      // Internal data handling
      void build_element_reorder_map(int start, int count);
      void build_node_reorder_map(int *new_ids, int count);

      int handle_node_ids(int* ids, int num_to_get);
      int handle_element_ids(const Ioss::ElementBlock *eb, int* ids, int num_to_get);

      int add_results_fields(char const *type, Ioss::GroupingEntity *entity,
			     int count, int id=0) const;
      Ioss::Field get_next_field(char** names, int *index, int num_names,
				 int count, int* optional_truth_table) const;
      int get_face_connectivity(const Ioss::EntityBlock* fb, int id, int face_count,
				int *fconnect,
				size_t data_size) const;
      int get_face_distributions(const Ioss::EntityBlock* fb, int id,
				 int face_count, 
				 double *fconnect, size_t data_size) const;

      void add_region_fields() const;
      void store_reduction_field(const char *type,
				 const Ioss::Field& field,
				 const Ioss::GroupingEntity *ge,
				 void *data) const;

      void get_reduction_field(const char *type,
			       const Ioss::Field& field,
			       const Ioss::GroupingEntity *ge,
			       int count, void *variables) const;
      void write_reduction_fields() const;
      void read_reduction_fields() const;

      int get_face_edge_field(const Ioss::EntityBlock* ef_blk,
			      const Ioss::Field& field,
			      void *data, size_t data_size) const;
      int put_face_edge_field(const Ioss::EntityBlock* fb,
			      const Ioss::Field& field,
			      void *data, size_t data_size) const;

      // Handle special output time requests -- primarily restart (cycle, keep, overwrite)
      // Given the global region step, return the step on the database...
      int get_database_step(int global_step) const;

      // xdmf variables
      Ioss::FileInfo hdfname;
      Ioss::FileInfo xmlname;
      XdmfHDF    *Hdf;
      std::ostringstream *MainXML;
      XmlContainer BlockGridXmls;
      XmlContainer BlockParameterXmls;
      XmlContainer BlockXmls;
      XmlContainer BlockElementVarXmls;
      XmlContainer BlockExtraAttributeXmls;
      XmlContainer BlockNodeVarXmls;
      XmlContainer BlockFinishXmls;
      mutable IndexNameMap BlockIndexNames;
      IndexNameMap SideSetIndexNames;
      IndexNameMap NodeSetIndexNames;
      int NumOfIterations;

      // Private member data...
      mutable EntityIdSet ids_;

      std::string databaseTitle;
      int spatialDimension;
      int nodeCount;
      int elementCount;

      int nodeBlockCount;
      int elementBlockCount;
      int nodesetCount;
      int sidesetCount;

      // Communication Set Data
      int *nodeCmapIds;
      int *nodeCmapNodeCnts;
      int *elemCmapIds;
      int *elemCmapElemCnts;
      int commsetNodeCount;
      int commsetElemCount;

      int *elementTruthTable;
      int *nodesetTruthTable;
      int *sidesetTruthTable;

      // Bulk Data

      // MAPS -- Used to convert from local exodusII ids/names to Sierra
      // database global ids/names

      //---Node Map -- Maps internal (1..NUMNP) ids to global ids used on the
      //               sierra side.   global = nodeMap[local]
      // nodeMap[0] contains: -1 if sequential, 0 if ordering unknown, 1
      // if nonsequential
      Ioss::MapContainer        nodeMap;
      Ioss::MapContainer        reorderNodeMap;
      Ioss::ReverseMapContainer reverseNodeMap;
      bool sequentialNG2L; // true if reverse node map is sequential
      // (local==global)

      //---Element Map -- Maps internal (1..NUMEL) ids to global ids used on the
      //               sierra side.   global = elementMap[local]
      // elementMap[0] contains: -1 if sequential, 0 if ordering unknown,
      // 1 if nonsequential
      Ioss::MapContainer        elementMap;
      Ioss::MapContainer        reorderElementMap;
      Ioss::ReverseMapContainer reverseElementMap;
      bool sequentialEG2L; // true if reverse element map is sequential

      // (local==global)

      // --- Nodal/Element/Attribute Variable Names -- Maps from sierra
      // field names to index of nodal/element/attribute variable in
      // exodusII. Note that the component suffix of the field is added on
      // prior to searching the map for the index.  For example, given the
      // Sierra field 'displ' which is a VECTOR_3D, the names stored in
      // 'elementMap' would be 'displ_x', 'displ_y' and 'displ_z'.  All
      // names are converted to lowercase.

      VariableNameMap nodalVariables;
      VariableNameMap elementVariables;
      VariableNameMap elementAttributes;
      VariableNameMap nodesetVariables;
      VariableNameMap sidesetVariables;
      VariableNameMap globalVariables;
      mutable ValueContainer  globalValues;

      mutable bool fileExists; // False if file has never been opened/created
    };

  // ------------------------------------------------------------------------
  // Node and Element mapping functions.  The ExodusII database
  // stores ids in a local-id system (1..NUMNP), (1..NUMEL) but
  // Sierra wants entities in a global system. These routines
  // take care of the mapping from local <-> global

  typedef std::vector<Ioss::IdPair>::const_iterator RMapI;
  inline int DatabaseIO::node_global_to_local(int global, bool must_exist) const
    {
      if (nodeMap.empty()) {
	get_node_map();
      }
      int local = global;
      if (!sequentialNG2L) {
	std::pair<RMapI, RMapI> iter = std::equal_range(reverseNodeMap.begin(),
							reverseNodeMap.end(),
							global,
							Ioss::IdPairCompare());
	if (iter.first != iter.second)
	  local = (iter.first)->second;
	else
	  local = 0;
	if (must_exist && iter.first == iter.second) {
	  std::ostringstream errmsg;
	  errmsg << "Node with global id equal to " << global
		 << " does not exist in this mesh on this processor\n";
	  IOSS_ERROR(errmsg);
	}
      } else if (!must_exist && global > nodeCount) {
	local = 0;
      }
      if (local > nodeCount || (local <= 0 && must_exist)) {
	std::ostringstream errmsg;
	errmsg << "Node with global id equal to " << global
	       << " returns a local id of " << local
	       << " which is invalid. This should not happen, please report.\n";
	IOSS_ERROR(errmsg);
      }
      return local;
    }

  inline int DatabaseIO::element_global_to_local(int global) const
    {
      if (elementMap.empty()) {
	get_element_map();
      }
      int local = global;
      if (!sequentialEG2L) {
	std::pair<RMapI, RMapI> iter = std::equal_range(reverseElementMap.begin(),
							reverseElementMap.end(),
							global,
							Ioss::IdPairCompare());
	if (iter.first == iter.second) {
	  std::ostringstream errmsg;
	  errmsg << "Element with global id equal to " << global
		 << " does not exist in this mesh on this processor\n";
	  IOSS_ERROR(errmsg);
	}
	local = (iter.first)->second;
      }
      if (local > elementCount || local <= 0) {
	std::ostringstream errmsg;
	errmsg << "Element with global id equal to " << global
	       << " returns a local id of " << local
	       << " which is invalid. This should not happen, please report.\n";
	IOSS_ERROR(errmsg);
      }
      return local;
    }
}
#endif
