#ifndef XPETRA_STRIDEDEPETRAMAP_HPP
#define XPETRA_STRIDEDEPETRAMAP_HPP

#include "Xpetra_EpetraConfigDefs.hpp"

#include "Xpetra_StridedMap.hpp"
#include "Xpetra_EpetraMap.hpp"

#include "Xpetra_Utils.hpp"

#include "Xpetra_EpetraUtils.hpp"
#include "Xpetra_ConfigDefs.hpp"

namespace Xpetra {


  class StridedEpetraMap
    : public virtual EpetraMap, public virtual StridedMap<int, int>
  {

    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Map constructor with Tpetra-defined contiguous uniform distribution. The elements are distributed among nodes so that the subsets of global elements are non-overlapping and contiguous and as evenly distributed across the nodes as possible.
    StridedEpetraMap(global_size_t numGlobalElements, GlobalOrdinal indexBase, std::vector<size_t>& stridingInfo, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, LocalOrdinal stridedBlockId=-1, LocalGlobal lg=GloballyDistributed, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map constructor with a user-defined contiguous distribution. The elements are distributed among the nodes so that the subsets of global elements are non-overlapping and contiguous.
    StridedEpetraMap(global_size_t numGlobalElements, size_t numLocalElements, GlobalOrdinal indexBase, std::vector<size_t>& stridingInfo, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, LocalOrdinal stridedBlockId=-1, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map constructor with user-defined non-contiguous (arbitrary) distribution.
    //StridedEpetraMap(global_size_t numGlobalElements, const Teuchos::ArrayView< const GlobalOrdinal > &elementList, GlobalOrdinal indexBase, std::vector<size_t>& stridingInfo, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, LocalOrdinal stridedBlockId=-1, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map destructor.
    ~StridedEpetraMap() { }

    //@}

    //! @name Map Attribute Methods
    //@{

    //! Returns the number of elements in this Map.
    global_size_t getGlobalNumElements() const { return EpetraMap::getGlobalNumElements(); }

    //! Returns the number of elements belonging to the calling node.
    size_t getNodeNumElements() const { return EpetraMap::getNodeNumElements(); }

    //! Returns the index base for this Map.
    GlobalOrdinal getIndexBase() const { return EpetraMap::getIndexBase(); }

    //! Returns minimum local index.
    LocalOrdinal getMinLocalIndex() const { return EpetraMap::getMinLocalIndex(); }

    //! Returns maximum local index.
    LocalOrdinal getMaxLocalIndex() const { return EpetraMap::getMaxLocalIndex(); }

    //! Returns minimum global index owned by this node.
    GlobalOrdinal getMinGlobalIndex() const { return EpetraMap::getMinGlobalIndex(); }

    //! Returns maximum global index owned by this node.
    GlobalOrdinal getMaxGlobalIndex() const { return EpetraMap::getMaxGlobalIndex(); }

    //! Return the minimum global index over all nodes.
    GlobalOrdinal getMinAllGlobalIndex() const { return EpetraMap::getMinAllGlobalIndex(); }

    //! Return the maximum global index over all nodes.
    GlobalOrdinal getMaxAllGlobalIndex() const { return EpetraMap::getMaxAllGlobalIndex(); }

    //! Return the local index for a given global index.
    LocalOrdinal getLocalElement(GlobalOrdinal globalIndex) const { return EpetraMap::getLocalElement(globalIndex); }

    //! Returns the node IDs and corresponding local indices for a given list of global indices.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList, const Teuchos::ArrayView< LocalOrdinal > &LIDList) const { return EpetraMap::getRemoteIndexList(GIDList, nodeIDList, LIDList); }

    //! Returns the node IDs for a given list of global indices.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList) const { return EpetraMap::getRemoteIndexList(GIDList, nodeIDList); }

    //! Return a list of the global indices owned by this node.
    Teuchos::ArrayView< const GlobalOrdinal > getNodeElementList() const { return EpetraMap::getNodeElementList(); }

    //! Returns true if the local index is valid for this Map on this node; returns false if it isn't.
    bool isNodeLocalElement(LocalOrdinal localIndex) const { return EpetraMap::isNodeLocalElement(localIndex); }

    //! Returns true if the global index is found in this Map on this node; returns false if it isn't.
    bool isNodeGlobalElement(GlobalOrdinal globalIndex) const { return EpetraMap::isNodeGlobalElement(globalIndex); }

    //! Returns true if this Map is distributed contiguously; returns false otherwise.
    bool isContiguous() const { return EpetraMap::isContiguous(); }

    //! Returns true if this Map is distributed across more than one node; returns false otherwise.
    bool isDistributed() const { return EpetraMap::isDistributed(); }

    //@}

    //! @name 
    //@{

    //! Get the Comm object for this Map.
    const Teuchos::RCP< const Teuchos::Comm< int > >  getComm() const { return EpetraMap::getComm(); }

    //! Get the Node object for this Map.
    const Teuchos::RCP< Node >  getNode() const { return EpetraMap::getNode(); }

    //@}


    //! @name Boolean Tests
    //@{

    //! Returns true if map is compatible with this Map.
    bool isCompatible(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const { return EpetraMap::isCompatible(map); }

    //! Returns true if map is identical to this Map.
    bool isSameAs(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const { return EpetraMap::isSameAs(map); }

    //@}

    //! @name 
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const;

    //! Print the object with some verbosity level to a FancyOStream object.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const;

    //@}

     //! Return the global index for a given local index.  Note that this returns -1 if not found on this processor.  (This is different than Epetra's behavior!)
    GlobalOrdinal getGlobalElement(LocalOrdinal localIndex) const { return EpetraMap::getGlobalElement(localIndex); }

    //! @name Xpetra specific
    //@{

    //! EpetraMap constructor to wrap a Epetra_Map object
    /*StridedEpetraMap(const Teuchos::RCP<const Epetra_BlockMap> &map) 
      : map_(map) { }*/

    //! Get the library used by this object (Epetra or Epetra?)
    UnderlyingLib lib() const { return Xpetra::UseEpetra; }
    
    /*//! Get the underlying Epetra map
    //const RCP< const Epetra_Map > & getEpetra_Map() const { return map_; }
    const Epetra_BlockMap& getEpetra_BlockMap() const { return *map_; }
    const Epetra_Map& getEpetra_Map() const { return (Epetra_Map &)*map_; } // Ugly, but the same is done in Epetra_CrsMatrix.h to get the map.*/

    //@}
   
    bool CheckConsistency() {
      if(getStridedBlockId() == -1) {
	//if(isContiguous() == false) return false;
	if(getNodeNumElements() % getFixedBlockSize() != 0) return false;
	if(getGlobalNumElements() % getFixedBlockSize() != 0) return false;
      }
      else {
	Teuchos::ArrayView< const GlobalOrdinal > dofGids = getNodeElementList();
	//std::sort(dofGids.begin(),dofGids.end());
	
	// determine nStridedOffset
	size_t nStridedOffset = 0;
	for(int j=0; j<stridedBlockId_; j++) {
	  nStridedOffset += stridingInfo_[j];
	}
	//size_t nDofsPerNode = stridingInfo_[stridedBlockId_];
	
	const GlobalOrdinal goStridedOffset = Teuchos::as<GlobalOrdinal>(nStridedOffset);
	const GlobalOrdinal goZeroOffset = (dofGids[0] - nStridedOffset) / Teuchos::as<GlobalOrdinal>(getFixedBlockSize());
	
	GlobalOrdinal cnt = 0;
	for(size_t i = 0; i<Teuchos::as<size_t>(dofGids.size())/stridingInfo_[stridedBlockId_]; i+=stridingInfo_[stridedBlockId_]) {
	  
	  for(size_t j=0; j<stridingInfo_[stridedBlockId_]; j++) {
	    const GlobalOrdinal gid = dofGids[i+j];
	    if((gid - Teuchos::as<GlobalOrdinal>(j) - goStridedOffset) / Teuchos::as<GlobalOrdinal>(getFixedBlockSize()) - goZeroOffset - cnt != 0) {
	      //std::cout << "gid: " << gid << " GID: " <<  (gid - Teuchos::as<GlobalOrdinal>(j) - goStridedOffset) / Teuchos::as<GlobalOrdinal>(getFixedBlockSize()) - goZeroOffset - cnt << std::endl;
	      return false;
	    }
	  }
	  cnt++;
	}
      }
      
      return true;
    }
   
  private:

    //RCP<const Epetra_BlockMap> map_;

  }; // StridedEpetraMap class

} // Xpetra namespace

#endif // XPETRA_STRIDEDEPETRAMAP_HPP
