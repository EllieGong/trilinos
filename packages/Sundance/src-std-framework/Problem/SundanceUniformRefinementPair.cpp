#include "SundanceUniformRefinementPair.hpp"
using std::cout;
using std::endl;
using std::setw;

namespace Sundance
{
using namespace Teuchos;

UniformRefinementPair::UniformRefinementPair(const MeshType& meshType,
  const Mesh& coarse)
  : meshType_(meshType),
    coarse_(coarse),
    fine_()
{
  if (coarse.spatialDim() == 2)
  {
    refineTriMesh();
  }
  TEST_FOR_EXCEPT(coarse.spatialDim() != 2);
}

void UniformRefinementPair::refineTriMesh()
{
  const MPIComm& comm = coarse_.comm();
  fine_ = meshType_.createEmptyMesh(2, comm);

  TEST_FOR_EXCEPT(fine_.spatialDim() != 2);
  TEST_FOR_EXCEPT(comm.getNProc() > 1);

  int numVerts = coarse_.numCells(0);
  int numEdges = coarse_.numCells(1);
  int numElems = coarse_.numCells(2);

  int numNewVerts = numVerts + numEdges;
  int numNewEdges = 2*numEdges + 3*numElems;
  int numNewElems = 4*numElems;

  Array<int> vertDone(numVerts, 0);
  Array<int> oldEdgeDone(numEdges, 0);
  Array<int> newEdgeDone(numNewEdges, 0);

  oldToNewVertMap_ = Array<int>(numVerts, -1);
  newVertToOldLIDMap_ = Array<int>(numVerts+numEdges, -1);
  newVertIsOnEdge_ = Array<int>(numNewVerts, 0);
  oldEdgeToNewVertMap_ = Array<int>(numEdges, -1);
    
  ArrayOfTuples<int> elemVerts(numElems, 6);
  oldToNewElemMap_ = ArrayOfTuples<int>(numElems, 4);
  newToOldElemMap_ = Array<int>(numNewElems, -1);
  oldEdgeChildren_ = Array<Array<int> >(numEdges);
  oldEdgeParallels_ = Array<Array<int> >(numEdges);
  newEdgeParents_ = Array<int>(numNewEdges, -1);
  newEdgeParallels_ = Array<int>(numNewEdges, -1);
  
  int newVertLID = 0;

  for (int c=0; c<numElems; c++)
  {
    /* add the old vertices to the new mesh */
    const int* verts = coarse_.elemZeroFacetView(c);
    for (int i=0; i<3; i++)
    {
      int v = verts[i];
      if (!vertDone[v])
      {
        int vertOwner = coarse_.ownerProcID(0, v);
        int vertLabel = coarse_.label(0, v);
        fine_.addVertex(newVertLID, coarse_.nodePosition(v), vertOwner, vertLabel);

        oldToNewVertMap_[v] = newVertLID;
        newVertToOldLIDMap_[newVertLID] = v;
        elemVerts.value(c, i) = newVertLID;
        newVertLID++;
        vertDone[v]=true;
      }
      else
      {
        elemVerts.value(c, i) = oldToNewVertMap_[v];
      }
    }
  
    /* put new vertices at the centroids of the old edges */
    for (int i=0; i<3; i++)
    {
      int ori;
      int e = coarse_.facetLID(2, c, 1, i, ori);
      if (!oldEdgeDone[e])
      {
        int edgeOwner = coarse_.ownerProcID(1, e);
        Point centroid = coarse_.centroid(1, e);
        fine_.addVertex(newVertLID, centroid, edgeOwner, 0);
        elemVerts.value(c, 3+i) = newVertLID;
        oldEdgeToNewVertMap_[e] = newVertLID;
        newVertIsOnEdge_[newVertLID] = true;
        newVertToOldLIDMap_[newVertLID] = e;
        oldEdgeDone[e] = true;
        newVertLID++;
      }
      else
      {
        elemVerts.value(c, 3+i) = oldEdgeToNewVertMap_[e];
      }
    }
  }

  Array<Array<int> > pairs
    = tuple<Array<int> >(
      tuple(0,1),
      tuple(1,2),
      tuple(2,0)
      );

  Array<Array<int> > vertPtrs 
    = tuple<Array<int> >(
      tuple(0,4,5),
      tuple(1,3,5),
      tuple(3,4,5),
      tuple(2,3,4)
      );
  
  int newElemLID=0;      
  for (int c=0; c<numElems; c++)
  {
    /* find the coarse edges */
    Array<int> edgeLIDs(3);
    for (int s=0; s<3; s++)
    {
      int ori = 0;
      edgeLIDs[s] = coarse_.facetLID(2, c, 1, s, ori);
    }

    /* make the new elements */
    int elemOwner = coarse_.ownerProcID(2, c);
    int elemLabel = coarse_.label(2, c);
    for (int p=0; p<vertPtrs.size(); p++)
    {
      Array<int> verts = tuple(elemVerts.value(c, vertPtrs[p][0]),
        elemVerts.value(c, vertPtrs[p][1]),
        elemVerts.value(c, vertPtrs[p][2]));
      fine_.addElement(newElemLID, verts, elemOwner, elemLabel);
      /* put the new elements in the map */
      oldToNewElemMap_.value(c, p) = newElemLID;
      newToOldElemMap_[newElemLID] = c;

      /* Find the relations between old and new edges */
      for (int side=0; side<3; side++)
      {
        int v1 = verts[pairs[side][0]];
        int v2 = verts[pairs[side][1]];
        int newEdge = lookupEdge(fine_, v1, v2);
        if (newEdgeDone[newEdge]) continue;
        
        
        if (newVertIsOnEdge_[v1] && newVertIsOnEdge_[v2]) // edge is interior
        {
          /* find the coarse edge parallel to the new interior edge */
          int parEdge = -1;
          for (int i=0; i<3; i++)
          {
            int ev = oldEdgeToNewVertMap_[edgeLIDs[i]];
            if (ev==v1 || ev==v2) continue;
            parEdge = edgeLIDs[i];
            break;
          }
          TEST_FOR_EXCEPT(parEdge==-1);
          oldEdgeParallels_[parEdge].append(newEdge);
          newEdgeParallels_[newEdge] = parEdge;
        }
        else // edge is a child
        {
          for (int n=0; n<2; n++)
          {
            int v = verts[pairs[side][n]];
            if (newVertIsOnEdge_[v]) 
            {
              int oldEdge = newVertToOldLIDMap_[v];
              oldEdgeChildren_[oldEdge].append(newEdge);
              newEdgeParents_[newEdge] = oldEdge;
            }
          }
        }
        newEdgeDone[newEdge] = true;
      }
      newElemLID++;
    }
  }

  TEST_FOR_EXCEPT(newElemLID != 4*numElems);


}

int UniformRefinementPair::lookupEdge(const Mesh& mesh,
  int v1, int v2) const
{
  Array<int> cofacetLIDs;
  mesh.getCofacets(0, v1, 1, cofacetLIDs);
  for (int c=0; c<cofacetLIDs.size(); c++)
  {
    int ori;
    for (int f=0; f<2; f++)
    {
      int v = mesh.facetLID(1, cofacetLIDs[c], 0, f, ori);
      if (v == v2) return cofacetLIDs[c];
    }
  }

  TEST_FOR_EXCEPTION(true, std::runtime_error,
    "edge (" << v1 << ", " << v2 << ") not found in mesh");
  return -1;
}


int UniformRefinementPair::check() const 
{
  int bad = 0;
  double tol = 1.0e-12;
  cout << "old vertex to new vertex map" << endl;
  cout << "----------------------------------------" << endl;
  for (int v=0; v<coarse_.numCells(0); v++)
  {
    int vNew = oldToNewVertMap()[v];
    double d = coarse_.centroid(0, v).distance(fine_.centroid(0, vNew));
    string state = "GOOD";
    if (d > tol) {state="BAD"; bad++;}
    if (fine_.label(0,vNew) != coarse_.label(0,v)) {state="BAD"; bad++;}
    cout << setw(4) << v 
         << setw(16) << coarse_.centroid(0, v) 
         << setw(4) << coarse_.label(0,v)
         << setw(4) << vNew 
         << setw(16) << fine_.centroid(0, vNew) 
         << setw(4) << fine_.label(0,vNew)
         << setw(6) << state << endl;
  }

  cout << endl << endl;

  cout << "old edge to new vertex map" << endl;
  cout << "----------------------------------------" << endl;
  for (int e=0; e<coarse_.numCells(1); e++)
  {
    int vNew = oldEdgeToNewVertMap()[e];
    double d = coarse_.centroid(1, e).distance(fine_.centroid(0, vNew));
    string state = "GOOD";
    if (d > tol) {state="BAD"; bad++;}
    cout << setw(4) << e << setw(16) 
         << coarse_.centroid(1, e) << "\t" << setw(10)
         << vNew << setw(16)
         << fine_.centroid(0, vNew) << setw(6) << state  << endl;
  }

  cout << endl << endl;
  cout << "old vertex to new vertex map inversion" << endl;
  cout << "----------------------------------------" << endl;
  for (int v=0; v<coarse_.numCells(0); v++)
  {
    int vNew = oldToNewVertMap()[v];
    int vOld = newVertToOldLIDMap()[vNew];
      
    string state = "GOOD";
    if (vOld != v) {state="BAD"; bad++;}
    cout << setw(4) << v << setw(4) << vNew << setw(4) << vOld
         << setw(6) << state << endl;
  }

  cout << endl << endl;
  cout << "old edge to new vertex map inversion" << endl;
  cout << "----------------------------------------" << endl;
  for (int e=0; e<coarse_.numCells(1); e++)
  {
    int vNew = oldEdgeToNewVertMap()[e];
    int eOld = newVertToOldLIDMap()[vNew];
      
    string state = "GOOD";
    if (eOld != e) {state="BAD"; bad++;}
    cout << setw(4) << e << setw(4) << vNew << setw(4) << eOld
         << setw(6) << state << endl;
  }




  cout << endl << endl;
  cout << "old to new elem map" << endl;
  cout << "----------------------------------------" << endl;
  for (int c=0; c<coarse_.numCells(2); c++)
  {
    cout << setw(6) << c;
    for (int e=0; e<4; e++) 
    {
      int eNew = oldToNewElemMap().value(c, e);
      int eOld = newToOldElemMap()[eNew];
      cout << setw(4) << eNew ;
      string state = "OK";
      if (eOld != c) {state="BAD"; bad++;}
      cout << setw(4) << state;
    }
    cout << endl;
      
  }

  cout << endl << endl;
  cout << "old edge children and parallels" << endl;
  cout << "----------------------------------------" << endl;
  for (int e=0; e<coarse_.numCells(1); e++)
  {
    string state = "GOOD";
    const Array<int>& children = oldEdgeChildren()[e];
    const Array<int>& parallels = oldEdgeParallels()[e];
    Array<int> parents(children.size());
    Array<int> coarsePar(parallels.size());
    for (int i=0; i<children.size(); i++) 
    {
      parents[i] = newEdgeParents()[children[i]];
      if (parents[i] != e) {state="BAD"; bad++;}
    }
    for (int i=0; i<parallels.size(); i++) 
    {
      coarsePar[i] = newEdgeParallels()[parallels[i]];
      if (coarsePar[i] != e) {state="BAD"; bad++;}
    }
      
    cout << setw(4) << e << setw(12) << children 
         << setw(12) << parents 
         << setw(12) << parallels
         << setw(12) << coarsePar 
         << setw(6) << state << endl;
  }

  return bad;
}

}
