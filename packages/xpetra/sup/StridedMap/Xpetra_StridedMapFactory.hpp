#ifndef XPETRA_STRIDEDMAPFACTORY_HPP
#define XPETRA_STRIDEDMAPFACTORY_HPP

#include "Xpetra_ConfigDefs.hpp"

#include "Xpetra_StridedMap.hpp"

#ifdef HAVE_XPETRA_TPETRA
#include "Xpetra_StridedTpetraMap.hpp"
#endif
#ifdef HAVE_XPETRA_EPETRA
#include "Xpetra_StridedEpetraMap.hpp"
#endif

#include "Xpetra_Exceptions.hpp"

// This factory creates Xpetra::Map. User have to specify the exact class of object that he want to create (ie: a Xpetra::TpetraMap or a Xpetra::EpetraMap).

namespace Xpetra {

  template <class LocalOrdinal, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType>
  class StridedMapFactory {
    
  private:
    //! Private constructor. This is a static class. 
    StridedMapFactory() {}
    
  public:
    
    //! Map constructor with Xpetra-defined contiguous uniform distribution.
    static Teuchos::RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, global_size_t numGlobalElements, GlobalOrdinal indexBase, std::vector<size_t>& stridingInfo, const Teuchos::RCP<const Teuchos::Comm<int> > &comm, LocalOrdinal stridedBlockId=-1, GlobalOrdinal offset = 0, LocalGlobal lg=Xpetra::GloballyDistributed, const Teuchos::RCP<Node> &node = Kokkos::DefaultNode::getDefaultNode()) {

#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra)
        return Teuchos::rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (numGlobalElements, indexBase, stridingInfo, comm, stridedBlockId, offset, lg, node) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(lib);
      XPETRA_FACTORY_END;
    }

    //! Map constructor with a user-defined contiguous distribution.
    static Teuchos::RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, global_size_t numGlobalElements, size_t numLocalElements, GlobalOrdinal indexBase, std::vector<size_t>& stridingInfo, const Teuchos::RCP<const Teuchos::Comm<int> > &comm, LocalOrdinal stridedBlockId=-1, GlobalOrdinal offset = 0, const Teuchos::RCP<Node> &node = Kokkos::DefaultNode::getDefaultNode()) {

#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra)       
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (numGlobalElements, numLocalElements, indexBase, stridingInfo, comm, stridedBlockId, offset, node) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(lib);
      XPETRA_FACTORY_END;
    }

    static RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> >& map, std::vector<size_t>& stridingInfo, LocalOrdinal stridedBlockId=-1, GlobalOrdinal offset = 0) {

#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra) {
        Teuchos::RCP<const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> > tmap = Teuchos::rcp_dynamic_cast<const TpetraMap<LocalOrdinal, GlobalOrdinal, Node> >(map);
        TEUCHOS_TEST_FOR_EXCEPTION(tmap == Teuchos::null, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: bad cast. map is not a TpetraMap.");
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (tmap->getTpetra_Map(), stridingInfo, stridedBlockId, offset) );
      }
#endif
      XPETRA_FACTORY_ERROR_IF_EPETRA(lib);
      XPETRA_FACTORY_END;
    }

    // special constructor for generating a given subblock of a strided map
    static RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(const RCP<const StridedMap<LocalOrdinal, GlobalOrdinal, Node> >& map, LocalOrdinal stridedBlockId) {
      TEUCHOS_TEST_FOR_EXCEPTION(stridedBlockId < 0, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: constructor expects stridedBlockId > -1.");
#ifdef HAVE_XPETRA_TPETRA
      if (map->lib() == UseTpetra) {
        TEUCHOS_TEST_FOR_EXCEPTION(map->getStridedBlockId() != -1, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: constructor expects a full map (stridedBlockId == -1).");
        std::vector<size_t> stridingInfo = map->getStridingData();

        /////////////////////////////////////////////
        Teuchos::ArrayView< const GlobalOrdinal > dofGids = map->getNodeElementList();
        //std::sort(dofGids.begin(),dofGids.end()); // TODO: do i need this?

        // determine nStridedOffset
        size_t nStridedOffset = 0;
        for(int j=0; j<map->getStridedBlockId(); j++) {
          nStridedOffset += stridingInfo[j];
        }

        size_t numMyBlockDofs = stridingInfo[stridedBlockId] / map->getFixedBlockSize() * map->getNodeNumElements();
        std::vector<GlobalOrdinal> subBlockDofGids(numMyBlockDofs);

        // TODO fill vector with dofs
        typename Teuchos::ArrayView< const GlobalOrdinal >::iterator it;
        for(it = dofGids.begin(); it!=dofGids.end(); ++it) {
          if(map->GID2StridingBlockId( *it ) == stridedBlockId) {
            subBlockDofGids.push_back( *it );
          }
        }

        const Teuchos::ArrayView<const LocalOrdinal> subBlockDofGids_view(&subBlockDofGids[0],subBlockDofGids.size());

        // call constructor for TpetraMap
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (subBlockDofGids.size(), subBlockDofGids_view, map->getIndexBase(), stridingInfo, map->getComm(), stridedBlockId, map->getNode()) );
        ////////////////////////////////////////

      }
#endif
      XPETRA_FACTORY_ERROR_IF_EPETRA(map->lib());
      XPETRA_FACTORY_END;
    }

#if 0  // TODO
    //! Map constructor with user-defined non-contiguous (arbitrary) distribution.
    static Teuchos::RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, global_size_t numGlobalElements, const Teuchos::ArrayView<const GlobalOrdinal> &elementList, GlobalOrdinal indexBase, const Teuchos::RCP<const Teuchos::Comm<int> > &comm, const Teuchos::RCP<Node> &node = Kokkos::DefaultNode::getDefaultNode()) {

#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra) 
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (numGlobalElements, elementList, indexBase, comm, node) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(lib);
      XPETRA_FACTORY_END;
    }
