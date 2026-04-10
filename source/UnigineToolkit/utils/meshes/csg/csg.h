// Constructive Solid Geometry (CSG) is a modeling technique that uses Boolean
// operations like union and intersection to combine 3D solids. This library
// implements CSG operations on meshes elegantly and concisely using BSP trees,
// and is meant to serve as an easily understandable implementation of the
// algorithm. All edge cases involving overlapping coplanar polygons in both
// solids are correctly handled.
//
// Provides: union, subtract, intersect.
// MIT License
// Based on the CSG algorithm by Evan Wallace (csg.js) and common BSP implementations.
// ------------------------------------------------------------
// Usage example:
// int main()
// {
//     Unigine::Vector<Vec3> Apos{/* ... */}, Bpos{/* ... */};
//     Unigine::Vector<std::array<int,3>> Atri{/* ... */}, Btri{/* ... */};
//
//     auto A = meshToPolys(Apos, {}, {}, Atri);
//     auto B = meshToPolys(Bpos, {}, {}, Btri);
//
//     auto U = csg::Union(A, B);
//     auto I = csg::Intersect(A, B);
//     auto S = csg::Subtract(A, B);
//
//     // convert to mesh
//     Unigine::Vector<Vec3> pos; Unigine::Vector<Vec3> nrm; Unigine::Vector<Vec2> uv;
//     Unigine::Vector<std::array<int,3>> tri;
//     polysToMesh(U, pos, nrm, uv, tri);
//     // ...fill MeshPtr
// }

#pragma once
#include <UnigineMathLib.h>
#include <UnigineMesh.h>
#include <UnigineObjects.h>
#include <array>

namespace csg {
constexpr double EPS = 1e-6;		  // base classify tolerance
constexpr double T_EPS = 1e-9;		  // avoid t at [0,1]
constexpr double AREA_EPS = 1e-12;	  // minimal polygon area
constexpr int MAX_BSP_DEPTH = 512;

struct Plane
{
	Unigine::Math::dvec3 n;	   // unit normal
	double w;				   // distance: dot(n, p) = w
	Plane() = default;
	Plane(const Unigine::Math::dvec3 &normal, double W);
	bool isValid() const;
	static Plane fromPoints(const Unigine::Math::dvec3 &a, const Unigine::Math::dvec3 &b,
		const Unigine::Math::dvec3 &c);
	int classify(const Unigine::Math::dvec3 &p) const;
};

struct Vertex
{
	Unigine::Math::dvec3 pos;
	Unigine::Math::dvec3 normal;
	Unigine::Math::dvec2 uv;
	Vertex() = default;
	Vertex(const Unigine::Math::dvec3 &p, const Unigine::Math::dvec3 &n,
		const Unigine::Math::dvec2 &t = {});
	Vertex interpolate(const Vertex &other, double t) const;
};

struct CsgPolygon
{
	Unigine::Vector<Vertex> verts;
	Plane plane;
	int material = 0;	 // optional

	CsgPolygon() = default;
	CsgPolygon(const Unigine::Vector<Vertex> &v, int mat = 0);

	// Split polygon by plane into coplanarFront, coplanarBack, front, back.
	static void split(const csg::Plane &plane, const CsgPolygon &poly,
		Unigine::Vector<CsgPolygon> &coplanarFront, Unigine::Vector<CsgPolygon> &coplanarBack,
		Unigine::Vector<CsgPolygon> &front, Unigine::Vector<CsgPolygon> &back);
};

struct CsgNode
{
	Unigine::Vector<CsgPolygon> polygons;
	CsgNode *front{nullptr};
	CsgNode *back{nullptr};
	Plane plane;

	CsgNode() = default;
	explicit CsgNode(const Unigine::Vector<CsgPolygon> &list);
	~CsgNode();

	void invert();
	void clipPolygons(Unigine::Vector<CsgPolygon> &list) const;
	static Unigine::Vector<CsgPolygon> allPolygonsClipped(const Unigine::Vector<CsgPolygon> &in);
	void clipTo(const CsgNode &other);
	Unigine::Vector<CsgPolygon> allPolygons() const;
	void build(const Unigine::Vector<CsgPolygon> &list, int depth = 0);

private:
	bool is_finite(double x);
	bool is_finite(const Unigine::Math::dvec3 &v);
	bool good_plane_from_poly(const CsgPolygon &p, Plane &out);
};

// High-level CSG ops
Unigine::Vector<CsgPolygon> Union(
	const Unigine::Vector<CsgPolygon> &a, const Unigine::Vector<CsgPolygon> &b);
Unigine::Vector<CsgPolygon> Subtract(
	const Unigine::Vector<CsgPolygon> &a, const Unigine::Vector<CsgPolygon> &b);
Unigine::Vector<CsgPolygon> Intersect(
	const Unigine::Vector<CsgPolygon> &a, const Unigine::Vector<CsgPolygon> &b);

Unigine::Vector<CsgPolygon> transformCsg(
	const Unigine::Vector<CsgPolygon> &csg, const Unigine::Math::dmat4 &transform);

// convert mesh to csg polygons
Unigine::Vector<CsgPolygon> meshToCsg(const Unigine::MeshPtr &mesh);
Unigine::Vector<CsgPolygon> meshToPolys(const Unigine::Vector<Unigine::Math::dvec3> &positions,
	const Unigine::Vector<Unigine::Math::dvec3> &normals,
	const Unigine::Vector<Unigine::Math::dvec2> &uvs,
	const Unigine::Vector<std::array<int, 3>> &triangles, int material_id = 0);

// convert csg polygons to mesh
void CsgToMesh(const Unigine::Vector<CsgPolygon> &polys, const Unigine::MeshPtr &out_mesh);
void polysToMesh(const Unigine::Vector<CsgPolygon> &polys,
	Unigine::Vector<Unigine::Math::dvec3> &out_pos, Unigine::Vector<Unigine::Math::dvec3> &out_nrm,
	Unigine::Vector<Unigine::Math::dvec2> &out_uv, Unigine::Vector<std::array<int, 3>> &out_tris);

class Boolean
{
public:
	// a - first object
	// b - second object
	// rewrite_file_a - save result of operation to a mesh file of the first object. Otherwise
	//					use runtime-only (not persistent) modification
	static void Union(const Unigine::ObjectMeshStaticPtr &a, const Unigine::ObjectMeshStaticPtr &b,
		bool rewrite_file_a = false);
	static void Subtract(const Unigine::ObjectMeshStaticPtr &a,
		const Unigine::ObjectMeshStaticPtr &b, bool rewrite_file_a = false);
	static void Intersect(const Unigine::ObjectMeshStaticPtr &a,
		const Unigine::ObjectMeshStaticPtr &b, bool rewrite_file_a = false);

private:
	static void convert_to_csg(const Unigine::ObjectMeshStaticPtr &a,
		const Unigine::ObjectMeshStaticPtr &b, Unigine::Vector<CsgPolygon> &a_csg,
		Unigine::Vector<CsgPolygon> &b_csg);
	static void convert_to_mesh(const Unigine::ObjectMeshStaticPtr &a,
		Unigine::Vector<CsgPolygon> &a_csg, bool rewrite_file = false);
};

}	 // namespace csg
