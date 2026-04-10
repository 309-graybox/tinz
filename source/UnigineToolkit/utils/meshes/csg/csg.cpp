#include "csg.h"

#include "csg_helpers.h"

#include <UnigineMathLib.h>
#include <UnigineVisualizer.h>

using namespace Unigine;
using namespace Math;
using namespace csg;

namespace {
// compute polygon area (fan triangulation)
static double polygon_area(const Vector<Vertex> &poly)
{
	if (poly.size() < 3)
		return 0.0;
	const dvec3 &p0 = poly[0].pos;
	double A = 0.0;
	for (int i = 1; i + 1 < (int)poly.size(); ++i)
	{
		dvec3 a = poly[i].pos - p0;
		dvec3 b = poly[i + 1].pos - p0;
		A += length(cross(a, b));
	}
	return 0.5 * A;
}

// basis on plane for 2D projection
static void plane_basis(const dvec3 &n, dvec3 &u, dvec3 &v)
{
	dvec3 up = (abs(n.z) < 0.9) ? dvec3(0, 0, 1) : dvec3(0, 1, 0);
	u = normalize(cross(up, n));
	v = cross(n, u);
}

// 2D ear-clipping triangulation on polygon projected to its plane
static void triangulate_poly_ear(const CsgPolygon &poly, Vector<dvec3> &out_pos,
	Vector<dvec3> &out_nrm, Vector<dvec2> &out_uv, Vector<ivec3> &out_tris)
{
	const auto &verts = poly.verts;
	if (verts.size() < 3)
		return;
	const dvec3 n = normalize(poly.plane.n);

	dvec3 U, V;
	plane_basis(n, U, V);

	// project to 2D
	struct P2
	{
		double x, y;
	};
	Vector<P2> P;
	P.resize(verts.size());
	for (int i = 0; i < verts.size(); ++i)
	{
		const dvec3 &p = verts[i].pos;
		P[i] = {dot(U, p), dot(V, p)};
	}

	auto signed_area2 = [&]() {
		double s = 0.0;
		for (int i = 0; i < (int)P.size(); ++i)
		{
			int j = (i + 1) % P.size();
			s += P[i].x * P[j].y - P[j].x * P[i].y;
		}
		return s * 0.5;
	};
	bool ccw = signed_area2() > 0.0;

	auto is_convex = [&](int i) -> bool {
		int i0 = (i - 1 + (int)P.size()) % (int)P.size();
		int i1 = i;
		int i2 = (i + 1) % (int)P.size();
		dvec2 a(P[i1].x - P[i0].x, P[i1].y - P[i0].y);
		dvec2 b(P[i2].x - P[i1].x, P[i2].y - P[i1].y);
		double z = a.x * b.y - a.y * b.x;
		return ccw ? (z > -EPS) : (z < EPS);
	};
	auto point_in_tri = [&](const P2 &p, const P2 &a, const P2 &b, const P2 &c) -> bool {
		auto s = [](const P2 &p1, const P2 &p2, const P2 &p3) {
			return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
		};
		double s1 = s(p, a, b), s2 = s(p, b, c), s3 = s(p, c, a);
		bool has_neg = (s1 < -EPS) || (s2 < -EPS) || (s3 < -EPS);
		bool has_pos = (s1 > EPS) || (s2 > EPS) || (s3 > EPS);
		return !(has_neg && has_pos);
	};

	// indices into verts/P
	Vector<int> idx;
	idx.resize(verts.size());
	for (int i = 0; i < idx.size(); ++i)
		idx[i] = i;

	// append vertices to output buffer and remember base index
	int base = out_pos.size();
	for (const auto &vtx : verts)
	{
		out_pos.push_back(vtx.pos);
		out_nrm.push_back(vtx.normal.length2() > 0.0 ? normalize(vtx.normal) : n);
		out_uv.push_back(vtx.uv);
	}

	int guard = 0;
	while (idx.size() > 3 && guard < 10000)
	{
		++guard;
		bool cut = false;
		for (int k = 0; k < (int)idx.size(); ++k)
		{
			int i0 = idx[(k - 1 + (int)idx.size()) % (int)idx.size()];
			int i1 = idx[k];
			int i2 = idx[(k + 1) % (int)idx.size()];

			if (!is_convex(k))
				continue;

			// check if any other point lies inside ear
			bool any_inside = false;
			for (int m = 0; m < (int)idx.size(); ++m)
			{
				int im = idx[m];
				if (im == i0 || im == i1 || im == i2)
					continue;
				if (point_in_tri(P[im], P[i0], P[i1], P[i2]))
				{
					any_inside = true;
					break;
				}
			}
			if (any_inside)
				continue;

			// ear found
			out_tris.push_back(ivec3(base + i0, base + i1, base + i2));
			idx.remove(k);
			cut = true;
			break;
		}
		if (!cut)
			break;	  // degeneracy - fall back below
	}
	if (idx.size() == 3)
	{
		out_tris.push_back(ivec3(base + idx[0], base + idx[1], base + idx[2]));
	}
	else if (idx.size() > 3)
	{
		// fallback: fan (should be rare)
		for (int i = 1; i + 1 < (int)idx.size(); ++i)
			out_tris.push_back(ivec3(base + idx[0], base + idx[i], base + idx[i + 1]));
	}
}

void triangulatePolygons(const Vector<CsgPolygon> &polys, Vector<dvec3> &out_pos,
	Vector<dvec3> &out_nrm, Vector<dvec2> &out_uv, Vector<ivec3> &out_tris)
{
	for (const auto &p : polys)
	{
		if (p.verts.size() < 3)
			continue;
		if (polygon_area(p.verts) < AREA_EPS)
			continue;
		triangulate_poly_ear(p, out_pos, out_nrm, out_uv, out_tris);
	}
}
}	 // namespace