#endif


  };

  template <>
  class StridedMapFactory<int, int> {

    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;
    
  private:
    //! Private constructor. This is a static class. 
    StridedMapFactory() {}
    
  public:
    
    static RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, global_size_t numGlobalElements, int indexBase, std::vector<size_t>& stridingInfo, const Teuchos::RCP<const Teuchos::Comm<int> > &comm, LocalOrdinal stridedBlockId=-1, GlobalOrdinal offset = 0, LocalGlobal lg=GloballyDistributed, const Teuchos::RCP<Kokkos::DefaultNode::DefaultNodeType> &node = Kokkos::DefaultNode::getDefaultNode()) {

#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra)
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (numGlobalElements, indexBase, stridingInfo, comm, stridedBlockId, offset, lg, node) );
#endif

#ifdef HAVE_XPETRA_EPETRA
      if (lib == UseEpetra)
        return rcp( new StridedEpetraMap(numGlobalElements, indexBase, stridingInfo, comm, stridedBlockId, offset, lg, node) );
#endif

      XPETRA_FACTORY_END;
    }

    static RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, global_size_t numGlobalElements, size_t numLocalElements, int indexBase, std::vector<size_t>& stridingInfo, const Teuchos::RCP<const Teuchos::Comm<int> > &comm, LocalOrdinal stridedBlockId=-1, GlobalOrdinal offset = 0, const Teuchos::RCP<Kokkos::DefaultNode::DefaultNodeType> &node = Kokkos::DefaultNode::getDefaultNode()) {

#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra)       
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (numGlobalElements, numLocalElements, indexBase, stridingInfo, comm, stridedBlockId, offset, node) );
#endif

#ifdef HAVE_XPETRA_EPETRA
      if (lib == UseEpetra)
        return rcp( new StridedEpetraMap(numGlobalElements, numLocalElements, indexBase, stridingInfo, comm, stridedBlockId, offset, node) );
#endif

      XPETRA_FACTORY_END;
    }

    static RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> >& map, std::vector<size_t>& stridingInfo, LocalOrdinal stridedBlockId=-1, GlobalOrdinal offset = 0) {

#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra) {
	Teuchos::RCP<const TpetraMap<LocalOrdinal,GlobalOrdinal,Node> > tmap = Teuchos::rcp_dynamic_cast<const TpetraMap<LocalOrdinal, GlobalOrdinal, Node> >(map);
	TEUCHOS_TEST_FOR_EXCEPTION(tmap == Teuchos::null, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: bad cast. map is not a TpetraMap.");
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (tmap->getTpetra_Map(), stridingInfo, stridedBlockId, offset) );
      }
#endif

