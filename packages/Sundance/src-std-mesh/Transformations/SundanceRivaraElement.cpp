#include "SundanceRivaraElement.hpp"
#include "SundanceRivaraEdge.hpp"
#include "SundanceRivaraNode.hpp"
#include "SundanceRivaraMesh.hpp"
#include "SundanceOut.hpp"



using namespace SundanceStdMesh::Rivara;
using namespace Teuchos;
using std::endl;


Element::Element(RivaraMesh* mesh,
                 const RefCountPtr<Node>& a,
                 const RefCountPtr<Node>& b,
                 const RefCountPtr<Node>& c,
  int ownerProc,
  int label)
  : label_(label),
    nodes_(tuple(a,b,c)),
    edges_(3),
    faces_(0),
    edgeSigns_(3),
		ownerProc_(ownerProc)
{
  for (int i=0; i<3; i++)
    {
      edges_[i] = mesh->tryEdge(nodes_[i], nodes_[(i+1)%3], edgeSigns_[i]);
      edges_[i]->addConnectingElement(this);
      nodes_[i]->addConnectingElement(this);

      nodes_[i]->addConnectingEdge(edges_[i].get());
      nodes_[(i+1) % 3]->addConnectingEdge(edges_[i].get());
    }
}

Element::Element(RivaraMesh* mesh,
                 const RefCountPtr<Node>& a,
                 const RefCountPtr<Node>& b,
                 const RefCountPtr<Node>& c,
                 const RefCountPtr<Node>& d,
  int ownerProc,
  int label)
  : label_(label),
    nodes_(tuple(a,b,c,d)),
    edges_(6),
    faces_(4),
    edgeSigns_(6),
		ownerProc_(ownerProc)
{

  for (int i=0; i<4; i++)
    {
      nodes_[i]->addConnectingElement(this);
    }

  int k=0;
  for (int i=0; i<4; i++)
    {
      for (int j=0; j<i; j++)
        {
          edges_[k] = mesh->tryEdge(nodes_[i], nodes_[j], edgeSigns_[k]);
          edges_[k]->addConnectingElement(this);
          nodes_[i]->addConnectingEdge(edges_[k].get());          
          nodes_[j]->addConnectingEdge(edges_[k].get());          
          k++;
        }
    }

  faces_[0] = mesh->tryFace(a,b,d);
  faces_[1] = mesh->tryFace(b,c,d);
  faces_[2] = mesh->tryFace(a,d,c);
  faces_[3] = mesh->tryFace(c,b,a);
}

int Element::longestEdgeIndex() const 
{
  int e = 0;
  double L = -1.0;
  for (int i=0; i<edges_.length(); i++)
    {
      if (edges_[i]->length() > L) {e = i; L = edges_[i]->length();}
    }
  return e;
}

bool Element::hasHangingNode() const 
{
  bool hasHangingNode = false;
  for (int i=0; i<edges_.length(); i++)
    {
      if (edges_[i]->hasChildren()) hasHangingNode = true;
    }
  return hasHangingNode;
}