Plane::Plane(const dvec3 &normal, double W)
	: n(normal)
	, w(W)
{}

bool Plane::isValid() const
{
	return n.length2() > 0;
}

Plane Plane::fromPoints(const dvec3 &a, const dvec3 &b, const dvec3 &c)
{
	dvec3 n = normalize(cross(b - a, c - a));
	return {n, dot(n, a)};
}

int Plane::classify(const dvec3 &p) const
{
	double d = dot(n, p) - w;
	double tol = EPS * (1.0 + std::abs(w));
	if (d > tol)
		return 1;	 // front
	if (d < -tol)
		return -1;	  // back
	return 0;		  // coplanar
}

Vertex::Vertex(const dvec3 &p, const dvec3 &n, const dvec2 &t)
	: pos(p)
	, normal(n)
	, uv(t)
{}

Vertex Vertex::interpolate(const Vertex &other, double t) const
{
	return {pos + (other.pos - pos) * t, normalize(normal + (other.normal - normal) * t),
		uv + (other.uv - uv) * t};
}

CsgPolygon::CsgPolygon(const Vector<Vertex> &v, int mat)
	: verts(v)
	, plane(Plane::fromPoints(v[0].pos, v[1].pos, v[2].pos))
	, material(mat)
{}

// Split polygon by plane into coplanarFront, coplanarBack, front, back.
void CsgPolygon::split(const csg::Plane &plane, const CsgPolygon &poly,
	Vector<CsgPolygon> &coplanarFront, Vector<CsgPolygon> &coplanarBack, Vector<CsgPolygon> &front,
	Vector<CsgPolygon> &back)
{
	// Classify each vertex
	Vector<int> types(poly.verts.size());
	for (size_t i = 0; i < poly.verts.size(); ++i)
		types[i] = plane.classify(poly.verts[i].pos);

	bool hasFront = false, hasBack = false;
	for (int t : types)
	{
		hasFront |= (t == 1);
		hasBack |= (t == -1);
	}

	// Coplanar
	if (!hasFront && !hasBack)
	{
		if (dot(plane.n, poly.plane.n) > 0)
			coplanarFront.push_back(poly);
		else
			coplanarBack.push_back(poly);
		return;
	}

	// Completely in front or back
	if (!hasBack)
	{
		front.push_back(poly);
		return;
	}
	if (!hasFront)
	{
		back.push_back(poly);
		return;
	}

	// Needs to be split
	Vector<Vertex> f, b;
	for (size_t i = 0; i < poly.verts.size(); ++i)
	{
		size_t j = (i + 1) % poly.verts.size();
		const Vertex &vi = poly.verts[i];
		const Vertex &vj = poly.verts[j];
		int ti = types[i], tj = types[j];

		if (ti >= 0)
			f.push_back(vi);
		if (ti <= 0)
			b.push_back(vi);

		if ((ti == 1 && tj == -1) || (ti == -1 && tj == 1))
		{
			// compute intersection t on segment vi->vj: dot(n, p) = w
			dvec3 ab = vj.pos - vi.pos;
			double denom = dot(plane.n, ab);
			double tol = EPS * length(ab);
			if (std::abs(denom) > tol)
			{
				double t = (plane.w - dot(plane.n, vi.pos)) / denom;
				t = Math::clamp(t, 0.0, 1.0);
				if (t > T_EPS && t < 1.0 - T_EPS)
				{
					Vertex v = vi.interpolate(vj, t);
					f.push_back(v);
					b.push_back(v);
				}
			}
		}
	}

	// weld vertices
	auto weld_vertices = [&](Vector<Vertex> &verts) {
		for (int i = 0; i < verts.size(); ++i)
		{
			for (int j = i + 1; j < verts.size(); ++j)
			{
				if (length2(verts[i].pos - verts[j].pos) < EPS * EPS)
				{
					verts.remove(j);
					--j;
				}
			}
		}
	};
	weld_vertices(f);
	weld_vertices(b);

	if (f.size() >= 3 && polygon_area(f) >= AREA_EPS)
		front.emplace_back(f, poly.material);
	if (b.size() >= 3 && polygon_area(b) >= AREA_EPS)
		back.emplace_back(b, poly.material);
}

