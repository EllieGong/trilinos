#include "CLArgs.hpp"
#include "MeshImpl.hpp"

void add_domain_args( CLArgs& args );

Mesquite::MeshDomain* process_domain_args( Mesquite::MeshImpl* mesh );

const char SPHERE_FLAG = 'S';
const char PLANE_FLAG = 'P';
const char CYLINDER_FLAG = 'C';
const char LINE_FLAG = 'l';
const char CIRCLE_FLAG = 'c';
const char POINT_FLAG = 'v';
const char SKIN_FLAG = 's';