void Element::refine(RivaraMesh* mesh, double maxArea)
{
  /* if have children, refine the children */
  if (hasChildren())
    {
      dynamic_cast<Element*>(left())->refine(mesh, maxArea);
      dynamic_cast<Element*>(right())->refine(mesh, maxArea);
      return;
    }


  /* We will not need to refine if there is no hanging node and if
   * the area is already small enough */
  if (!hasHangingNode() && volume() < maxArea)
    {
      return;
    }
  if (!hasHangingNode() && maxArea < 0.0)
    {
      return;
    }

  /* --- If we're to this point, we need to refine --- */

  
  /* find the longest edge */
  int e = longestEdgeIndex();

	Element* sub1;
	Element* sub2;
  
	if (nodes_.length()==3)
		{
			/* classify the nodes relative to the longest edge:
			 * Nodes a and b are on the longest edge.
			 * Node c is opposite the longest edge.
			 */
			RefCountPtr<Node> a, b, c;
			if (edgeSigns_[e] > 0)
				{
					a = edges_[e]->node(0);
					b = edges_[e]->node(1);
				}
			else
				{
					b = edges_[e]->node(0);
					a = edges_[e]->node(1);
				}
			/* find the node that's not on the longest edge */
			for (int i=0; i<nodes_.length(); i++)
				{
					if (nodes_[i].get() == a.get() || nodes_[i].get() == b.get()) continue;
					c = nodes_[i];
					break;
				}

			/* bisect the edge, creating a new node at the midpoint and two child
			 * edges */
			RefCountPtr<Node> mid = edges_[e]->bisect(mesh);

			/* create the triangles defined by bisecting the longest edge */
			sub1 = new Element(mesh, a, mid, c, ownerProc_, label_);
			sub2 = new Element(mesh, c, mid, b, ownerProc_, label_);
		}
	else
		{
			/* classify the nodes relative to the longest edge:
			 * Nodes a and b are on the longest edge.
			 * Nodes c and d are on the edge that does not intersect the 
			 * longest edge.
			 */
			RefCountPtr<Node> a, b, c, d;
			if (edgeSigns_[e] > 0)
				{
					a = edges_[e]->node(0);
					b = edges_[e]->node(1);
				}
			else
				{
					b = edges_[e]->node(0);
					a = edges_[e]->node(1);
				}
			/* find the edge whose nodes are not on the longest edge */
			for (int i=0; i<edges_.length(); i++)
				{
					const RefCountPtr<Node>& node1 = edges_[i]->node(0);
					const RefCountPtr<Node>& node2 = edges_[i]->node(1);
					if (node1.get()==a.get() || node1.get()==b.get()
							|| node2.get()==a.get() || node2.get()==b.get())
						{
							continue;
						}
					if (edgeSigns_[i] > 0)
						{
							c = edges_[i]->node(0);
							d = edges_[i]->node(1);
						}
					else
						{
							d = edges_[i]->node(0);
							c = edges_[i]->node(1);
						}
					break;
				}

			/* bisect the edge, creating a new node at the midpoint and two child
			 * edges */
			RefCountPtr<Node> mid = edges_[e]->bisect(mesh);

			/* create the tets defined by bisecting the longest edge */
			sub1 = new Element(mesh, a, mid, c, d, ownerProc_, label_);
			sub2 = new Element(mesh, b, c, mid, d, ownerProc_, label_);


      /* if I have a label, then label the new faces */
      
      if (label_ != -1)
      {
        RefCountPtr<Face> abc = mesh->getFace(a,b,c);
        RefCountPtr<Face> abd = mesh->getFace(a,b,d);
        RefCountPtr<Face> acd = mesh->getFace(a,c,d);
        RefCountPtr<Face> bcd = mesh->getFace(b,c,d);
        
        RefCountPtr<Face> amd = mesh->getFace(a,mid,d);
        RefCountPtr<Face> amc = mesh->getFace(a,mid,c);
        RefCountPtr<Face> bmc = mesh->getFace(b,mid,c);
        RefCountPtr<Face> bmd = mesh->getFace(b,mid,d);
        
        amd->setLabel(abd->label());
        amc->setLabel(abc->label());
        
        bmd->setLabel(abd->label());
        bmc->setLabel(abc->label());
      }
      
		}

  sub1->setParent(this);
  sub2->setParent(this);

  setChildren(sub1, sub2);

  /* if the new node is hanging on the edge's other cofacets, mark the
   * other cofacets for refinement */
  Array<Element*> others;
  edges_[e]->getUnrefinedCofacets(others);
  for (int i=0; i<others.length(); i++)
    {
      Element* other = others[i];
      if (other==0) continue;
      mesh->refinementSet().push(other);
      mesh->refinementAreas().push(-1.0);
    }

  if (sub1->hasHangingNode() || maxArea > 0.0) 
    sub1->refine(mesh, maxArea);
  if (sub2->hasHangingNode() || maxArea > 0.0) 
    sub2->refine(mesh, maxArea);
}

namespace SundanceStdMesh
{
inline Point cross3(const Point& a, const Point& b)
{
  return Point(a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2],
    a[0]*b[1]-a[1]*b[0]);
}

inline double cross2(const Point& a, const Point& b)
{
  return a[0]*b[1] - a[1]*b[0];
}
}

double Element::volume() const 
{
	if (nodes_.length()==3)
		{
      return 0.5*::fabs(cross2(nodes_[2]->pt()-nodes_[0]->pt(), 
          nodes_[1]->pt()-nodes_[0]->pt()));
		}
	else
		{
      Point AB = nodes_[1]->pt() - nodes_[0]->pt();
      Point AC = nodes_[2]->pt() - nodes_[0]->pt();
      Point AD = nodes_[3]->pt() - nodes_[0]->pt();
			return 1.0/6.0 * fabs( AB * cross3(AC, AD) );
		}
}


Array<int> Element::showNodes() const
{
	Array<int> rtn(nodes_.length());
	for (int i=0; i<nodes_.length(); i++) rtn[i]=nodes_[i]->localIndex();
	return rtn;
}

bool Element::hasNoEdgeLabels() const
{
  for (int i=0; i<3; i++) 
  {
    if (edges_[i]->label() != -1) return false;
  }
  return true;
}

bool Element::hasNoFaceLabels() const
{
  for (int i=0; i<4; i++) 
  {
    if (faces_[i]->label() != -1) return false;
  }
  return true;
}