CsgNode::CsgNode(const Vector<CsgPolygon> &list)
{
	build(list);
}

CsgNode::~CsgNode()
{
	delete front;
	delete back;
}

void CsgNode::invert()
{
	for (auto &p : polygons)
	{
		std::reverse(p.verts.begin(), p.verts.end());
		for (auto &v : p.verts)
			v.normal = v.normal * -1.0;
		p.plane.n = p.plane.n * -1.0;
		p.plane.w = -p.plane.w;
	}
	plane.n = plane.n * -1.0;
	plane.w = -plane.w;

	if (front)
		front->invert();
	if (back)
		back->invert();
	std::swap(front, back);
}

void CsgNode::clipPolygons(Vector<CsgPolygon> &list) const
{
	Vector<CsgPolygon> f, b;

	for (const auto &p : list)
	{
		Vector<CsgPolygon> cf, cb, ff, bb;
		CsgPolygon::split(plane, p, cf, cb, ff, bb);

		if (!cf.empty())
			f.append(cf);
		if (!ff.empty())
			f.append(ff);

		if (!cb.empty())
			b.append(cb);
		if (!bb.empty())
			b.append(bb);
	}

	// if there is node, recursively clip front
	if (front)
		front->clipPolygons(f);
	// if there is no back-node - skip back-part (cut inside)
	if (back)
		back->clipPolygons(b);
	else
		b.clear();

	// merge result: front + back
	list.clear();
	list.append(f);
	list.append(b);
}

Vector<CsgPolygon> CsgNode::allPolygonsClipped(const Vector<CsgPolygon> &in)
{
	// For leaf without splitter plane treat as empty space = keep all
	return in;
}

void CsgNode::clipTo(const CsgNode &other)
{
	other.clipPolygons(polygons);
	if (front)
		front->clipTo(other);
	if (back)
		back->clipTo(other);
}

Vector<CsgPolygon> CsgNode::allPolygons() const
{
	Vector<CsgPolygon> r = polygons;
	if (front)
	{
		auto t = front->allPolygons();
		r.append(t);
	}
	if (back)
	{
		auto t = back->allPolygons();
		r.append(t);
	}
	return r;
}

