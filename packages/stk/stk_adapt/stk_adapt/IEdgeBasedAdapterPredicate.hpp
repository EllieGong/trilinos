#ifndef stk_adapt_IEdgeBasedAdapterPredicate_hpp
#define stk_adapt_IEdgeBasedAdapterPredicate_hpp

#include <stk_adapt/Refiner.hpp>
#include <stk_percept/PerceptMesh.hpp>

#include <functional>

namespace stk {
  namespace adapt {

    /** Signatures for predicate objects that can be used to select entities (elements, edges, faces,...) for
     *   refinement or unrefinement.  The class must supply an operator() that takes an entity and decides
     *   on whether it should be refined, or unrefined, or ignored.
     *
     * The Selector pattern as shown below is useful for selecting only entities that belong to particular
     *   mesh Parts, for example, or any other definition of Selector.
     *
     * We follow the unary_function pattern to enable the structs to be used in STL algorithms that know about
     *   unary_functions.
     *
     * The following are a couple of examples of what refine and unrefine predicates might look like.
     *
     * Following these examples we show the prototype for the operations that are performed on these predicates.
     */

    // Example 
    struct IEdgeBasedAdapterPredicate {
      PerceptMesh& m_eMesh;
      stk::mesh::Selector * m_selector;
      stk::mesh::FieldBase *m_field;
      double m_tolerance;
    protected:
      IEdgeBasedAdapterPredicate(PerceptMesh& eMesh, stk::mesh::Selector * selector = 0, stk::mesh::FieldBase *field = 0, double tolerance=0.0) :
        m_eMesh(eMesh), m_selector(selector), m_field(field), m_tolerance(tolerance) {}

      /// for a Refine predicate, Return true for refine, false for ignore
      /// for an Unrefine predicate, return true for unrefine, false for ignore
      bool operator()(const stk::mesh::Entity element, unsigned which_edge, stk::mesh::Entity node0, stk::mesh::Entity node1,
                      double *coord0, double *coord1, std::vector<int>* existing_edge_marks);

    };

  }
}
#endif
