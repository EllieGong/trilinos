/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef stk_mesh_base_Combo_hpp
#define stk_mesh_base_Combo_hpp

#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/Relation.hpp>
#include <stk_mesh/base/BucketConnectivity.hpp>
#include <stk_mesh/base/EntityKey.hpp>
#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/Trace.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/Part.hpp>
#include <stk_mesh/base/ConnectivityMap.hpp>

#include <stk_util/environment/ReportHandler.hpp>
#include <stk_util/environment/OutputLog.hpp>
#include <stk_util/util/PairIter.hpp>

#include <boost/static_assert.hpp>
#include <boost/range.hpp>
#include <boost/type_traits/is_pod.hpp>

#include <stk_topology/topology.hpp>

#include <utility>
#include <vector>
#include <iosfwd>
#include <string>
#include <algorithm>

#include <boost/functional/hash.hpp>

#ifdef SIERRA_MIGRATION
namespace stk {
namespace mesh {
typedef RelationVector::const_iterator   RelationIterator;
typedef boost::iterator_range<RelationIterator> RelationRange;
}
}
#endif // SIERRA_MIGRATION

#include <stk_mesh/base/Bucket.tcc> //only place where this file should be included

///////////////////////////////////////////////////////////////////////////////
// Put methods below that could not otherwise be inlined due to cyclic dependencies between
// Relation/Entity/Bucket
///////////////////////////////////////////////////////////////////////////////

namespace stk {
namespace mesh {




//
// BucketConnectivity
//

template <EntityRank TargetRank>
template <typename BulkData> // hack to get around dependency
inline
void impl::BucketConnectivity<TargetRank, FIXED_CONNECTIVITY>::end_modification(BulkData* mesh)
{
  //TODO: If bucket is blocked, no longer need to shrink to fit!

  if (m_targets.size() < m_targets.capacity()) {

    {
      EntityVector temp(m_targets.begin(), m_targets.end());
      m_targets.swap(temp);
    }

    {
      PermutationVector temp(m_permutations.begin(), m_permutations.end());
      m_permutations.swap(temp);
    }
  }

  invariant_check_helper(mesh);
}

template <EntityRank TargetRank>
template <typename BulkData>
inline
void impl::BucketConnectivity<TargetRank, DYNAMIC_CONNECTIVITY>::end_modification(BulkData* mesh)
{
  if (m_active && m_needs_shrink_to_fit) {
    resize_and_order_by_index();

    {
      UInt32Vector temp(m_indices.begin(), m_indices.end());
      m_indices.swap(temp);
    }

    {
      UInt16Vector temp(m_num_connectivities.begin(), m_num_connectivities.end());
      m_num_connectivities.swap(temp);
    }

    m_needs_shrink_to_fit = false;
  }

  invariant_check_helper(mesh);
}

template <typename BulkData>
inline
bool impl::LowerConnectivitityRankSensitiveCompare<BulkData>::operator()(Entity first_entity, ConnectivityOrdinal first_ordinal,
                                                                         Entity second_entity, ConnectivityOrdinal second_ordinal) const
{
  const EntityRank first_rank = m_mesh.entity_rank(first_entity);
  const EntityRank second_rank = m_mesh.entity_rank(second_entity);

  return (first_rank < second_rank)
         || ((first_rank == second_rank) && (first_ordinal < second_ordinal));
}

template <typename BulkData>
inline
bool impl::HigherConnectivityRankSensitiveCompare<BulkData>::operator()(Entity first_entity, ConnectivityOrdinal first_ordinal, Entity second_entity, ConnectivityOrdinal second_ordinal) const
{
  const EntityRank first_rank = m_mesh.entity_rank(first_entity);
  const EntityRank second_rank = m_mesh.entity_rank(second_entity);

  if (first_rank < second_rank) {
    return true;
  }
  if (first_rank > second_rank) {
    return false;
  }
  // Needs to match LessRelation in BulkData.hpp
  return std::make_pair(first_ordinal,  first_entity.is_local_offset_valid() ?  first_entity.local_offset()  : Entity::MaxEntity) <
         std::make_pair(second_ordinal, second_entity.is_local_offset_valid() ? second_entity.local_offset() : Entity::MaxEntity);
}




} // namespace mesh
} // namespace stk


#ifdef SIERRA_MIGRATION

namespace stk {
namespace mesh {
struct Entity;
class BulkData;
}
}

namespace sierra {
namespace Fmwk {

class MeshObjRoster;
class MeshObjSharedAttr;
class MeshBulkData;

namespace detail {
bool set_attributes( MeshBulkData& meshbulk, stk::mesh::Entity , const int , const MeshObjSharedAttr*, const int);
bool set_attributes( MeshBulkData& meshbulk, stk::mesh::Entity , const MeshObjSharedAttr*, const int);
bool update_relation( stk::mesh::Entity, const stk::mesh::RelationIterator ir, const bool back_rel_flag, MeshBulkData& bulk);
}

namespace roster_only {
void destroy_meshobj(stk::mesh::Entity, MeshBulkData& meshbulk );
}

const MeshObjSharedAttr * get_shared_attr(const stk::mesh::Entity mesh_obj, const stk::mesh::BulkData& meshbulk);
bool insert_relation( stk::mesh::Entity , const stk::mesh::RelationType,  stk::mesh::Entity , const unsigned, const unsigned, const bool, MeshBulkData &);
bool remove_relation(stk::mesh::Entity , const stk::mesh::RelationIterator, MeshBulkData &);
}
}

namespace sierra {
  namespace Fmwk {
extern const stk::mesh::RelationIterator INVALID_RELATION_ITR;
  }
}


#endif


#endif /* stk_mesh_Combo_hpp */