void CsgNode::build(const Vector<CsgPolygon> &list, int depth)
{
	if (list.empty())
		return;

	if (depth > MAX_BSP_DEPTH)
	{
		polygons.append(list);
		return;
	}

	if (polygons.empty())
	{
		// polygons.push_back(list[0]);
		// plane = polygons[0].plane;

		Plane pl;
		bool found = false;
		for (const auto &poly : list)
		{
			if (good_plane_from_poly(poly, pl))
			{
				plane = pl;
				found = true;
				break;
			}
		}
		if (!found)
			return;
	}

	Vector<CsgPolygon> f, b;
	for (size_t i = 0; i < list.size(); ++i)
	{
		const auto &p = list[i];
		Vector<CsgPolygon> cf, cb, ff, bb;
		CsgPolygon::split(plane, p, cf, cb, ff, bb);
		polygons.append(cf);
		polygons.append(cb);
		if (!ff.empty())
			f.append(ff);
		if (!bb.empty())
			b.append(bb);
	}

	bool noFrontProgress = !f.empty() && (f.size() == list.size());
	bool noBackProgress = !b.empty() && (b.size() == list.size());
	if (noFrontProgress || noBackProgress)
	{
		// try to find another splitter from list
		Plane alt;
		bool switched = false;
		for (const auto &p : list)
		{
			if (good_plane_from_poly(p, alt))
			{
				plane = alt;
				switched = true;
				break;
			}
		}
		if (switched)
		{
			build(list, depth + 1);
			return;
		}

		// polygons.append(list);
		// return;
	}

	if (!f.empty())
	{
		if (!front)
			front = new CsgNode();
		front->build(f, depth + 1);
	}
	if (!b.empty())
	{
		if (!back)
			back = new CsgNode();
		back->build(b, depth + 1);
	}
}

bool CsgNode::is_finite(double x)
{
	return std::isfinite(x);
}

bool CsgNode::is_finite(const dvec3 &v)
{
	return is_finite(v.x) && is_finite(v.y) && is_finite(v.z);
}

bool CsgNode::good_plane_from_poly(const CsgPolygon &p, Plane &out)
{
	if (p.verts.size() < 3)
		return false;
	const dvec3 &a = p.verts[0].pos, &b = p.verts[1].pos, &c = p.verts[2].pos;
	if (!is_finite(a) || !is_finite(b) || !is_finite(c))
		return false;
	dvec3 n = cross(b - a, c - a);
	double len2 = dot(n, n);
	if (len2 < 1e-16)
		return false;	 // zero
	n = n / std::sqrt(len2);
	if (!is_finite(n))
		return false;
	out = Plane(n, dot(n, a));
	return is_finite(out.w);
}

// High-level CSG ops
Vector<CsgPolygon> csg::Union(const Vector<CsgPolygon> &a, const Vector<CsgPolygon> &b)
{
	CsgNode A(a), B(b);
	A.clipTo(B);
	B.clipTo(A);
	B.invert();
	B.clipTo(A);
	B.invert();
	A.build(B.allPolygons());
	return A.allPolygons();
}

Vector<CsgPolygon> csg::Subtract(const Vector<CsgPolygon> &a, const Vector<CsgPolygon> &b)
{
	CsgNode A(a), B(b);
	A.invert();
	A.clipTo(B);
	B.clipTo(A);
	B.invert();
	B.clipTo(A);
	B.invert();
	A.build(B.allPolygons());
	A.invert();
	return A.allPolygons();
}

Vector<CsgPolygon> csg::Intersect(const Vector<CsgPolygon> &a, const Vector<CsgPolygon> &b)
{
	CsgNode A(a), B(b);
	A.invert();
	B.clipTo(A);
	B.invert();
	A.clipTo(B);
	B.clipTo(A);
	A.build(B.allPolygons());
	A.invert();
	return A.allPolygons();
}

Vector<CsgPolygon> csg::transformCsg(
	const Vector<CsgPolygon> &csg, const Unigine::Math::dmat4 &transform)
{
	Vector<CsgPolygon> result = csg;

	quat Nm = transform.getRotate();	// transpose(inverse(mat3(transform)));

	for (int i = 0; i < result.size(); i++)
	{
		auto &poly = result[i];
		for (int j = 0; j < poly.verts.size(); j++)
		{
			auto &v = poly.verts[j];
			v.pos = transform * v.pos;
			if (length2(v.normal) > 0.0)
				v.normal = normalize(Nm * v.normal);
		}
		if (poly.verts.size() >= 3)
		{
			const auto &a = poly.verts[0].pos;
			const auto &b = poly.verts[1].pos;
			const auto &c = poly.verts[2].pos;
			poly.plane = Plane::fromPoints(a, b, c);
		}
	}

	return result;
}

