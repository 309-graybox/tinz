/*
Example of usage:

Unigine::Vector<CsgPolygon> polys = csg::meshToCsg(mesh);

// merges vertices
 * within a specific distance of one another
WeldPolysByPosition(polys, 1e-5);

// check that
 * geometry is ready for CSG operations:
// 1. It should be watertight (2-manifold)
auto stats =
 * CheckPolysWatertight(polys, 1e-5);
if (stats.open_edges > 0)
	CapOpenBoundariesOnPolys(polys,
 * 1e-5); // cap open loops

// 2. Geometry shouldn't has self intersections between triangles
auto
 * self_int = CheckPolysSelfIntersections(polys);
Log::message("Self-intersection check: %d tris, %d
 * intersecting pairs\n", self_int.tri_count,
	self_int.intersections);
*/

#pragma once

#include "csg.h"

#include <UnigineMathLib.h>
#include <UnigineMesh.h>
#include <array>
#include <unordered_map>
#include <unordered_set>

namespace csg {
//////////////////////////////////////////////////////////////////////////
// CHECKS
//////////////////////////////////////////////////////////////////////////

struct SelfIntersectionStats
{
	int tri_count = 0;
	int intersections = 0;
};
SelfIntersectionStats CheckPolysSelfIntersections(
	const Unigine::Vector<CsgPolygon> &polys, double eps = 1e-6);

struct MeshCheckStats
{
	int tris = 0;
	int open_edges = 0;
	int nonmanifold_edges = 0;
	int degenerate_tris = 0;
};
MeshCheckStats CheckPolysWatertight(const Unigine::Vector<CsgPolygon> &polys, double eps = 1e-6);

//////////////////////////////////////////////////////////////////////////
// OPERATIONS
//////////////////////////////////////////////////////////////////////////

void CapOpenBoundariesOnPolys(Unigine::Vector<CsgPolygon> &polys, double eps = 1e-6);
void WeldPolysByPosition(Unigine::Vector<CsgPolygon> &polys, double eps = 1e-6);
Unigine::Vector<CsgPolygon> MergeCoplanarFaces(const Unigine::Vector<CsgPolygon> &polys,
	double n_snap = 1e-6, double w_snap = 1e-6, double xy_snap = 1e-6);

struct MeshStruct
{
	Unigine::Vector<Unigine::Math::dvec3> positions;
	Unigine::Vector<Unigine::Math::dvec3> normals;
	Unigine::Vector<Unigine::Math::dvec2> uvs;
	Unigine::Vector<Unigine::Math::ivec3> indices;	  // triangles
};
MeshStruct WeldPolysByPosition(const MeshStruct &mesh, double eps = 1e-6, double normal_tol = 1e-3);
MeshStruct MergeCoplanarFaces(
	const MeshStruct &mesh, double n_snap = 1e-6, double w_snap = 1e-6, double xy_snap = 1e-6);
}	 // namespace csg
