
#include <stk_mesh/base/FieldRestriction.hpp>
#include <sstream>                      // for operator<<, etc
#include "stk_mesh/base/Selector.hpp"   // for Selector, operator<<
#include "stk_mesh/base/Types.hpp"      // for FieldArrayRank

namespace stk {
namespace mesh {

void FieldRestriction::print(
  std::ostream & os,
  const Selector & selector,
  FieldArrayRank field_rank
  ) const
{
  os << "FieldRestriction[ selector: \"" << selector << "\", dimension: " << m_dimension << ", scalars per entity: " << m_num_scalars_per_entity << " ]" ;
}

std::string print_restriction(
  const FieldRestriction & restr,
  const Selector& selector,
  FieldArrayRank field_rank
                              )
{
  std::ostringstream oss;
  restr.print(oss, selector, field_rank);
  return oss.str();
}

} // namespace mesh
} // namespace stk