Unigine::Vector<CsgPolygon> csg::meshToCsg(const Unigine::MeshPtr &mesh)
{
	Vector<CsgPolygon> polys;
	for (int s = 0; s < mesh->getNumSurfaces(); s++)
	{
		mesh->remapCVertex(s);
		int normals = mesh->getNumNormals(s);

		for (int i = 0; i < mesh->getNumIndices(s); i += 3)
		{
			int i0 = mesh->getIndex(i + 0, s);
			int i1 = mesh->getIndex(i + 1, s);
			int i2 = mesh->getIndex(i + 2, s);

			vec3 v0 = mesh->getVertex(i0, s);
			vec3 v1 = mesh->getVertex(i1, s);
			vec3 v2 = mesh->getVertex(i2, s);

			vec3 calc_n = normalize(cross(v1 - v0, v2 - v0));

			vec3 n0 = i0 < normals ? mesh->getNormal(i0, s) : calc_n;
			vec3 n1 = i1 < normals ? mesh->getNormal(i1, s) : calc_n;
			vec3 n2 = i2 < normals ? mesh->getNormal(i2, s) : calc_n;

			vec2 uv0 = mesh->getTexCoord0(i0, s);
			vec2 uv1 = mesh->getTexCoord0(i1, s);
			vec2 uv2 = mesh->getTexCoord0(i2, s);

			Vertex fv0{dvec3(v0), dvec3(n0), dvec2(uv0)};
			Vertex fv1{dvec3(v1), dvec3(n1), dvec2(uv1)};
			Vertex fv2{dvec3(v2), dvec3(n2), dvec2(uv2)};

			polys.emplace_back(Vector<Vertex>{fv0, fv1, fv2}, s);
		}
	}
	return polys;
}

Vector<CsgPolygon> csg::meshToPolys(const Vector<dvec3> &positions, const Vector<dvec3> &normals,
	const Vector<dvec2> &uvs, const Vector<std::array<int, 3>> &triangles, int material_id)
{
	Vector<CsgPolygon> polys;
	polys.reserve(triangles.size());
	for (auto tri : triangles)
	{
		Vertex v0{positions[tri[0]], normals.empty() ? dvec3{} : normals[tri[0]],
			uvs.empty() ? dvec2{} : uvs[tri[0]]};
		Vertex v1{positions[tri[1]], normals.empty() ? dvec3{} : normals[tri[1]],
			uvs.empty() ? dvec2{} : uvs[tri[1]]};
		Vertex v2{positions[tri[2]], normals.empty() ? dvec3{} : normals[tri[2]],
			uvs.empty() ? dvec2{} : uvs[tri[2]]};

		// recalculate normals inside Polygon constructor
		polys.emplace_back(Vector<Vertex>{v0, v1, v2}, material_id);
	}
	return polys;
}

void csg::CsgToMesh(const Unigine::Vector<CsgPolygon> &polys, const Unigine::MeshPtr &out_mesh)
{
	csg::MeshStruct m;

	// old fan triangulation for n-gon
	/*
	out_mesh->clear();
	out_mesh->addSurface();
	for (const auto &p : polys)
	{
		if (p.verts.size() < 3)
			continue;
		int surface = 0;
		int base = out_mesh->getNumVertex(surface);
		for (const auto &v : p.verts)
		{
			out_mesh->addVertex(vec3(v.pos), surface);
			out_mesh->addNormal(vec3(v.normal), surface);
			out_mesh->addTexCoord0(vec2(v.uv), surface);
		}
		for (int i = 1; i + 1 < (int)p.verts.size(); ++i)
		{
			out_mesh->addIndex(base);
			out_mesh->addIndex(base + i);
			out_mesh->addIndex(base + i + 1);
		}
	}
	*/

	// use ear-clipping triangulation
	triangulatePolygons(polys, m.positions, m.normals, m.uvs, m.indices);

	// optimize result
	m = MergeCoplanarFaces(m);
	m = WeldPolysByPosition(m);

	// fill MeshPtr
	out_mesh->clear();
	out_mesh->addSurface();
	for (int i = 0; i < m.positions.size(); i++)
	{
		out_mesh->addVertex(vec3(m.positions[i]), 0);
		out_mesh->addNormal(vec3(m.normals[i]), 0);
		out_mesh->addTexCoord0(vec2(m.uvs[i]), 0);
	}
	for (int i = 0; i < m.indices.size(); i++)
	{
		auto &t = m.indices[i];
		out_mesh->addIndex(t.x);
		out_mesh->addIndex(t.y);
		out_mesh->addIndex(t.z);
	}
}

