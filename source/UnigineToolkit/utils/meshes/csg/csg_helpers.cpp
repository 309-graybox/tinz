#include "csg_helpers.h"

#include <UnigineMathLib.h>

using namespace Unigine;
using namespace Math;
using namespace csg;

namespace {
static inline double _tt_dot(const Unigine::Math::dvec3 &a, const Unigine::Math::dvec3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
static inline Unigine::Math::dvec3 _tt_cross(
	const Unigine::Math::dvec3 &a, const Unigine::Math::dvec3 &b)
{
	return dvec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// 2D helpers for coplanar case (projected)
struct _tt_vec2
{
	double x, y;
};
static inline double _tt_orient2d(const _tt_vec2 &a, const _tt_vec2 &b, const _tt_vec2 &c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
static bool _tt_seg_overlap_2d(
	const _tt_vec2 &a, const _tt_vec2 &b, const _tt_vec2 &c, const _tt_vec2 &d)
{
	auto sgn = [](double v) { return (v > 0) - (v < 0); };
	double o1 = _tt_orient2d(a, b, c);
	double o2 = _tt_orient2d(a, b, d);
	double o3 = _tt_orient2d(c, d, a);
	double o4 = _tt_orient2d(c, d, b);

	if ((sgn(o1) * sgn(o2) < 0) && (sgn(o3) * sgn(o4) < 0))
		return true;

	auto on_seg = [](const _tt_vec2 &p, const _tt_vec2 &q, const _tt_vec2 &r) {
		if (std::min(p.x, r.x) - 1e-12 <= q.x && q.x <= std::max(p.x, r.x) + 1e-12
			&& std::min(p.y, r.y) - 1e-12 <= q.y && q.y <= std::max(p.y, r.y) + 1e-12)
			return true;
		return false;
	};
	if (std::abs(o1) < 1e-12 && on_seg(a, c, b))
		return true;
	if (std::abs(o2) < 1e-12 && on_seg(a, d, b))
		return true;
	if (std::abs(o3) < 1e-12 && on_seg(c, a, d))
		return true;
	if (std::abs(o4) < 1e-12 && on_seg(c, b, d))
		return true;
	return false;
}
static bool _tt_point_in_tri_2d(
	const _tt_vec2 &p, const _tt_vec2 &a, const _tt_vec2 &b, const _tt_vec2 &c)
{
	double o1 = _tt_orient2d(a, b, p);
	double o2 = _tt_orient2d(b, c, p);
	double o3 = _tt_orient2d(c, a, p);
	bool has_neg = (o1 < -1e-12) || (o2 < -1e-12) || (o3 < -1e-12);
	bool has_pos = (o1 > 1e-12) || (o2 > 1e-12) || (o3 > 1e-12);
	return !(has_neg && has_pos);
}
static bool _tt_coplanar_tri_tri_3d(const Unigine::Math::dvec3 &A, const Unigine::Math::dvec3 &B,
	const Unigine::Math::dvec3 &C, const Unigine::Math::dvec3 &D, const Unigine::Math::dvec3 &E,
	const Unigine::Math::dvec3 &F, const Unigine::Math::dvec3 &N)
{
	// select the projection axis (onto the plane with the largest normal component)
	dvec3 n = dvec3(std::abs(N.x), std::abs(N.y), std::abs(N.z));
	int i0 = 0;
	if (n.y > n.x)
		i0 = 1;
	if ((i0 == 0 && n.z > n.x) || (i0 == 1 && n.z > n.y))
		i0 = 2;

	auto proj = [i0](const dvec3 &p) -> _tt_vec2 {
		if (i0 == 0)
			return {p.y, p.z};
		if (i0 == 1)
			return {p.x, p.z};
		return {p.x, p.y};
	};
	_tt_vec2 a = proj(A), b = proj(B), c = proj(C);
	_tt_vec2 d = proj(D), e = proj(E), f = proj(F);

	// check for edge intersections
	if (_tt_seg_overlap_2d(a, b, d, e) || _tt_seg_overlap_2d(a, b, e, f)
		|| _tt_seg_overlap_2d(a, b, f, d))
		return true;
	if (_tt_seg_overlap_2d(b, c, d, e) || _tt_seg_overlap_2d(b, c, e, f)
		|| _tt_seg_overlap_2d(b, c, f, d))
		return true;
	if (_tt_seg_overlap_2d(c, a, d, e) || _tt_seg_overlap_2d(c, a, e, f)
		|| _tt_seg_overlap_2d(c, a, f, d))
		return true;

	// or if one lies inside the other
	if (_tt_point_in_tri_2d(a, d, e, f) || _tt_point_in_tri_2d(d, a, b, c))
		return true;
	return false;
}

static bool tri_tri_overlap_test(const Unigine::Math::dvec3 &V0, const Unigine::Math::dvec3 &V1,
	const Unigine::Math::dvec3 &V2, const Unigine::Math::dvec3 &U0, const Unigine::Math::dvec3 &U1,
	const Unigine::Math::dvec3 &U2)
{
	// plane of first triangle
	dvec3 E1 = V1 - V0;
	dvec3 E2 = V2 - V0;
	dvec3 N1 = _tt_cross(E1, E2);
	double d1 = -_tt_dot(N1, V0);

	// signs of distances from the vertices of the second triangle to the plane of the first
	double du0 = _tt_dot(N1, U0) + d1;
	double du1 = _tt_dot(N1, U1) + d1;
	double du2 = _tt_dot(N1, U2) + d1;

	// robust zeroing for distances close to zero
	auto zfix = [](double &v) {
		if (std::abs(v) < 1e-12)
			v = 0.0;
	};
	zfix(du0);
	zfix(du1);
	zfix(du2);

	if ((du0 > 0 && du1 > 0 && du2 > 0) || (du0 < 0 && du1 < 0 && du2 < 0))
		return false;	 // entirely on one side

	// plane of second triangle
	dvec3 F1 = U1 - U0;
	dvec3 F2 = U2 - U0;
	dvec3 N2 = _tt_cross(F1, F2);
	double d2 = -_tt_dot(N2, U0);

	double dv0 = _tt_dot(N2, V0) + d2;
	double dv1 = _tt_dot(N2, V1) + d2;
	double dv2 = _tt_dot(N2, V2) + d2;

	zfix(dv0);
	zfix(dv1);
	zfix(dv2);

	if ((dv0 > 0 && dv1 > 0 && dv2 > 0) || (dv0 < 0 && dv1 < 0 && dv2 < 0))
		return false;

	// coplanar case
	if (du0 == 0 && du1 == 0 && du2 == 0 && dv0 == 0 && dv1 == 0 && dv2 == 0)
	{
		// check intersection of projections
		return _tt_coplanar_tri_tri_3d(V0, V1, V2, U0, U1, U2, N1);
	}

	// direction of the intersection line of planes
	dvec3 D = _tt_cross(N1, N2);

	// select a component for parameterization (max. by module)
	double maxx = std::abs(D.x), maxy = std::abs(D.y), maxz = std::abs(D.z);
	int index = 0;
	if (maxy > maxx)
		index = 1;
	if ((index == 0 && maxz > maxx) || (index == 1 && maxz > maxy))
		index = 2;

	auto isect = [&](const dvec3 &P0, const dvec3 &P1, double s0, double s1, double &is0,
					 double &is1) {
		// parameter t on the edge P0->P1, where the plane is different gives s=0
		double t0 = s0 / (s0 - s1);
		dvec3 I0 = P0 + (P1 - P0) * t0;

		// another end from another triangle vertex (by a pair of characters)
		// In practice, two intervals are enough; for simplicity, we will make the second one
		// exactly the same, but call it for another pair in the calling code.
		is0 = (index == 0) ? I0.x : (index == 1) ? I0.y : I0.z;
		is1 = is0;	  // will be overwritten by the second call
	};

	// collect the intervals of projections onto the selected axis for both triangles
	double isectA0, isectA1, isectB0, isectB1;

	// for triangle U: take two points of intersection of edges (where the signs are different)
	auto compute_interval = [&](const dvec3 &P0, const dvec3 &P1, const dvec3 &P2, double s0,
								double s1, double s2, double &o0, double &o1) {
		// find two intersecting edges
		if ((s0 > 0 && s1 < 0) || (s0 < 0 && s1 > 0))
		{
			double t = s0 / (s0 - s1);
			dvec3 I = P0 + (P1 - P0) * t;
			o0 = (index == 0) ? I.x : (index == 1) ? I.y : I.z;
		}
		else if (s0 == 0)
		{
			o0 = (index == 0) ? P0.x : (index == 1) ? P0.y : P0.z;
		}
		else if (s1 == 0)
		{
			o0 = (index == 0) ? P1.x : (index == 1) ? P1.y : P1.z;
		}
		else
		{
			// s0,s1 on one side: use edge (0,2) or (1,2)
			double t = s0 / (s0 - s2);
			dvec3 I = P0 + (P2 - P0) * t;
			o0 = (index == 0) ? I.x : (index == 1) ? I.y : I.z;
		}

		// second end of the interval
		if ((s0 > 0 && s2 < 0) || (s0 < 0 && s2 > 0))
		{
			double t = s0 / (s0 - s2);
			dvec3 I = P0 + (P2 - P0) * t;
			o1 = (index == 0) ? I.x : (index == 1) ? I.y : I.z;
		}
		else if (s2 == 0)
		{
			o1 = (index == 0) ? P2.x : (index == 1) ? P2.y : P2.z;
		}
		else
		{
			double t = s1 / (s1 - s2);
			dvec3 I = P1 + (P2 - P1) * t;
			o1 = (index == 0) ? I.x : (index == 1) ? I.y : I.z;
		}

		if (o0 > o1)
			std::swap(o0, o1);
	};

	compute_interval(U0, U1, U2, du0, du1, du2, isectA0, isectA1);
	compute_interval(V0, V1, V2, dv0, dv1, dv2, isectB0, isectB1);

	// checking the intersection of intervals
	// small tolerance for touch
	double eps = 1e-12;
	if (isectA1 < isectB0 - eps)
		return false;
	if (isectB1 < isectA0 - eps)
		return false;
	return true;
}

// helper: axis-aligned bound box
struct AABB
{
	dvec3 min, max;
	bool overlaps(const AABB &o) const
	{
		return (min.x <= o.max.x && max.x >= o.min.x) && (min.y <= o.max.y && max.y >= o.min.y)
			   && (min.z <= o.max.z && max.z >= o.min.z);
	}
};

struct EdgeKey
{
	int a, b;
	bool operator==(const EdgeKey &o) const { return a == o.a && b == o.b; }
};
struct EdgeHash
{
	size_t operator()(const EdgeKey &e) const
	{
		return (size_t)e.a * 73856093u ^ (size_t)e.b * 19349663u;
	}
};
struct PosKey
{
	int x, y, z;
	bool operator==(const PosKey &o) const { return x == o.x && y == o.y && z == o.z; }
};
struct PosKeyHash
{
	size_t operator()(const PosKey &p) const
	{
		return (size_t)p.x * 73856093u ^ (size_t)p.y * 19349663u ^ (size_t)p.z * 83492791u;
	}
};
static PosKey quantize(const dvec3 &p, double eps)
{
	return {(int)floor(p.x / eps + 0.5), (int)floor(p.y / eps + 0.5), (int)floor(p.z / eps + 0.5)};
}
}	 // namespace

csg::SelfIntersectionStats csg::CheckPolysSelfIntersections(
	const Vector<CsgPolygon> &polys, double eps)
{
	struct Tri
	{
		dvec3 a, b, c;
		AABB box;
	};
	Vector<Tri> tris;
	tris.reserve(polys.size() * 2);

	// fan-triangulation
	for (auto &poly : polys)
	{
		if (poly.verts.size() < 3)
			continue;
		for (int i = 2; i < poly.verts.size(); ++i)
		{
			Tri t;
			t.a = poly.verts[0].pos;
			t.b = poly.verts[i - 1].pos;
			t.c = poly.verts[i].pos;
			t.box.min = min(min(t.a, t.b), t.c);
			t.box.max = max(max(t.a, t.b), t.c);
			tris.push_back(t);
		}
	}

	SelfIntersectionStats S{};
	S.tri_count = tris.size();

	for (int i = 0; i < tris.size(); ++i)
	{
		for (int j = i + 1; j < tris.size(); ++j)
		{
			if (!tris[i].box.overlaps(tris[j].box))
				continue;
			if (tri_tri_overlap_test(
					tris[i].a, tris[i].b, tris[i].c, tris[j].a, tris[j].b, tris[j].c))
			{
				S.intersections++;
			}
		}
	}
	return S;
}

MeshCheckStats csg::CheckPolysWatertight(const Vector<CsgPolygon> &polys, double eps)
{
	MeshCheckStats R{};
	std::unordered_map<PosKey, int, PosKeyHash> pos2id;
	int next_vid = 0;

	auto get_vid = [&](const dvec3 &p) {
		PosKey key = quantize(p, eps);
		auto it = pos2id.find(key);
		if (it != pos2id.end())
			return it->second;
		int id = next_vid++;
		pos2id[key] = id;
		return id;
	};

	std::unordered_map<EdgeKey, int, EdgeHash> edge_count;
	for (const auto &poly : polys)
	{
		if (poly.verts.size() < 3)
			continue;
		for (int i = 2; i < poly.verts.size(); ++i)
		{
			int v0 = get_vid(poly.verts[0].pos);
			int v1 = get_vid(poly.verts[i - 1].pos);
			int v2 = get_vid(poly.verts[i].pos);
			if (v0 == v1 || v1 == v2 || v2 == v0)
			{
				R.degenerate_tris++;
				continue;
			}
			R.tris++;
			auto add_edge = [&](int a, int b) {
				if (a > b)
					std::swap(a, b);
				edge_count[{a, b}]++;
			};
			add_edge(v0, v1);
			add_edge(v1, v2);
			add_edge(v2, v0);
		}
	}

	for (auto &kv : edge_count)
	{
		if (kv.second == 1)
			R.open_edges++;
		else if (kv.second > 2)
			R.nonmanifold_edges++;
	}
	return R;
}

void csg::CapOpenBoundariesOnPolys(Vector<CsgPolygon> &polys, double eps)
{
	// create map of positions and edges
	std::unordered_map<PosKey, int, PosKeyHash> pos2id;
	Vector<dvec3> id2pos;
	auto get_vid = [&](const dvec3 &p) {
		PosKey key = quantize(p, eps);
		auto it = pos2id.find(key);
		if (it != pos2id.end())
			return it->second;
		int id = id2pos.size();
		pos2id[key] = id;
		id2pos.push_back(p);
		return id;
	};

	struct Edge
	{
		int a, b;
	};
	std::unordered_map<EdgeKey, int, EdgeHash> edge_count;
	for (const auto &poly : polys)
	{
		if (poly.verts.size() < 3)
			continue;
		for (int i = 2; i < poly.verts.size(); ++i)
		{
			int v0 = get_vid(poly.verts[0].pos);
			int v1 = get_vid(poly.verts[i - 1].pos);
			int v2 = get_vid(poly.verts[i].pos);
			auto add_edge = [&](int a, int b) {
				if (a > b)
					std::swap(a, b);
				edge_count[{a, b}]++;
			};
			add_edge(v0, v1);
			add_edge(v1, v2);
			add_edge(v2, v0);
		}
	}

	// collect open_edges
	std::unordered_map<int, Vector<int>> adj;
	for (auto &kv : edge_count)
	{
		if (kv.second == 1)
		{
			adj[kv.first.a].push_back(kv.first.b);
			adj[kv.first.b].push_back(kv.first.a);
		}
	}

	// find loops
	std::unordered_set<int> visited;
	for (auto &kv : adj)
	{
		int start = kv.first;
		if (visited.count(start))
			continue;
		Vector<int> loop;
		int cur = start, prev = -1;
		while (true)
		{
			loop.push_back(cur);
			visited.insert(cur);
			int next = -1;
			for (int nb : adj[cur])
				if (nb != prev)
				{
					next = nb;
					break;
				}
			if (next < 0)
				break;
			prev = cur;
			cur = next;
			if (cur == start)
				break;
			if (visited.count(cur) && cur != start)
				break;
		}
		if (loop.size() < 3)
			continue;

		// create normal and plane
		dvec3 c;
		for (auto vid : loop)
			c += id2pos[vid];
		c /= double(loop.size());
		dvec3 n;
		for (int i = 0; i < loop.size(); ++i)
		{
			dvec3 a = id2pos[loop[i]] - c;
			dvec3 b = id2pos[loop[(i + 1) % loop.size()]] - c;
			n += cross(a, b);
		}
		if (length2(n) < 1e-16)
			continue;
		n = normalize(n);

		// create polygon
		Vector<Vertex> verts;
		for (auto vid : loop)
			verts.emplace_back(id2pos[vid], n, dvec2_zero);
		polys.emplace_back(verts, 0);
	}
}

void csg::WeldPolysByPosition(Vector<CsgPolygon> &polys, double eps)
{
	for (auto &poly : polys)
	{
		for (int i = 0; i < poly.verts.size(); ++i)
		{
			for (int j = i + 1; j < poly.verts.size(); ++j)
			{
				if (length2(poly.verts[i].pos - poly.verts[j].pos) < eps * eps)
				{
					poly.verts.remove(j);
					--j;
				}
			}
		}
		if (poly.verts.size() >= 3)
		{
			poly.plane = Plane::fromPoints(poly.verts[0].pos, poly.verts[1].pos, poly.verts[2].pos);
		}
	}
}

namespace {
static inline double qstep(double x, double s)
{
	return (s > 0.0) ? std::round(x / s) * s : x;
}
static inline Math::dvec3 qvec(const Math::dvec3 &v, double s)
{
	return Math::dvec3(qstep(v.x, s), qstep(v.y, s), qstep(v.z, s));
}
static inline Math::dvec2 qvec2(const Math::dvec2 &v, double s)
{
	return Math::dvec2(qstep(v.x, s), qstep(v.y, s));
}
static inline bool lex_negative(const Math::dvec3 &n)
{
	// fix normal orientation
	if (n.z < 0)
		return true;
	if (n.z > 0)
		return false;
	if (n.y < 0)
		return true;
	if (n.y > 0)
		return false;
	return n.x < 0;
}

// 2D cross function
static inline double cross2(const Math::dvec2 &a, const Math::dvec2 &b)
{
	return a.x * b.y - a.y * b.x;
}
static inline double dot2(const Math::dvec2 &a, const Math::dvec2 &b)
{
	return a.x * b.x + a.y * b.y;
}

// pick leftmost edge from dir -> CCW
static int pick_leftmost_next(int cur, int prev, const Unigine::Vector<Math::dvec2> &pts2,
	Unigine::Vector<Unigine::Vector<int>> &adj)
{
	const Math::dvec2 curP = pts2[cur];
	Math::dvec2 dir = curP - pts2[prev];
	if (prev == cur || (dir.x == 0.0 && dir.y == 0.0))
		dir = Math::dvec2(0.0, -1.0);	 // start
	int best = -1;
	double best_ang = -1e100;

	auto &outs = adj[cur];
	for (int k = 0; k < outs.size(); ++k)
	{
		int nxt = outs[k];
		const Math::dvec2 v = pts2[nxt] - curP;
		if (v.x == 0.0 && v.y == 0.0)
			continue;
		// atan2(cross, dot) - angle of rotation from dir to v; select maximum
		double ang = std::atan2(cross2(dir, v), dot2(dir, v));
		if (ang > best_ang)
		{
			best_ang = ang;
			best = k;
		}
	}
	return best;	// outs' index (not an id)
}

static inline void plane_basis(const Math::dvec3 &n, Math::dvec3 &u, Math::dvec3 &v)
{
	using namespace Math;

	dvec3 nn = n;
	double len = length(nn);
	if (len == 0.0)
	{
		u = dvec3(1.0, 0.0, 0.0);
		v = dvec3(0.0, 1.0, 0.0);
		return;
	}
	nn /= len;

	dvec3 a =
		(std::fabs(nn.x) <= std::fabs(nn.y) && std::fabs(nn.x) <= std::fabs(nn.z))
			? dvec3(1.0, 0.0, 0.0)
			: (std::fabs(nn.y) <= std::fabs(nn.z) ? dvec3(0.0, 1.0, 0.0) : dvec3(0.0, 0.0, 1.0));

	u = cross(nn, a);
	double lu = length(u);
	if (lu < 1e-30)
	{
		a = (a.x == 0.0) ? dvec3(1.0, 0.0, 0.0) : dvec3(0.0, 1.0, 0.0);
		u = cross(nn, a);
		lu = length(u);
		if (lu < 1e-30)
		{
			u = dvec3(1.0, 0.0, 0.0);
			v = dvec3(0.0, 1.0, 0.0);
			return;
		}
	}
	u /= lu;

	v = cross(nn, u);
}
}	 // namespace

Unigine::Vector<CsgPolygon> csg::MergeCoplanarFaces(
	const Unigine::Vector<CsgPolygon> &in, double n_snap, double w_snap, double xy_snap)
{
	using namespace Unigine;
	using namespace Math;

	struct PlaneKey
	{
		// quantitized values
		long long nx, ny, nz;
		long long wq;
		int material;
		bool operator==(const PlaneKey &o) const
		{
			return nx == o.nx && ny == o.ny && nz == o.nz && wq == o.wq && material == o.material;
		}
	};
	struct PlaneKeyHash
	{
		size_t operator()(const PlaneKey &k) const
		{
			// simple mix
			uint64_t h = 1469598103934665603ull;
			auto mix = [&](uint64_t v) {
				h ^= v;
				h *= 1099511628211ull;
			};
			mix((uint64_t)k.nx);
			mix((uint64_t)k.ny);
			mix((uint64_t)k.nz);
			mix((uint64_t)k.wq);
			mix((uint64_t)k.material);
			return (size_t)h;
		}
	};

	// 1) group by plane + material
	std::unordered_map<PlaneKey, Vector<int>, PlaneKeyHash> groups;
	groups.reserve(in.size());

	auto to_key = [&](const CsgPolygon &p) -> PlaneKey {
		dvec3 n = p.plane.n;
		double w = p.plane.w;
		// fix orientation
		if (lex_negative(n))
		{
			n = -n;
			w = -w;
		}
		// normalize (just in case)
		double len = length(n);
		if (len > 0.0)
		{
			n /= len;
			w /= len;
		}
		// quantity
		long long qnx = (long long)std::llround(n.x / n_snap);
		long long qny = (long long)std::llround(n.y / n_snap);
		long long qnz = (long long)std::llround(n.z / n_snap);
		long long qw = (long long)std::llround(w / w_snap);
		return {qnx, qny, qnz, qw, p.material};
	};

	Vector<Plane> group_plane;
	group_plane.reserve(in.size());
	std::unordered_map<size_t, int> plane_index;
	plane_index.reserve(in.size());

	for (int i = 0; i < in.size(); ++i)
	{
		PlaneKey key = to_key(in[i]);
		size_t hk = PlaneKeyHash{}(key);
		auto it = plane_index.find(hk);
		if (it == plane_index.end())
		{
			groups[key] = {};
			plane_index[hk] = (int)group_plane.size();
			group_plane.push_back(in[i].plane);
		}
		groups[key].push_back(i);
	}

	Vector<CsgPolygon> out;
	out.reserve(in.size());	   // after merge will be low

	// 2) processing every group
	for (const auto &kv : groups)
	{
		const PlaneKey &key = kv.first;
		const Vector<int> &ids = kv.second;
		if (ids.empty())
			continue;

		// restore oriented plane (normal/basis)
		// (find first)
		Plane base = in[ids[0]].plane;
		dvec3 n = base.n;
		double w = base.w;
		if (lex_negative(n))
		{
			n = -n;
			w = -w;
		}
		double len = length(n);
		if (len > 0.0)
		{
			n /= len;
			w /= len;
		}

		// plane basis (u,v)
		dvec3 u, v;
		plane_basis(n, u, v);

		// 2.1) Collect "welded" 2d vertices and set id to them
		struct VInfo
		{
			dvec3 p3;
			dvec2 uv;
			int count = 0;

			VInfo(const dvec3 &v0, const dvec2 &v1, int v2)
			{
				p3 = v0;
				uv = v1;
				count = v2;
			}
		};
		std::unordered_map<long long, int> point_id;	// key = (qx<<32) ^ qy
		Vector<dvec2> pts2;
		Vector<VInfo> pts3;

		auto key2d = [&](const dvec2 &p) -> long long {
			long long qx = (long long)std::llround(p.x / xy_snap);
			long long qy = (long long)std::llround(p.y / xy_snap);
			return (qx << 32) ^ (qy & 0xffffffffll);
		};
		auto get_vid = [&](const Vertex &vin) -> int {
			dvec2 p2(dot(vin.pos, u), dot(vin.pos, v));
			long long k2 = key2d(p2);
			auto it = point_id.find(k2);
			if (it == point_id.end())
			{
				int nid = (int)pts2.size();
				point_id.emplace(k2, nid);
				auto qstep = [&](double x) {
					return (xy_snap > 0.0) ? std::round(x / xy_snap) * xy_snap : x;
				};
				dvec2 p2q(qstep(p2.x), qstep(p2.y));
				pts2.push_back(p2q);
				pts3.push_back({vin.pos, vin.uv, 1});
				return nid;
			}
			else
			{
				int id = it->second;
				VInfo &acc = pts3[id];
				acc.p3 += vin.pos;
				acc.uv += vin.uv;
				acc.count++;
				return id;
			}
		};
		// PASS 1: add vertices
		Vector<Vector<int>> poly_vids;
		poly_vids.resize(ids.size());
		for (int ii = 0; ii < ids.size(); ++ii)
		{
			const auto &poly = in[ids[ii]];
			auto &vids = poly_vids[ii];
			vids.resize(poly.verts.size());
			for (int i = 0; i < poly.verts.size(); ++i)
				vids[i] = get_vid(poly.verts[i]);
		}

		// 2.2) add edges
		auto edge_key = [](int a, int b) -> unsigned long long {
			return ((unsigned long long)(unsigned int)a << 32) | (unsigned int)b;
		};
		std::unordered_map<unsigned long long, int> E;	  // multiset with cancellation
		auto add_edge = [&](int a, int b) {
			if (a == b)
				return;
			auto rev = edge_key(b, a);
			auto it = E.find(rev);
			if (it != E.end())
			{
				if (--(it->second) == 0)
					E.erase(it);
			}
			else
				E[edge_key(a, b)]++;
		};

		auto on_segment = [&](int a, int b, int c) -> bool {
			const dvec2 &A = pts2[a], &B = pts2[b], &C = pts2[c];
			dvec2 AB = B - A, AC = C - A;
			double AB2 = dot2(AB, AB);
			if (AB2 < xy_snap * xy_snap)
				return false;
			if (std::abs(cross2(AB, AC)) > xy_snap * std::max(1.0, std::sqrt(AB2)))
				return false;
			double t = dot2(AC, AB) / AB2;
			return (t > xy_snap) && (t < 1.0 - xy_snap);
		};

		// PASS 2: build edges, unified orientation
		for (int ii = 0; ii < ids.size(); ++ii)
		{
			const auto &poly = in[ids[ii]];
			const bool flip = dot(poly.plane.n, n) < 0;
			const auto &vids = poly_vids[ii];

			for (int i = 0; i < vids.size(); ++i)
			{
				int a = vids[i], b = vids[(i + 1) % vids.size()];
				if (a == b)
					continue;

				struct Node
				{
					double t;
					int id;
				};
				std::vector<Node> chain;
				chain.reserve(8);
				chain.push_back({0.0, a});
				chain.push_back({1.0, b});

				const dvec2 A = pts2[a], B = pts2[b];
				dvec2 AB = B - A;
				double AB2 = dot2(AB, AB);

				for (int c = 0; c < pts2.size(); ++c)
				{
					if (c == a || c == b)
						continue;
					if (!on_segment(a, b, c))
						continue;
					double t = dot2(pts2[c] - A, AB) / AB2;
					chain.push_back({t, c});
				}
				std::sort(chain.begin(), chain.end(),
					[](const Node &L, const Node &R) { return L.t < R.t; });
				chain.erase(std::unique(chain.begin(), chain.end(),
								[](const Node &L, const Node &R) { return L.id == R.id; }),
					chain.end());

				for (int k = 0; k + 1 < (int)chain.size(); ++k)
				{
					int p = chain[k].id, q = chain[k + 1].id;
					if (length2(pts2[q] - pts2[p]) < xy_snap * xy_snap)
						continue;
					if (flip)
						add_edge(q, p);
					else
						add_edge(p, q);
				}
			}
		}

		if (E.empty())
			continue;

		// finalize
		Vector<Vertex> id2vertex;
		id2vertex.resize(pts2.size());
		for (int i = 0; i < pts2.size(); ++i)
		{
			VInfo &acc = pts3[i];
			int c = max(1, acc.count);
			dvec3 p = acc.p3 / double(c);
			dvec2 uv = acc.uv / double(c);
			id2vertex[i] = Vertex{p, n, uv};
		}

		// 2.3) Collect oriented edges
		Vector<Vector<int>> adj;
		adj.resize(pts2.size());
		for (auto &e : E)
		{
			unsigned long long k = e.first;
			int a = int((k >> 32) & 0xffffffffu);
			int b = int(k & 0xffffffffu);
			for (int cnt = 0; cnt < e.second; ++cnt)
				adj[a].push_back(b);
		}

		// 2.4) Restore contour
		auto pop_edge = [&](int a, int idx_in_list) {
			auto &lst = adj[a];
			lst.remove(idx_in_list);
		};

		// helpers
		auto remove_collinear = [&](Unigine::Vector<int> &loop) {
			if (loop.size() < 3)
				return;
			Unigine::Vector<int> out_ids;
			out_ids.reserve(loop.size());
			for (int i = 0; i < loop.size(); ++i)
			{
				int ia = loop[(i + loop.size() - 1) % loop.size()];
				int ib = loop[i];
				int ic = loop[(i + 1) % loop.size()];
				dvec2 a = pts2[ia], b = pts2[ib], c = pts2[ic];
				dvec2 ab = b - a, bc = c - b;
				if (std::fabs(cross2(ab, bc))
					<= std::max(1.0, std::max(length(ab), length(bc))) * 1e-12)
					continue;	 // collinear point
				out_ids.push_back(ib);
			}
			if (out_ids.size() >= 3)
				loop = std::move(out_ids);
		};

		// origin edged
		for (int start = 0; start < adj.size(); ++start)
		{
			while (!adj[start].empty())
			{
				// start new contour
				Unigine::Vector<int> contour;
				int a = start;
				int idx = pick_leftmost_next(a, a, pts2, adj);
				if (idx < 0)
				{
					adj[a].clear();
					break;
				}
				int b = adj[a][idx];
				pop_edge(a, idx);
				contour.push_back(a);

				int prev = a, cur = b;
				while (true)
				{
					contour.push_back(cur);
					if (cur == start)
						break;
					int kidx = pick_leftmost_next(cur, prev, pts2, adj);
					if (kidx < 0)
						break;
					int nxt = adj[cur][kidx];
					pop_edge(cur, kidx);
					prev = cur;
					cur = nxt;
					if (contour.size() > 100000)
						break;
				}

				if (contour.size() >= 3 && contour.front() == contour.back())
				{
					contour.removeLast();
					remove_collinear(contour);
					if (contour.size() >= 3)
					{
						// CsgPolygon
						Unigine::Vector<Vertex> verts;
						verts.reserve(contour.size());
						for (int vi : contour)
							verts.push_back(id2vertex[vi]);

						double area2 = 0.0;
						for (int i = 0; i < contour.size(); ++i)
						{
							const dvec2 &p = pts2[contour[i]];
							const dvec2 &q = pts2[contour[(i + 1) % contour.size()]];
							area2 += cross2(p, q);
						}
						if (area2 < 0)
							std::reverse(verts.begin(), verts.end());

						out.emplace_back(verts, key.material);
					}
				}
			}
		}
	}

	if (out.empty())
		return in;

	return out;
}

namespace csg {
static inline double dot2(const dvec2 &a, const dvec2 &b)
{
	return a.x * b.x + a.y * b.y;
}
static inline double cross2(const dvec2 &a, const dvec2 &b)
{
	return a.x * b.y - a.y * b.x;
}
static inline double len2(const dvec2 &v)
{
	return dot2(v, v);
}
static inline bool lex_negative(const dvec3 &n)
{
	if (n.z < 0)
		return true;
	if (n.z > 0)
		return false;
	if (n.y < 0)
		return true;
	if (n.y > 0)
		return false;
	return n.x < 0;
}
static inline void plane_basis(const dvec3 &n_in, dvec3 &u, dvec3 &v)
{
	dvec3 n = n_in;
	double L = length(n);
	if (L == 0.0)
	{
		u = dvec3(1, 0, 0);
		v = dvec3(0, 1, 0);
		return;
	}
	n /= L;
	dvec3 a = (fabs(n.x) <= fabs(n.y) && fabs(n.x) <= fabs(n.z))
				  ? dvec3(1, 0, 0)
				  : (fabs(n.y) <= fabs(n.z) ? dvec3(0, 1, 0) : dvec3(0, 0, 1));
	u = normalize(cross(n, a));
	v = cross(n, u);
}

// ---------------- ear clipping for a simple polygon in 2D ----------------
static bool point_in_tri(const dvec2 &p, const dvec2 &a, const dvec2 &b, const dvec2 &c)
{
	// barycentric with same-side tests
	dvec2 v0 = c - a, v1 = b - a, v2 = p - a;
	double den = v0.x * v1.y - v1.x * v0.y;
	if (fabs(den) < 1e-20)
		return false;
	double u = (v2.x * v1.y - v1.x * v2.y) / den;
	double v = (v0.x * v2.y - v2.x * v0.y) / den;
	return (u >= -1e-12) && (v >= -1e-12) && (u + v <= 1.0 + 1e-12);
}

static void triangulate_ccw_polygon(const Vector<int> &loop_ids, const Vector<dvec2> &pts2,
	Vector<ivec3> &out_tris, int base_vertex_offset)
{
	// assumes CCW order
	const int n = loop_ids.size();
	if (n < 3)
		return;
	Vector<int> idx = loop_ids;

	auto is_convex = [&](int i) -> bool {
		const dvec2 &a = pts2[idx[(i + idx.size() - 1) % idx.size()]];
		const dvec2 &b = pts2[idx[i]];
		const dvec2 &c = pts2[idx[(i + 1) % idx.size()]];
		return cross2(b - a, c - b) > 0.0;
	};

	int guard = 0;
	while (idx.size() > 3 && guard++ < 10000)
	{
		bool clipped = false;
		for (int i = 0; i < idx.size(); ++i)
		{
			if (!is_convex(i))
				continue;
			int ia = idx[(i + idx.size() - 1) % idx.size()];
			int ib = idx[i];
			int ic = idx[(i + 1) % idx.size()];
			// check if any other point lies inside ear
			bool any_inside = false;
			for (int j = 0; j < idx.size(); ++j)
			{
				if (j == i || j == (i + 1) % idx.size() || j == (i + idx.size() - 1) % idx.size())
					continue;
				if (point_in_tri(pts2[idx[j]], pts2[ia], pts2[ib], pts2[ic]))
				{
					any_inside = true;
					break;
				}
			}
			if (any_inside)
				continue;
			out_tris.push_back(
				ivec3(base_vertex_offset + ia, base_vertex_offset + ib, base_vertex_offset + ic));
			idx.remove(i);
			clipped = true;
			break;
		}
		if (!clipped)
			break;
	}
	if (idx.size() == 3)
	{
		out_tris.push_back(ivec3(
			base_vertex_offset + idx[0], base_vertex_offset + idx[1], base_vertex_offset + idx[2]));
	}
}

static inline double signed_area_loop(
	const Unigine::Vector<int> &loop, const Unigine::Vector<Unigine::Math::dvec2> &pts2)
{
	using namespace Unigine::Math;
	double a = 0.0;
	for (int i = 0; i < loop.size(); ++i)
	{
		const dvec2 &A = pts2[loop[i]];
		const dvec2 &B = pts2[loop[(i + 1) % loop.size()]];
		a += A.x * B.y - A.y * B.x;
	}
	return 0.5 * a;
}

static bool point_in_poly2d(const Unigine::Math::dvec2 &P, const Unigine::Vector<int> &loop,
	const Unigine::Vector<Unigine::Math::dvec2> &pts2)
{
	using namespace Unigine::Math;
	bool inside = false;
	for (int i = 0, j = loop.size() - 1; i < loop.size(); j = i++)
	{
		const dvec2 &A = pts2[loop[i]];
		const dvec2 &B = pts2[loop[j]];
		bool inter =
			((A.y > P.y) != (B.y > P.y))
			&& (P.x < (B.x - A.x) * (P.y - A.y) / ((B.y - A.y) != 0.0 ? (B.y - A.y) : 1e-30) + A.x);
		if (inter)
			inside = !inside;
	}
	return inside;
}

// ---------------- main function ----------------

MeshStruct WeldPolysByPosition(const MeshStruct &mesh, double eps, double normal_tol)
{
	using namespace Unigine;
	using namespace Unigine::Math;

	MeshStruct out;
	if (mesh.indices.empty())
		return mesh;

	struct Candidate
	{
		int id;
		dvec3 pos;
		dvec3 normal;
	};

	auto hash_key = [](const Unigine::Math::dvec3 &p, double eps) -> long long {
		using namespace Unigine::Math;
		const double inv = 1.0 / eps;
		long long qx = (long long)llround(p.x * inv);
		long long qy = (long long)llround(p.y * inv);
		long long qz = (long long)llround(p.z * inv);
		// Morton-like hash
		return (qx * 73856093) ^ (qy * 19349663) ^ (qz * 83492791);
	};

	auto normals_close = [&](const dvec3 &a, const dvec3 &b) -> bool {
		// angle between normals
		double d = dot(normalize(a), normalize(b));
		return d > (1.0 - normal_tol);	  // almost equal
	};

	std::unordered_map<long long, std::vector<Candidate>> table;

	out.positions.reserve(mesh.positions.size());
	out.normals.reserve(mesh.normals.size());
	out.uvs.reserve(mesh.uvs.size());
	out.indices.resize(mesh.indices.size());

	for (int t = 0; t < mesh.indices.size(); ++t)
	{
		ivec3 tri{};
		for (int k = 0; k < 3; ++k)
		{
			int i = mesh.indices[t][k];
			const dvec3 &p = mesh.positions[i];
			const dvec3 &n = (i < mesh.normals.size() ? mesh.normals[i] : dvec3(0, 0, 1));
			const dvec2 &uv = (i < mesh.uvs.size() ? mesh.uvs[i] : dvec2(0, 0));

			long long h = hash_key(p, eps);
			int new_id = -1;

			auto it = table.find(h);
			if (it != table.end())
			{
				for (const Candidate &c : it->second)
				{
					if (length2(c.pos - p) <= eps * eps && normals_close(c.normal, n))
					{
						new_id = c.id;
						break;
					}
				}
			}

			if (new_id < 0)
			{
				new_id = out.positions.size();
				out.positions.push_back(p);
				out.normals.push_back(n);
				out.uvs.push_back(uv);
				table[h].push_back({new_id, p, n});
			}

			tri[k] = new_id;
		}
		out.indices[t] = tri;
	}
	return out;
}

MeshStruct MergeCoplanarFaces(const MeshStruct &mesh, double n_snap, double w_snap, double xy_snap)
{
	MeshStruct out;
	if (mesh.indices.empty())
		return mesh;

	struct Tri
	{
		int i0, i1, i2;
		dvec3 n;
		double w;
	};
	const Vector<dvec3> &P = mesh.positions;
	const Vector<dvec2> &UV = mesh.uvs;

	Vector<Tri> tris;
	tris.reserve(mesh.indices.size());
	for (const ivec3 &t : mesh.indices)
	{
		int i0 = t.x, i1 = t.y, i2 = t.z;
		const dvec3 &a = P[i0], &b = P[i1], &c = P[i2];
		dvec3 n = normalize(cross(b - a, c - a));
		if (length2(n) == 0.0)
			continue;
		double w = dot(n, a);
		tris.push_back({i0, i1, i2, n, w});
	}

	struct PlaneKey
	{
		long long nx, ny, nz, wq;
		bool operator==(const PlaneKey &o) const
		{
			return nx == o.nx && ny == o.ny && nz == o.nz && wq == o.wq;
		}
	};
	struct PlaneKeyHash
	{
		size_t operator()(const PlaneKey &k) const
		{
			uint64_t h = 1469598103934665603ull;
			auto mix = [&](uint64_t v) {
				h ^= v;
				h *= 1099511628211ull;
			};
			mix((uint64_t)k.nx);
			mix((uint64_t)k.ny);
			mix((uint64_t)k.nz);
			mix((uint64_t)k.wq);
			return (size_t)h;
		}
	};
	auto make_key = [&](dvec3 n, double w) -> PlaneKey {
		if (lex_negative(n))
		{
			n = -n;
			w = -w;
		}
		double L = length(n);
		if (L > 0.0)
		{
			n /= L;
			w /= L;
		}
		long long qnx = (long long)llround(n.x / n_snap);
		long long qny = (long long)llround(n.y / n_snap);
		long long qnz = (long long)llround(n.z / n_snap);
		long long qw = (long long)llround(w / w_snap);
		return {qnx, qny, qnz, qw};
	};

	std::unordered_map<PlaneKey, std::vector<int>, PlaneKeyHash> groups;
	groups.reserve(tris.size());
	for (int i = 0; i < tris.size(); ++i)
	{
		groups[make_key(tris[i].n, tris[i].w)].push_back(i);
	}

	auto q2 = [&](const dvec2 &p) -> dvec2 {
		if (xy_snap <= 0.0)
			return p;
		return dvec2(std::round(p.x / xy_snap) * xy_snap, std::round(p.y / xy_snap) * xy_snap);
	};

	Vector<dvec3> Rpos;
	Rpos.reserve(P.size());
	Vector<dvec3> Rnor;
	Rnor.reserve(P.size());
	Vector<dvec2> Ruv;
	Ruv.reserve(UV.size());
	Vector<ivec3> Ridx;
	Ridx.reserve(mesh.indices.size());

	for (auto &g : groups)
	{
		const std::vector<int> &ids = g.second;
		if (ids.empty())
			continue;

		auto choose_group_normal = [&](const std::vector<int> &ids) -> dvec3 {
			double bestA = 0.0;
			dvec3 bestN = dvec3_zero;
			for (int ti : ids)
			{
				const Tri &T = tris[ti];
				dvec3 ab = P[T.i1] - P[T.i0], ac = P[T.i2] - P[T.i0];
				dvec3 n = cross(ab, ac);
				double A = length(n);
				if (A > bestA)
				{
					bestA = A;
					bestN = n / std::max(A, 1e-30);
				}
			}
			if (bestA <= 0.0)
				return tris[ids.front()].n;
			return bestN;
		};
		dvec3 n = choose_group_normal(ids);
		dvec3 u, v;
		plane_basis(n, u, v);

		auto EmitGroupOriginal = [&](const std::vector<int> &ids, const dvec3 &n_flat) {
			for (int ti : ids)
			{
				const Tri &T = tris[ti];
				int b0 = (int)Rpos.size();

				Rpos.push_back(P[T.i0]);
				Ruv.push_back((T.i0 < UV.size()) ? UV[T.i0] : dvec2_zero);
				Rnor.push_back(n_flat);

				Rpos.push_back(P[T.i1]);
				Ruv.push_back((T.i1 < UV.size()) ? UV[T.i1] : dvec2_zero);
				Rnor.push_back(n_flat);

				Rpos.push_back(P[T.i2]);
				Ruv.push_back((T.i2 < UV.size()) ? UV[T.i2] : dvec2_zero);
				Rnor.push_back(n_flat);

				Ridx.push_back(ivec3(b0 + 0, b0 + 1, b0 + 2));
			}
		};

		struct VAcc
		{
			dvec3 p3;
			dvec2 uv;
			int c = 0;

			VAcc(const dvec3 &v0, const dvec2 &v1, int v2)
			{
				p3 = v0;
				uv = v1;
				c = v2;
			}
		};
		std::unordered_map<long long, int> vid;
		Vector<dvec2> pts2;
		pts2.reserve(ids.size() * 3);
		std::vector<VAcc> acc;
		acc.reserve(ids.size() * 3);

		auto key2d = [&](const dvec2 &p) -> long long {
			long long qx = (long long)llround(p.x / xy_snap);
			long long qy = (long long)llround(p.y / xy_snap);
			return (qx << 32) ^ (qy & 0xffffffffll);
		};
		auto add_vertex = [&](int i) -> int {
			const dvec3 &p3 = P[i];
			dvec2 p2 = q2(dvec2(dot(p3, u), dot(p3, v)));
			long long k = key2d(p2);
			auto it = vid.find(k);
			if (it == vid.end())
			{
				int id = (int)pts2.size();
				vid.emplace(k, id);
				pts2.push_back(p2);
				dvec2 uv = (i < UV.size() ? UV[i] : dvec2_zero);
				acc.push_back({p3, uv, 1});
				return id;
			}
			else
			{
				int id = it->second;
				acc[id].p3 += p3;
				if (i < UV.size())
					acc[id].uv += UV[i];
				acc[id].c++;
				return id;
			}
		};

		Vector<ivec3> tri_vids;
		tri_vids.resize((int)ids.size());
		for (int t = 0; t < (int)ids.size(); ++t)
		{
			const Tri &T = tris[ids[t]];
			tri_vids[t] = ivec3(add_vertex(T.i0), add_vertex(T.i1), add_vertex(T.i2));
		}

		auto ekey_undirected = [](int a, int b) -> unsigned long long {
			unsigned int A = (unsigned int)std::min(a, b);
			unsigned int B = (unsigned int)std::max(a, b);
			return ((unsigned long long)A << 32) | (unsigned long long)B;
		};
		std::unordered_map<unsigned long long, int> E;
		E.reserve(ids.size() * 6);

		auto add_edge_xor = [&](int a, int b) {
			if (a == b)
				return;
			auto k = ekey_undirected(a, b);
			E[k] ^= 1;	  // 0->1, 1->0
		};

		auto on_segment = [&](int a, int b, int c) -> bool {
			const dvec2 &A = pts2[a], &B = pts2[b], &C = pts2[c];
			dvec2 AB = B - A, AC = C - A;
			double AB2 = len2(AB);
			if (AB2 < xy_snap * xy_snap)
				return false;
			if (std::abs(cross2(AB, AC)) > xy_snap * std::max(1.0, std::sqrt(AB2)))
				return false;
			double t = dot2(AC, AB) / AB2;
			return (t > xy_snap) && (t < 1.0 - xy_snap);
		};

		auto orient2 = [&](const dvec2 &a, const dvec2 &b, const dvec2 &c) {
			return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
		};
		auto seg_intersect_strict = [&](const dvec2 &A, const dvec2 &B, const dvec2 &C,
										const dvec2 &D) -> bool {
			double eps = std::max(1e-12, xy_snap * 8.0);
			double o1 = orient2(A, B, C), o2 = orient2(A, B, D);
			double o3 = orient2(C, D, A), o4 = orient2(C, D, B);

			bool proper = ((o1 > eps && o2 < -eps) || (o1 < -eps && o2 > eps))
						  && ((o3 > eps && o4 < -eps) || (o3 < -eps && o4 > eps));
			if (proper)
				return true;

			auto overlap_1d = [&](double a0, double a1, double b0, double b1) {
				if (a0 > a1)
					std::swap(a0, a1);
				if (b0 > b1)
					std::swap(b0, b1);
				return !(a1 < b0 - eps || b1 < a0 - eps);
			};
			bool col = std::abs(o1) < eps && std::abs(o2) < eps && std::abs(o3) < eps
					   && std::abs(o4) < eps;
			if (col)
			{
				return overlap_1d(A.x, B.x, C.x, D.x) && overlap_1d(A.y, B.y, C.y, D.y);
			}
			return false;
		};

		auto segment_intersects_loop = [&](int ia, int ib, const Vector<int> &loop) -> bool {
			const dvec2 &A = pts2[ia], &B = pts2[ib];
			for (int i = 0; i < loop.size(); ++i)
			{
				int c = loop[i], d = loop[(i + 1) % loop.size()];
				if (ia == c || ia == d || ib == c || ib == d)
					continue;
				if (seg_intersect_strict(A, B, pts2[c], pts2[d]))
					return true;
			}
			return false;
		};
		auto segment_intersects_any = [&](int ia, int ib, const Vector<int> &outer,
										  const Vector<Vector<int>> &holes) -> bool {
			if (segment_intersects_loop(ia, ib, outer))
				return true;
			for (const auto &H : holes)
				if (segment_intersects_loop(ia, ib, H))
					return true;
			return false;
		};

		auto midpoint_inside_outer_outside_holes = [&](const dvec2 &M, const Vector<int> &outer,
													   const Vector<Vector<int>> &holes) -> bool {
			if (!point_in_poly2d(M, outer, pts2))
				return false;
			for (const auto &H : holes)
				if (point_in_poly2d(M, H, pts2))
					return false;
			return true;
		};

		auto pick_visible_bridge_on_outer = [&](const Vector<int> &outer,
												const Vector<Vector<int>> &holes, int H) -> int {
			const dvec2 &Hp = pts2[H];
			struct Cand
			{
				int v;
				double key;
			};
			std::vector<Cand> cand;
			cand.reserve(outer.size());

			for (int i = 0; i < outer.size(); ++i)
			{
				int a = outer[i], b = outer[(i + 1) % outer.size()];
				const dvec2 &A = pts2[a], &B = pts2[b];
				if (std::abs(A.y - B.y) < 1e-30)
					continue;
				bool straddle = ((A.y > Hp.y) != (B.y > Hp.y));
				if (!straddle)
					continue;
				double x_int = A.x + (B.x - A.x) * (Hp.y - A.y) / (B.y - A.y);
				if (x_int <= Hp.x + xy_snap)
					continue;

				int ends[2] = {a, b};
				for (int t = 0; t < 2; ++t)
				{
					int V = ends[t];
					if (V == H)
						continue;
					if (segment_intersects_any(H, V, outer, holes))
						continue;
					dvec2 M = (Hp + pts2[V]) * 0.5;
					if (!midpoint_inside_outer_outside_holes(M, outer, holes))
						continue;
					cand.push_back({V, x_int - Hp.x});
				}
			}

			if (cand.empty())
			{
				for (int V : outer)
				{
					if (V == H)
						continue;
					if (segment_intersects_any(H, V, outer, holes))
						continue;
					dvec2 M = (Hp + pts2[V]) * 0.5;
					if (!midpoint_inside_outer_outside_holes(M, outer, holes))
						continue;
					double key = length(pts2[V] - Hp);
					cand.push_back({V, key});
				}
			}

			if (cand.empty())
			{
				int best = 0;
				for (int i = 1; i < outer.size(); ++i)
					if (pts2[outer[i]].x > pts2[outer[best]].x)
						best = i;
				return outer[best];
			}

			std::sort(cand.begin(), cand.end(),
				[](const Cand &L, const Cand &R) { return L.key < R.key; });
			cand.erase(std::unique(cand.begin(), cand.end(),
						   [](const Cand &L, const Cand &R) { return L.v == R.v; }),
				cand.end());
			return cand.front().v;
		};

		auto remove_collinear_keep_repeated = [&](Vector<int> &loop) {
			if (loop.size() < 3)
				return;

			std::unordered_map<int, int> mult;
			for (int id : loop)
				++mult[id];

			{
				Vector<int> tmp;
				tmp.reserve(loop.size());
				for (int i = 0; i < loop.size(); ++i)
				{
					int a = loop[(i + loop.size() - 1) % loop.size()];
					int b = loop[i];
					if (mult[b] == 1 && length(pts2[b] - pts2[a]) <= xy_snap * 0.5)
						continue;
					tmp.push_back(b);
				}
				if (tmp.size() >= 3)
					loop.swap(tmp);
			}

			bool changed = true;
			int guard = 0;
			while (changed && loop.size() >= 3 && guard++ < 4)
			{
				changed = false;
				Vector<int> outv;
				outv.reserve(loop.size());
				for (int i = 0; i < loop.size(); ++i)
				{
					int ia = loop[(i + loop.size() - 1) % loop.size()];
					int ib = loop[i];
					int ic = loop[(i + 1) % loop.size()];
					if (mult[ib] > 1)
					{
						outv.push_back(ib);
						continue;
					}

					const dvec2 &A = pts2[ia], &B = pts2[ib], &C = pts2[ic];
					dvec2 AB = B - A, BC = C - B, AC = C - A;
					double Ls = std::max({length(AB), length(BC), 1.0});
					double area2 = std::abs(orient2(A, B, C));
					bool between = (dot2(AB, AC) > 0.0) && (dot2(BC, AC) > 0.0);
					if (area2 <= (xy_snap * Ls) && between)
					{
						changed = true;
						continue;
					}
					outv.push_back(ib);
				}
				if (outv.size() >= 3)
					loop.swap(outv);
			}
		};

		for (int t = 0; t < (int)ids.size(); ++t)
		{
			const Tri &T = tris[ids[t]];
			const ivec3 &vtx = tri_vids[t];
			const bool flip = dot(T.n, n) < 0;

			int loop[3] = {vtx.x, vtx.y, vtx.z};
			for (int e = 0; e < 3; ++e)
			{
				int a = loop[e], b = loop[(e + 1) % 3];
				if (a == b)
					continue;

				struct Node
				{
					double t;
					int id;
				};
				std::vector<Node> chain;
				chain.reserve(8);
				const dvec2 A = pts2[a], B = pts2[b];
				dvec2 AB = B - A;
				double AB2 = len2(AB);
				chain.push_back({0.0, a});
				chain.push_back({1.0, b});
				for (int c = 0; c < pts2.size(); ++c)
				{
					if (c == a || c == b)
						continue;
					if (!on_segment(a, b, c))
						continue;
					double tp = dot2(pts2[c] - A, AB) / AB2;
					chain.push_back({tp, c});
				}
				std::sort(chain.begin(), chain.end(),
					[](const Node &L, const Node &R) { return L.t < R.t; });
				chain.erase(std::unique(chain.begin(), chain.end(),
								[](const Node &L, const Node &R) { return L.id == R.id; }),
					chain.end());

				for (int k = 0; k + 1 < (int)chain.size(); ++k)
				{
					int p = chain[k].id, q = chain[k + 1].id;
					if (len2(pts2[q] - pts2[p]) < xy_snap * xy_snap)
						continue;
					add_edge_xor(p, q);
				}
			}
		}
		if (E.empty())
		{
			EmitGroupOriginal(ids, n);
			continue;
		}

		const int base = (int)Rpos.size();
		Vector<int> local2global;
		local2global.resize(pts2.size());
		Rpos.reserve(Rpos.size() + pts2.size());
		Rnor.reserve(Rnor.size() + pts2.size());
		Ruv.reserve(Ruv.size() + pts2.size());
		for (int i = 0; i < pts2.size(); ++i)
		{
			int c = std::max(1, acc[i].c);
			Rpos.push_back(acc[i].p3 / double(c));
			Ruv.push_back(acc[i].uv / double(c));
			Rnor.push_back(n);
			local2global[i] = base + i;
		}

		Vector<Vector<int>> adj;
		adj.resize(pts2.size());
		for (auto &kv : E)
			if (kv.second == 1)
			{
				unsigned int A = (unsigned int)(kv.first >> 32),
							 B = (unsigned int)(kv.first & 0xffffffffu);
				adj[(int)A].push_back((int)B);
				adj[(int)B].push_back((int)A);
			}
		bool bad = false;
		for (auto &lst : adj)
			if (!lst.empty() && lst.size() != 2)
			{
				bad = true;
				break;
			}
		if (bad)
		{
			EmitGroupOriginal(ids, n);
			continue;
		}

		auto pop_edge = [&](int a) -> int {
			auto &lst = adj[a];
			if (lst.empty())
				return -1;
			int b = lst.last();
			lst.removeLast();
			return b;
		};
		auto remove_collinear = [&](Vector<int> &loop) {
			if (loop.size() < 3)
				return;
			{
				Vector<int> tmp;
				tmp.reserve(loop.size());
				for (int i = 0; i < loop.size(); ++i)
				{
					int a = loop[(i + loop.size() - 1) % loop.size()], b = loop[i];
					if (length(pts2[b] - pts2[a]) <= xy_snap * 0.5)
						continue;
					tmp.push_back(b);
				}
				if (tmp.size() >= 3)
					loop.swap(tmp);
			}
			bool changed = true;
			int guard = 0;
			while (changed && loop.size() >= 3 && guard++ < 4)
			{
				changed = false;
				Vector<int> outv;
				outv.reserve(loop.size());
				for (int i = 0; i < loop.size(); ++i)
				{
					int ia = loop[(i + loop.size() - 1) % loop.size()];
					int ib = loop[i];
					int ic = loop[(i + 1) % loop.size()];
					const dvec2 &A = pts2[ia], &B = pts2[ib], &C = pts2[ic];
					dvec2 AB = B - A, BC = C - B, AC = C - A;
					double Ls = std::max({length(AB), length(BC), 1.0});
					double area2 = fabs(cross2(AB, BC));
					bool between = (dot2(AB, AC) > 0.0) && (dot2(BC, AC) > 0.0);
					if (area2 <= (xy_snap * Ls) && between)
					{
						changed = true;
						continue;
					}
					outv.push_back(ib);
				}
				if (outv.size() >= 3)
					loop.swap(outv);
			}
		};

		auto erase_edge_at = [&](Unigine::Vector<Unigine::Vector<int>> &adj, int a, int idx) {
			auto &lst = adj[a];
			lst.remove(idx);
		};

		std::vector<char> usedV(pts2.size(), 0);
		Vector<Vector<int>> loops;
		for (int s = 0; s < adj.size(); ++s)
		{
			if (adj[s].empty() || usedV[s])
				continue;
			Vector<int> loop;
			int prev = -1, cur = s;
			do
			{
				loop.push_back(cur);
				usedV[cur] = 1;
				int a = adj[cur][0], b = adj[cur][1];
				int nxt = (a == prev ? b : a);
				prev = cur;
				cur = nxt;
			}
			while (cur != s && loop.size() < 200000);
			if (loop.size() >= 3)
				loops.push_back(std::move(loop));
		}
		if (loops.size() != 1)
		{
			EmitGroupOriginal(ids, n);
			continue;
		}
		if (loops.empty())
		{
			EmitGroupOriginal(ids, n);
			continue;
		}

		Vector<int> ring = std::move(loops[0]);

		auto uniq_consecutive = [&](Vector<int> &loop) {
			if (loop.empty())
				return;
			Vector<int> tmp;
			tmp.reserve(loop.size());
			tmp.push_back(loop[0]);
			for (int i = 1; i < loop.size(); ++i)
				if (loop[i] != tmp.last())
					tmp.push_back(loop[i]);
			if (tmp.size() >= 2 && tmp.front() == tmp.back())
				tmp.removeLast();
			loop.swap(tmp);
		};
		uniq_consecutive(ring);

		remove_collinear(ring);

		if (ring.size() < 3)
		{
			EmitGroupOriginal(ids, n);
			continue;
		}

		double area = signed_area_loop(ring, pts2);
		if (area < 0.0)
		{
			for (int i = 0, j = (int)ring.size() - 1; i < j; ++i, --j)
				std::swap(ring[i], ring[j]);
		}

		triangulate_ccw_polygon(ring, pts2, Ridx, base);
	}

	out.positions.swap(Rpos);
	out.normals.swap(Rnor);
	out.uvs.swap(Ruv);
	out.indices.swap(Ridx);
	return out;
}
}	 // namespace csg
