// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef TPETRA_BLOCKMAP_DEF_HPP
#define TPETRA_BLOCKMAP_DEF_HPP

#include <Tpetra_ConfigDefs.hpp>
#ifndef HAVE_TPETRA_CLASSIC_VBR
#  error "It is an error to include this file if VBR (variable-block-size) sparse matrix support is disabled in Tpetra.  If you would like to enable VBR support, please reconfigure Trilinos with the CMake option Tpetra_ENABLE_CLASSIC_VBR set to ON, and rebuild Trilinos."
#else

#include "Tpetra_Distributor.hpp"
#include "Tpetra_BlockMap_decl.hpp"

namespace Tpetra {

template<class LocalOrdinal,class GlobalOrdinal,class Node>
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::BlockMap(
    global_size_t numGlobalBlocks,
    LocalOrdinal blockSize,
    GlobalOrdinal indexBase,
    const Teuchos::RCP<const Teuchos::Comm<int> > &comm,
    const Teuchos::RCP<Node> &node)
 : pointMap_(),
   globalNumBlocks_(numGlobalBlocks),
   myGlobalBlockIDs_(),
   pbuf_firstPointInBlock_(),
   view_firstPointInBlock_(),
   blockIDsAreContiguous_(true),
   constantBlockSize_(blockSize)
{
  TEUCHOS_TEST_FOR_EXCEPTION( blockSize <= 0, std::runtime_error,
       "Tpetra::BlockMap::BlockMap ERROR: blockSize must be greater than 0.");

  global_size_t numGlobalPoints = numGlobalBlocks*blockSize;

  //we compute numLocalPoints to make sure that Tpetra::Map doesn't split
  //numGlobalPoints in a way that would separate the points within a block
  //onto different mpi processors.

  size_t numLocalPoints = numGlobalBlocks/comm->getSize();
  int remainder = numGlobalBlocks%comm->getSize();
  int localProc = comm->getRank();
  if (localProc < remainder) ++numLocalPoints;
  numLocalPoints *= blockSize;

  //now create the point-map specifying both numGlobalPoints and numLocalPoints:
  pointMap_ = Teuchos::rcp(new Map<LocalOrdinal,GlobalOrdinal,Node>(numGlobalPoints, numLocalPoints, indexBase, comm, node));

  size_t numLocalBlocks = pointMap_->getNodeNumElements()/blockSize;
  size_t checkLocalBlocks = numLocalPoints/blockSize;
  //can there be an inconsistency here???
  TEUCHOS_TEST_FOR_EXCEPTION(numLocalBlocks != checkLocalBlocks, std::runtime_error,
       "Tpetra::BlockMap::BlockMap ERROR: internal failure, numLocalBlocks not consistent with point-map.");

  myGlobalBlockIDs_.resize(numLocalBlocks);
  pbuf_firstPointInBlock_ = node->template allocBuffer<LocalOrdinal>(numLocalBlocks+1);
  Teuchos::ArrayRCP<LocalOrdinal> v_firstPoints = node->template viewBufferNonConst<LocalOrdinal>(KokkosClassic::WriteOnly, numLocalBlocks+1, pbuf_firstPointInBlock_);

  LocalOrdinal firstPoint = pointMap_->getMinLocalIndex();
  GlobalOrdinal blockID = pointMap_->getMinGlobalIndex()/blockSize;
  for(size_t i=0; i<numLocalBlocks; ++i) {
    myGlobalBlockIDs_[i] = blockID++;
    v_firstPoints[i] = firstPoint;
    firstPoint += blockSize;
  }
  v_firstPoints[numLocalBlocks] = firstPoint;
  v_firstPoints = Teuchos::null;
  view_firstPointInBlock_ = node->template viewBuffer<LocalOrdinal>(numLocalBlocks+1, pbuf_firstPointInBlock_);
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::BlockMap(
    global_size_t numGlobalBlocks,
    size_t numLocalBlocks,
    LocalOrdinal blockSize,
    GlobalOrdinal indexBase,
    const Teuchos::RCP<const Teuchos::Comm<int> > &comm,
    const Teuchos::RCP<Node> &node)
 : pointMap_(),
   globalNumBlocks_(numGlobalBlocks),
   myGlobalBlockIDs_(),
   pbuf_firstPointInBlock_(),
   view_firstPointInBlock_(),
   blockIDsAreContiguous_(true),
   constantBlockSize_(blockSize)
{
  using KokkosClassic::WriteOnly;
  using Teuchos::rcp;
  using Teuchos::REDUCE_SUM;
  using Teuchos::reduceAll;
  typedef LocalOrdinal LO;
  typedef global_size_t GST;

  TEUCHOS_TEST_FOR_EXCEPTION( blockSize <= 0, std::runtime_error,
       "Tpetra::BlockMap::BlockMap ERROR: blockSize must be greater than 0.");

  const GST INVALID = Teuchos::OrdinalTraits<GST>::invalid ();
  GST numGlobalPoints = numGlobalBlocks*blockSize;
  if (numGlobalBlocks == INVALID) {
    numGlobalPoints = INVALID;
  }

  size_t numLocalPoints = numLocalBlocks*blockSize;

  //now create the point-map specifying both numGlobalPoints and numLocalPoints:
  pointMap_ = rcp (new point_map_type (numGlobalPoints, numLocalPoints,
                                       indexBase, comm, node));

  if (numGlobalBlocks == INVALID) {
    reduceAll<int, GST> (*comm, REDUCE_SUM, 1, &numLocalBlocks, &globalNumBlocks_);
  }

  myGlobalBlockIDs_.resize(numLocalBlocks);
  pbuf_firstPointInBlock_ = node->template allocBuffer<LO> (numLocalBlocks + 1);
  Teuchos::ArrayRCP<LO> v_firstPoints =
    node->template viewBufferNonConst<LO> (WriteOnly, numLocalBlocks + 1,
                                           pbuf_firstPointInBlock_);

  LO firstPoint = pointMap_->getMinLocalIndex ();
  GlobalOrdinal blockID = pointMap_->getMinGlobalIndex () / blockSize;
  for (size_t i=0; i < numLocalBlocks; ++i) {
    myGlobalBlockIDs_[i] = blockID++;
    v_firstPoints[i] = firstPoint;
    firstPoint += blockSize;
  }
  v_firstPoints[numLocalBlocks] = firstPoint;
  v_firstPoints = Teuchos::null;
  view_firstPointInBlock_ =
    node->template viewBuffer<LO> (numLocalBlocks + 1, pbuf_firstPointInBlock_);
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::
BlockMap (const global_size_t numGlobalBlocks,
          const Teuchos::ArrayView<const GlobalOrdinal>& myGlobalBlockIDs,
          const Teuchos::ArrayView<const GlobalOrdinal>& myFirstGlobalPointInBlocks,
          const Teuchos::ArrayView<const LocalOrdinal>& blockSizes,
          const GlobalOrdinal indexBase,
          const Teuchos::RCP<const Teuchos::Comm<int> >& comm,
          const Teuchos::RCP<Node>& node)
 : pointMap_ (),
   globalNumBlocks_ (numGlobalBlocks),
   myGlobalBlockIDs_ (myGlobalBlockIDs),
   pbuf_firstPointInBlock_ (),
   view_firstPointInBlock_ (),
   blockIDsAreContiguous_ (false),
   constantBlockSize_ (0)
{
  using Teuchos::outArg;
  using Teuchos::rcp;
  using Teuchos::REDUCE_SUM;
  using Teuchos::reduceAll;
  typedef LocalOrdinal LO;
  typedef global_size_t GST;

  TEUCHOS_TEST_FOR_EXCEPTION(
    myGlobalBlockIDs_.size () != blockSizes.size (), std::runtime_error,
    "Tpetra::BlockMap::BlockMap: input myGlobalBlockIDs and blockSizes arrays "
    "must have the same length.");

  size_t sum_blockSizes = 0;
  Teuchos::Array<GlobalOrdinal> myGlobalPoints;
  typename Teuchos::ArrayView<const LocalOrdinal>::const_iterator
    iter = blockSizes.begin(), iend = blockSizes.end();
  size_t i = 0;
  for (; iter!=iend; ++iter) {
    LocalOrdinal bsize = *iter;
    sum_blockSizes += bsize;
    GlobalOrdinal firstPoint = myFirstGlobalPointInBlocks[i++];
    for(LocalOrdinal j=0; j<bsize; ++j) {
      myGlobalPoints.push_back(firstPoint+j);
    }
  }

  const GST INVALID = Teuchos::OrdinalTraits<GST>::invalid ();
  pointMap_ = rcp (new point_map_type (INVALID, myGlobalPoints (),
                                       indexBase, comm, node));

  global_size_t global_sum;
  reduceAll<int, GST> (*comm, REDUCE_SUM, static_cast<GST> (myGlobalBlockIDs.size ()), outArg (global_sum));
  globalNumBlocks_ = global_sum;

  iter = blockSizes.begin();
  LO firstBlockSize = (iter == iend) ? 0 : *iter;
  LO firstPoint = pointMap_->getMinLocalIndex ();
  LO numLocalBlocks = myGlobalBlockIDs.size ();
  pbuf_firstPointInBlock_ = node->template allocBuffer<LO> (numLocalBlocks + 1);
  Teuchos::ArrayRCP<LO> v_firstPoints =
    node->template viewBufferNonConst<LO> (KokkosClassic::WriteOnly,
                                           numLocalBlocks + 1,
                                           pbuf_firstPointInBlock_);

  bool blockSizesAreConstant = true;
  i=0;
  for(; iter!=iend; ++iter, ++i) {
    v_firstPoints[i] = firstPoint;
    firstPoint += *iter;
    if (*iter != firstBlockSize) {
      blockSizesAreConstant = false;
    }
  }
  v_firstPoints[i] = firstPoint;
  v_firstPoints = Teuchos::null;
  if (blockSizesAreConstant) {
    constantBlockSize_ = firstBlockSize;
  }

  size_t num_points = pointMap_->getNodeNumElements ();
  TEUCHOS_TEST_FOR_EXCEPTION(
    sum_blockSizes != num_points, std::runtime_error,
    "Tpetra::BlockMap::BlockMap: internal failure, sum of block sizes must "
    "equal pointMap->getNodeNumElements().");

  typename Teuchos::Array<GlobalOrdinal>::const_iterator
    b_iter = myGlobalBlockIDs_.begin(), b_end = myGlobalBlockIDs_.end();
  GlobalOrdinal id = b_iter==b_end ? 0 : *b_iter;
  if (b_iter != b_end) ++b_iter;
  blockIDsAreContiguous_ = true;
  for(; b_iter != b_end; ++b_iter) {
    if (*b_iter != id+1) {
      blockIDsAreContiguous_ = false;
      break;
    }
    ++id;
  }
  if (blockIDsAreContiguous_ == false) {
    setup_noncontig_mapping();
  }

  view_firstPointInBlock_ =
    node->template viewBuffer<LO> (numLocalBlocks+1, pbuf_firstPointInBlock_);
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::
BlockMap (const Teuchos::RCP<const point_map_type>& pointMap,
          const Teuchos::ArrayView<const GlobalOrdinal>& myGlobalBlockIDs,
          const Teuchos::ArrayView<const LocalOrdinal>& blockSizes) :
  pointMap_ (pointMap),
  globalNumBlocks_ (Teuchos::OrdinalTraits<global_size_t>::invalid ()),
  myGlobalBlockIDs_(myGlobalBlockIDs),
  pbuf_firstPointInBlock_ (),
  view_firstPointInBlock_ (),
  blockIDsAreContiguous_ (false),
  constantBlockSize_ (0)
{
  Teuchos::RCP<Node> node =
    pointMap.is_null () ? defaultArgNode<Node> () : pointMap->getNode ();
  initWithPointMap (pointMap, myGlobalBlockIDs, blockSizes, node);
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
TPETRA_DEPRECATED
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::
BlockMap (const Teuchos::RCP<const point_map_type>& pointMap,
          const Teuchos::ArrayView<const GlobalOrdinal>& myGlobalBlockIDs,
          const Teuchos::ArrayView<const LocalOrdinal>& blockSizes,
          const Teuchos::RCP<Node>& node) :
  pointMap_ (pointMap),
  globalNumBlocks_ (Teuchos::OrdinalTraits<global_size_t>::invalid ()),
  myGlobalBlockIDs_(myGlobalBlockIDs),
  pbuf_firstPointInBlock_ (),
  view_firstPointInBlock_ (),
  blockIDsAreContiguous_ (false),
  constantBlockSize_ (0)
{
  initWithPointMap (pointMap, myGlobalBlockIDs, blockSizes, node);
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
void
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::
initWithPointMap (const Teuchos::RCP<const point_map_type>& pointMap,
                  const Teuchos::ArrayView<const GlobalOrdinal>& myGlobalBlockIDs,
                  const Teuchos::ArrayView<const LocalOrdinal>& blockSizes,
                  const Teuchos::RCP<Node>& node)
{
  using KokkosClassic::WriteOnly;
  using Teuchos::ArrayRCP;
  using Teuchos::REDUCE_SUM;
  using Teuchos::reduceAll;
  typedef GlobalOrdinal GO;
  typedef LocalOrdinal LO;
  typedef global_size_t GST;

  TEUCHOS_TEST_FOR_EXCEPTION(
    myGlobalBlockIDs_.size () != blockSizes.size (), std::runtime_error,
    "Tpetra::BlockMap::BlockMap: input myGlobalBlockIDs and blockSizes arrays "
    "must have the same length.");

  const global_size_t numLocalBlocks = myGlobalBlockIDs.size ();
  reduceAll<int, GST> (* (pointMap->getComm ()), REDUCE_SUM, 1,
                       &numLocalBlocks, &globalNumBlocks_);

  pbuf_firstPointInBlock_ =
    node->template allocBuffer<LO> (myGlobalBlockIDs.size () + 1);
  ArrayRCP<LO> v_firstPoints =
    node->template viewBufferNonConst<LO> (WriteOnly, numLocalBlocks + 1,
                                           pbuf_firstPointInBlock_);

  LO firstPoint = pointMap->getMinLocalIndex ();
  size_t sum_blockSizes = 0;
  typename Teuchos::ArrayView<const LO>::const_iterator
    iter = blockSizes.begin(), iend = blockSizes.end();
  LocalOrdinal firstBlockSize = *iter;
  bool blockSizesAreConstant = true;
  size_t i = 0;
  for (; iter!=iend; ++iter, ++i) {
    sum_blockSizes += *iter;
    v_firstPoints[i] = firstPoint;
    firstPoint += *iter;
    if (*iter != firstBlockSize) {
      blockSizesAreConstant = false;
    }
  }
  v_firstPoints[i] = firstPoint;
  v_firstPoints = Teuchos::null;
  if (blockSizesAreConstant) {
    constantBlockSize_ = firstBlockSize;
  }

  const size_t num_points = pointMap->getNodeNumElements ();
  TEUCHOS_TEST_FOR_EXCEPTION(
    sum_blockSizes != num_points, std::runtime_error,
    "Tpetra::BlockMap::BlockMap: Sum of block sizes must equal "
    "pointMap->getNodeNumElements().");

  typename Teuchos::Array<GO>::const_iterator
    b_iter = myGlobalBlockIDs_.begin(), b_end = myGlobalBlockIDs_.end();
  GlobalOrdinal id = *b_iter;
  ++b_iter;
  blockIDsAreContiguous_ = true;
  for (; b_iter != b_end; ++b_iter) {
    if (*b_iter != id+1) {
      blockIDsAreContiguous_ = false;
      break;
    }
    ++id;
  }
  if (! blockIDsAreContiguous_) {
    setup_noncontig_mapping ();
  }

  view_firstPointInBlock_ =
    node->template viewBuffer<LO> (numLocalBlocks+1, pbuf_firstPointInBlock_);
}


template<class LocalOrdinal,class GlobalOrdinal,class Node>
void
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getRemoteBlockInfo(
    const Teuchos::ArrayView<const GlobalOrdinal>& GBIDs,
    const Teuchos::ArrayView<GlobalOrdinal>& firstGlobalPointInBlocks,
    const Teuchos::ArrayView<LocalOrdinal>& blockSizes) const
{
  Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > pbmap =
      convertBlockMapToPointMap(*this);

  Teuchos::Array<int> remoteProcs(GBIDs.size());
  pbmap->getRemoteIndexList(GBIDs(), remoteProcs());

  Tpetra::Distributor distor(pbmap->getComm());

  Tpetra::Array<GlobalOrdinal> exportGBIDs;
  Tpetra::Array<int> exportProcs;

  distor.createFromRecvs(GBIDs(), remoteProcs(), exportGBIDs, exportProcs);

  Tpetra::Array<GlobalOrdinal> exportFirstPointInBlocks(exportGBIDs.size());
  Tpetra::Array<LocalOrdinal> exportSizes(exportGBIDs.size());

  typename Teuchos::Array<GlobalOrdinal>::const_iterator
    iter = exportGBIDs.begin(),
    iter_end = exportGBIDs.end();
  for(int i=0; iter!=iter_end; ++iter, ++i) {
    GlobalOrdinal gbid = *iter;
    LocalOrdinal lbid = getLocalBlockID(gbid);
    exportFirstPointInBlocks[i] = getFirstGlobalPointInLocalBlock(lbid);
    exportSizes[i] = getLocalBlockSize(lbid);
  }

  size_t numPackets = 1;
  distor.doPostsAndWaits(exportFirstPointInBlocks().getConst(),
                         numPackets, firstGlobalPointInBlocks);

  distor.doPostsAndWaits(exportSizes().getConst(), numPackets, blockSizes);
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
void
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::setup_noncontig_mapping()
{
  typedef typename Teuchos::Array<GlobalOrdinal>::size_type Tsize_t;
  for(Tsize_t i=0; i<myGlobalBlockIDs_.size(); ++i) {
    LocalOrdinal li = i;
    // TODO: Use Tpetra::Details::HashTable here instead of std::map.
    map_global_to_local_.insert(std::make_pair(myGlobalBlockIDs_[i],li));
  }
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
global_size_t
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getGlobalNumBlocks() const
{
  return globalNumBlocks_;
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
size_t
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getNodeNumBlocks() const
{
  return myGlobalBlockIDs_.size();
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
Teuchos::ArrayView<const GlobalOrdinal>
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getNodeBlockIDs() const
{
  return myGlobalBlockIDs_();
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
bool
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::isBlockSizeConstant() const
{ return constantBlockSize_ != 0; }

template<class LocalOrdinal,class GlobalOrdinal,class Node>
Teuchos::ArrayRCP<const LocalOrdinal>
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getNodeFirstPointInBlocks() const
{
  return view_firstPointInBlock_;
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
Teuchos::ArrayRCP<const LocalOrdinal>
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getNodeFirstPointInBlocks_Device() const
{
  return pbuf_firstPointInBlock_;
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
GlobalOrdinal
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getGlobalBlockID(LocalOrdinal localBlockID) const
{
  LocalOrdinal invalid = Teuchos::OrdinalTraits<LocalOrdinal>::invalid();
  if (localBlockID < 0 || localBlockID >= myGlobalBlockIDs_.size()) {
    return invalid;
  }

  return myGlobalBlockIDs_[localBlockID];
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
LocalOrdinal
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getLocalBlockID(GlobalOrdinal globalBlockID) const
{
  LocalOrdinal invalid = Teuchos::OrdinalTraits<LocalOrdinal>::invalid();
  if (myGlobalBlockIDs_.size() == 0) {
    return invalid;
  }

  if (blockIDsAreContiguous_ == false) {
    // TODO: Use Tpetra::Details::HashTable instead of std::map.
    typename std::map<GlobalOrdinal,LocalOrdinal>::const_iterator iter =
      map_global_to_local_.find(globalBlockID);
    if (iter == map_global_to_local_.end()) return invalid;
    return iter->second;
  }

  LocalOrdinal localBlockID = globalBlockID - myGlobalBlockIDs_[0];
  LocalOrdinal numLocalBlocks = myGlobalBlockIDs_.size();

  if (localBlockID < 0 || localBlockID >= numLocalBlocks) {
    return invalid;
  }

  return localBlockID;
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
LocalOrdinal
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getLocalBlockSize(LocalOrdinal localBlockID) const
{
  if (constantBlockSize_ != 0) {
    return constantBlockSize_;
  }

  //should this be a debug-mode-only range check?
  if (localBlockID < 0 || localBlockID >= view_firstPointInBlock_.size()) {
    throw std::runtime_error("Tpetra::BlockMap::getLocalBlockSize ERROR: localBlockID out of range.");
  }

  return view_firstPointInBlock_[localBlockID+1]-view_firstPointInBlock_[localBlockID];
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
LocalOrdinal
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getFirstLocalPointInLocalBlock(LocalOrdinal localBlockID) const
{
  //should this be a debug-mode-only range check?
  if (localBlockID < 0 || localBlockID >= view_firstPointInBlock_.size()) {
    throw std::runtime_error("Tpetra::BlockMap::getFirstLocalPointInLocalBlock ERROR: localBlockID out of range.");
  }

  return view_firstPointInBlock_[localBlockID];
}

template<class LocalOrdinal,class GlobalOrdinal,class Node>
GlobalOrdinal
BlockMap<LocalOrdinal,GlobalOrdinal,Node>::getFirstGlobalPointInLocalBlock(LocalOrdinal localBlockID) const
{
  //should this be a debug-mode-only range check?
  if (localBlockID < 0 || localBlockID >= view_firstPointInBlock_.size()) {
    throw std::runtime_error("Tpetra::BlockMap::getFirstGlobalPointInLocalBlock ERROR: localBlockID out of range.");
  }

  return pointMap_->getGlobalElement(view_firstPointInBlock_[localBlockID]);
}

} // namespace Tpetra

//
// Explicit instantiation macro
//
// Must be expanded from within the Tpetra namespace!
//

#define TPETRA_BLOCKMAP_INSTANT(LO,GO,NODE) \
  \
  template class BlockMap< LO , GO , NODE >;

#endif // ! HAVE_TPETRA_CLASSIC_VBR
#endif // ! TPETRA_BLOCKMAP_DEF_HPP