void csg::polysToMesh(const Vector<CsgPolygon> &polys, Vector<dvec3> &out_pos,
	Vector<dvec3> &out_nrm, Vector<dvec2> &out_uv, Vector<std::array<int, 3>> &out_tris)
{
	for (const auto &p : polys)
	{
		if (p.verts.size() < 3)
			continue;
		int base = static_cast<int>(out_pos.size());
		for (const auto &v : p.verts)
		{
			out_pos.push_back(v.pos);
			out_nrm.push_back(v.normal);
			out_uv.push_back(v.uv);
		}
		for (int i = 1; i + 1 < (int)p.verts.size(); ++i)
			out_tris.push_back({base, base + i, base + i + 1});
	}
}

void Boolean::Union(const Unigine::ObjectMeshStaticPtr &a, const Unigine::ObjectMeshStaticPtr &b,
	bool rewrite_file_a)
{
	Vector<CsgPolygon> a_csg, b_csg;
	convert_to_csg(a, b, a_csg, b_csg);
	Vector<CsgPolygon> result = csg::Union(a_csg, b_csg);
	convert_to_mesh(a, result, rewrite_file_a);
}

void Boolean::Subtract(const Unigine::ObjectMeshStaticPtr &a, const Unigine::ObjectMeshStaticPtr &b,
	bool rewrite_file_a)
{
	Vector<CsgPolygon> a_csg, b_csg;
	convert_to_csg(a, b, a_csg, b_csg);
	Vector<CsgPolygon> result = csg::Subtract(a_csg, b_csg);
	convert_to_mesh(a, result, rewrite_file_a);
}

void Boolean::Intersect(const Unigine::ObjectMeshStaticPtr &a,
	const Unigine::ObjectMeshStaticPtr &b, bool rewrite_file_a)
{
	Vector<CsgPolygon> a_csg, b_csg;
	convert_to_csg(a, b, a_csg, b_csg);
	Vector<CsgPolygon> result = csg::Intersect(a_csg, b_csg);
	convert_to_mesh(a, result, rewrite_file_a);
}

void Boolean::convert_to_csg(const Unigine::ObjectMeshStaticPtr &a,
	const Unigine::ObjectMeshStaticPtr &b, Vector<CsgPolygon> &a_csg, Vector<CsgPolygon> &b_csg)
{
	a->loadForceRAM();
	MeshPtr a_mesh = Mesh::create();
	a->getCopyMeshRAM(a_mesh);

	b->loadForceRAM();
	MeshPtr b_mesh = Mesh::create();
	b->getCopyMeshRAM(b_mesh);

	a_csg = meshToCsg(a_mesh);
	b_csg =
		transformCsg(meshToCsg(b_mesh), dmat4(a->getIWorldTransform() * b->getWorldTransform()));
}

void Boolean::convert_to_mesh(
	const Unigine::ObjectMeshStaticPtr &a, Vector<CsgPolygon> &a_csg, bool rewrite_file)
{
	MeshPtr a_mesh = Mesh::create();
	CsgToMesh(a_csg, a_mesh);

	a_mesh->createTangents();
	a_mesh->createCollisionData();

	if (rewrite_file)
	{
		a->setMeshProceduralMode(ObjectMeshStatic::PROCEDURAL_MODE_DISABLE);
		StringStack<> path = a->getMeshPath();
		a_mesh->setSurfaceName(0, a->getSurfaceName(0));
		a_mesh->save(path);
	}
	else
	{
		a->setMeshProceduralMode(ObjectMeshStatic::PROCEDURAL_MODE_DYNAMIC);
		a->applyMoveMeshProceduralForce(a_mesh);
	}
}