#ifdef HAVE_XPETRA_EPETRA
      if (lib == UseEpetra) {
	Teuchos::RCP<const EpetraMap> emap = Teuchos::rcp_dynamic_cast<const EpetraMap>(map);
	TEUCHOS_TEST_FOR_EXCEPTION(emap == Teuchos::null, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: bad cast. map is not a EpetraMap.");	
        return rcp( new StridedEpetraMap(Teuchos::rcpFromRef(emap->getEpetra_Map()), stridingInfo,stridedBlockId, offset) ); // ugly: EpetraMap and TpetraMap differ here
      }
#endif

      XPETRA_FACTORY_END;
    }

    // special constructor for generating a given subblock of a strided map
    static RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(const RCP<const StridedMap<LocalOrdinal, GlobalOrdinal, Node> >& map, LocalOrdinal stridedBlockId) {
      TEUCHOS_TEST_FOR_EXCEPTION(stridedBlockId < 0, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: constructor expects stridedBlockId > -1.");
      typedef Xpetra::StridedMap<LocalOrdinal,GlobalOrdinal,Node> StridedMapClass;
      typename Teuchos::ArrayView< const GlobalOrdinal >::iterator it;
#ifdef HAVE_XPETRA_TPETRA
      if (map->lib() == UseTpetra) {
        TEUCHOS_TEST_FOR_EXCEPTION(map->getStridedBlockId() != -1, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: constructor expects a full map (stridedBlockId == -1).");
        std::vector<size_t> stridingInfo = map->getStridingData();

        /////////////////////////////////////////////
        Teuchos::ArrayView< const GlobalOrdinal > dofGids = map->getNodeElementList();
        //std::sort(dofGids.begin(),dofGids.end()); // TODO: do i need this?

        // determine nStridedOffset
        size_t nStridedOffset = 0;
        for(int j=0; j<map->getStridedBlockId(); j++) {
          nStridedOffset += stridingInfo[j];
        }

        size_t numMyBlockDofs = stridingInfo[stridedBlockId] / map->getFixedBlockSize() * map->getNodeNumElements();
        std::vector<GlobalOrdinal> subBlockDofGids(numMyBlockDofs);

        // TODO fill vector with dofs

        for(it = dofGids.begin(); it!=dofGids.end(); ++it) {
          if(map->GID2StridingBlockId( *it ) == Teuchos::as<size_t>(stridedBlockId)) {
            subBlockDofGids.push_back( *it );
          }
        }

        const Teuchos::ArrayView<const LocalOrdinal> subBlockDofGids_view(&subBlockDofGids[0],subBlockDofGids.size());

        // call constructor for TpetraMap
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (/*subBlockDofGids.size()*/Teuchos::OrdinalTraits<global_size_t>::invalid(), subBlockDofGids_view, map->getIndexBase(), stridingInfo, map->getComm(), stridedBlockId, map->getNode()) );
        ////////////////////////////////////////

      }
#endif
#ifdef HAVE_XPETRA_EPETRA
      if (map->lib() == UseEpetra) {
        TEUCHOS_TEST_FOR_EXCEPTION(map->getStridedBlockId() != -1, Exceptions::RuntimeError,"Xpetra::StridedMapFactory::Build: constructor expects a full map (stridedBlockId == -1).");
        std::vector<size_t> stridingInfo = map->getStridingData();

        /////////////////////////////////////////////
        Teuchos::ArrayView< const GlobalOrdinal > dofGids = map->getNodeElementList();
        //std::sort(dofGids.begin(),dofGids.end()); // TODO: do i need this?

        // determine nStridedOffset
        size_t nStridedOffset = 0;
        for(int j=0; j<map->getStridedBlockId(); j++) {
          nStridedOffset += stridingInfo[j];
        }

        size_t numMyBlockDofs = stridingInfo[stridedBlockId] / map->getFixedBlockSize() * map->getNodeNumElements();
        std::vector<GlobalOrdinal> subBlockDofGids(numMyBlockDofs);

        // TODO fill vector with dofs
        //Teuchos::ArrayView< const GlobalOrdinal >::iterator it;
        for(it = dofGids.begin(); it!=dofGids.end(); ++it) {
          if(map->GID2StridingBlockId( *it ) == Teuchos::as<size_t>(stridedBlockId)) {
            subBlockDofGids.push_back( *it );
          }
        }

        const Teuchos::ArrayView<const LocalOrdinal> subBlockDofGids_view(&subBlockDofGids[0],subBlockDofGids.size());

        // call constructor for TpetraMap
        return rcp( new StridedEpetraMap(Teuchos::OrdinalTraits<global_size_t>::invalid(), subBlockDofGids_view, map->getIndexBase(), stridingInfo, map->getComm(), stridedBlockId, map->getNode()) );
        ////////////////////////////////////////

      }
#endif
      XPETRA_FACTORY_END;
    }

#if 0 // TODO
    static RCP<StridedMap<LocalOrdinal,GlobalOrdinal, Node> > Build(UnderlyingLib lib, global_size_t numGlobalElements, const Teuchos::ArrayView<const int> &elementList, int indexBase, const Teuchos::RCP<const Teuchos::Comm<int> > &comm, const Teuchos::RCP<Kokkos::DefaultNode::DefaultNodeType> &node = Kokkos::DefaultNode::getDefaultNode()) {
#ifdef HAVE_XPETRA_TPETRA
      if (lib == UseTpetra) 
        return rcp( new StridedTpetraMap<LocalOrdinal,GlobalOrdinal, Node> (numGlobalElements, elementList, indexBase, comm, node) );
#endif

#ifdef HAVE_XPETRA_EPETRA
      if (lib == UseEpetra)
        return rcp( new StridedEpetraMap(numGlobalElements, elementList, indexBase, comm, node) );
#endif
      XPETRA_FACTORY_END;
    }
#endif

 
  };

}

#define XPETRA_STRIDEDMAPFACTORY_SHORT
#endif
//TODO: removed unused methods
