#include "MeshBuilder.h"

#include <UnigineFileSystem.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Unigine;
using namespace Math;

namespace {
// simplify() helper functions
struct SimplifyPositionKey
{
	long long x = 0;
	long long y = 0;
	long long z = 0;

	bool operator==(const SimplifyPositionKey &o) const { return x == o.x && y == o.y && z == o.z; }
};

struct SimplifyPositionKeyHash
{
	size_t operator()(const SimplifyPositionKey &key) const
	{
		uint64_t h = 1469598103934665603ull;
		auto mix = [&h](uint64_t value) {
			h ^= value;
			h *= 1099511628211ull;
		};
		mix(static_cast<uint64_t>(key.x));
		mix(static_cast<uint64_t>(key.y));
		mix(static_cast<uint64_t>(key.z));
		return static_cast<size_t>(h);
	}
};

struct SimplifyEdgeKey
{
	int a = -1;
	int b = -1;

	SimplifyEdgeKey() = default;
	SimplifyEdgeKey(int v0, int v1)
	{
		if (v0 <= v1)
		{
			a = v0;
			b = v1;
		}
		else
		{
			a = v1;
			b = v0;
		}
	}

	bool operator==(const SimplifyEdgeKey &o) const { return a == o.a && b == o.b; }
};

struct SimplifyEdgeKeyHash
{
	size_t operator()(const SimplifyEdgeKey &key) const
	{
		return (static_cast<size_t>(key.a) * 73856093u) ^ (static_cast<size_t>(key.b) * 19349663u);
	}
};

struct SimplifyQuadric
{
	// Symmetric 4x4 matrix packed as:
	// [0]=xx [1]=xy [2]=xz [3]=xw [4]=yy [5]=yz [6]=yw [7]=zz [8]=zw [9]=ww
	double m[10];

	SimplifyQuadric() { clear(); }

	void clear()
	{
		for (int i = 0; i < 10; ++i)
			m[i] = 0.0;
	}

	void add(const SimplifyQuadric &o)
	{
		for (int i = 0; i < 10; ++i)
			m[i] += o.m[i];
	}

	void addPlane(const vec3 &n, double d, double weight)
	{
		const double a = static_cast<double>(n.x);
		const double b = static_cast<double>(n.y);
		const double c = static_cast<double>(n.z);
		const double w = std::max(weight, 0.0);

		m[0] += w * a * a;
		m[1] += w * a * b;
		m[2] += w * a * c;
		m[3] += w * a * d;
		m[4] += w * b * b;
		m[5] += w * b * c;
		m[6] += w * b * d;
		m[7] += w * c * c;
		m[8] += w * c * d;
		m[9] += w * d * d;
	}

	double evaluate(const vec3 &p) const
	{
		const double x = static_cast<double>(p.x);
		const double y = static_cast<double>(p.y);
		const double z = static_cast<double>(p.z);

		return m[0] * x * x + 2.0 * m[1] * x * y + 2.0 * m[2] * x * z + 2.0 * m[3] * x
			   + m[4] * y * y + 2.0 * m[5] * y * z + 2.0 * m[6] * y + m[7] * z * z + 2.0 * m[8] * z
			   + m[9];
	}
};

struct SimplifyCorner
{
	int group = -1;
	vec2 uv = vec2_zero;
	vec3 normal_seed = vec3(0.0f, 0.0f, 1.0f);
};

struct SimplifyTriangle
{
	SimplifyCorner corners[3];
};

struct SimplifyEdgeInfo
{
	int face_count = 0;
	int triangles[2] = {-1, -1};
	int opposite[2] = {-1, -1};
	bool boundary = false;
	bool sharp = false;
};

struct SimplifyEdgeCandidate
{
	int a = -1;
	int b = -1;
	int face_count = 0;
	bool boundary = false;
	bool sharp = false;
	vec3 target_position = vec3_zero;
	double cost = 0.0;
};

struct SimplifySnapshot
{
	std::vector<vec3> triangle_normals;
	std::vector<float> triangle_area2;
	std::vector<std::vector<int>> group_triangles;
	std::vector<std::vector<int>> neighbors;
	std::vector<bool> boundary_vertices;
	std::vector<bool> feature_vertices;
	std::vector<SimplifyQuadric> quadrics;
	std::unordered_map<SimplifyEdgeKey, SimplifyEdgeInfo, SimplifyEdgeKeyHash> edges;
};

struct SimplifyOutputVertexKey
{
	int group = -1;
	long long uv_x = 0;
	long long uv_y = 0;
	long long n_x = 0;
	long long n_y = 0;
	long long n_z = 0;

	bool operator==(const SimplifyOutputVertexKey &o) const
	{
		return group == o.group && uv_x == o.uv_x && uv_y == o.uv_y && n_x == o.n_x && n_y == o.n_y
			   && n_z == o.n_z;
	}
};

struct SimplifyOutputVertexKeyHash
{
	size_t operator()(const SimplifyOutputVertexKey &key) const
	{
		uint64_t h = 1469598103934665603ull;
		auto mix = [&h](uint64_t value) {
			h ^= value;
			h *= 1099511628211ull;
		};
		mix(static_cast<uint64_t>(key.group));
		mix(static_cast<uint64_t>(key.uv_x));
		mix(static_cast<uint64_t>(key.uv_y));
		mix(static_cast<uint64_t>(key.n_x));
		mix(static_cast<uint64_t>(key.n_y));
		mix(static_cast<uint64_t>(key.n_z));
		return static_cast<size_t>(h);
	}
};

struct SimplifyOutputVertex
{
	MeshBuilder::Surface::Vertex vertex;
	vec3 normal_sum = vec3_zero;
	vec3 fallback_normal = vec3(0.0f, 0.0f, 1.0f);
};

static long long simplify_quantize(double value, double epsilon)
{
	const double safe_epsilon = std::max(epsilon, 1e-20);
	return static_cast<long long>(std::llround(value / safe_epsilon));
}

static SimplifyPositionKey simplify_make_position_key(const vec3 &p, double epsilon)
{
	SimplifyPositionKey key;
	key.x = simplify_quantize(p.x, epsilon);
	key.y = simplify_quantize(p.y, epsilon);
	key.z = simplify_quantize(p.z, epsilon);
	return key;
}

static vec3 simplify_safe_normalize(const vec3 &v, const vec3 &fallback = vec3_zero)
{
	if (length2(v) <= Consts::EPS)
		return fallback;
	return normalize(v);
}

static float simplify_triangle_area2(const vec3 &p0, const vec3 &p1, const vec3 &p2)
{
	return length(cross(p1 - p0, p2 - p0));
}

static bool simplify_solve_3x3(const double a_in[3][3], const double b_in[3], double out[3])
{
	double m[3][4];
	for (int row = 0; row < 3; ++row)
	{
		for (int col = 0; col < 3; ++col)
			m[row][col] = a_in[row][col];
		m[row][3] = b_in[row];
	}

	for (int pivot = 0; pivot < 3; ++pivot)
	{
		int best_row = pivot;
		double best_abs = std::abs(m[pivot][pivot]);
		for (int row = pivot + 1; row < 3; ++row)
		{
			const double v = std::abs(m[row][pivot]);
			if (v > best_abs)
			{
				best_abs = v;
				best_row = row;
			}
		}

		if (best_abs <= 1e-12)
			return false;

		if (best_row != pivot)
		{
			for (int col = pivot; col < 4; ++col)
				std::swap(m[pivot][col], m[best_row][col]);
		}

		const double inv = 1.0 / m[pivot][pivot];
		for (int col = pivot; col < 4; ++col)
			m[pivot][col] *= inv;

		for (int row = 0; row < 3; ++row)
		{
			if (row == pivot)
				continue;

			const double factor = m[row][pivot];
			if (std::abs(factor) <= 1e-20)
				continue;

			for (int col = pivot; col < 4; ++col)
				m[row][col] -= factor * m[pivot][col];
		}
	}

	out[0] = m[0][3];
	out[1] = m[1][3];
	out[2] = m[2][3];
	return true;
}

static vec3 simplify_pick_feature_position(
	const SimplifyQuadric &quadric, const vec3 &p0, const vec3 &p1)
{
	const vec3 midpoint = (p0 + p1) * 0.5f;
	const double error0 = quadric.evaluate(p0);
	const double error1 = quadric.evaluate(p1);
	const double error_mid = quadric.evaluate(midpoint);

	if (error_mid <= error0 && error_mid <= error1)
		return midpoint;
	return (error0 <= error1) ? p0 : p1;
}

static vec3 simplify_compute_collapse_position(const SimplifyQuadric &q0, const SimplifyQuadric &q1,
	const vec3 &p0, const vec3 &p1, bool constrain_to_feature)
{
	SimplifyQuadric merged = q0;
	merged.add(q1);

	vec3 best_position = simplify_pick_feature_position(merged, p0, p1);
	double best_error = merged.evaluate(best_position);

	if (constrain_to_feature)
		return best_position;

	double a[3][3] = {{merged.m[0], merged.m[1], merged.m[2]},
		{merged.m[1], merged.m[4], merged.m[5]}, {merged.m[2], merged.m[5], merged.m[7]}};
	double b[3] = {-merged.m[3], -merged.m[6], -merged.m[8]};
	double solved[3] = {0.0, 0.0, 0.0};

	if (simplify_solve_3x3(a, b, solved))
	{
		const vec3 candidate(static_cast<float>(solved[0]), static_cast<float>(solved[1]),
			static_cast<float>(solved[2]));
		const double candidate_error = merged.evaluate(candidate);
		if (std::isfinite(candidate_error) && candidate_error < best_error)
			best_position = candidate;
	}

	return best_position;
}

static void simplify_add_edge(
	std::unordered_map<SimplifyEdgeKey, SimplifyEdgeInfo, SimplifyEdgeKeyHash> &edges, int v0,
	int v1, int triangle_index, int opposite_vertex)
{
	SimplifyEdgeKey key(v0, v1);
	SimplifyEdgeInfo &info = edges[key];
	if (info.face_count < 2)
	{
		info.triangles[info.face_count] = triangle_index;
		info.opposite[info.face_count] = opposite_vertex;
	}
	info.face_count++;
}

static void simplify_build_input(const MeshBuilder::Surface &surface, double weld_epsilon,
	double area_epsilon, std::vector<vec3> &group_positions,
	std::vector<SimplifyTriangle> &triangles)
{
	group_positions.clear();
	triangles.clear();

	if (surface.vertices.empty() || surface.indices.empty())
		return;

	std::unordered_map<SimplifyPositionKey, std::vector<int>, SimplifyPositionKeyHash> buckets;
	buckets.reserve(static_cast<size_t>(surface.vertices.size()) * 2);

	std::vector<int> raw_to_group(surface.vertices.size(), -1);
	group_positions.reserve(surface.vertices.size());

	const float weld_epsilon_sq = static_cast<float>(weld_epsilon * weld_epsilon);

	for (int i = 0; i < surface.vertices.size(); ++i)
	{
		const vec3 &position = surface.vertices[i].position;
		const SimplifyPositionKey key = simplify_make_position_key(position, weld_epsilon);

		int group_index = -1;
		auto bucket_it = buckets.find(key);
		if (bucket_it != buckets.end())
		{
			for (size_t j = 0; j < bucket_it->second.size(); ++j)
			{
				const int candidate = bucket_it->second[j];
				if (length2(group_positions[candidate] - position) <= weld_epsilon_sq)
				{
					group_index = candidate;
					break;
				}
			}
		}

		if (group_index < 0)
		{
			group_index = static_cast<int>(group_positions.size());
			group_positions.push_back(position);
			buckets[key].push_back(group_index);
		}

		raw_to_group[i] = group_index;
	}

	triangles.reserve(surface.indices.size() / 3);
	for (int i = 0; i + 2 < surface.indices.size(); i += 3)
	{
		const int i0 = surface.indices[i + 0];
		const int i1 = surface.indices[i + 1];
		const int i2 = surface.indices[i + 2];
		if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= surface.vertices.size()
			|| i1 >= surface.vertices.size() || i2 >= surface.vertices.size())
		{
			continue;
		}

		SimplifyTriangle triangle;
		triangle.corners[0].group = raw_to_group[i0];
		triangle.corners[1].group = raw_to_group[i1];
		triangle.corners[2].group = raw_to_group[i2];
		triangle.corners[0].uv = surface.vertices[i0].uv;
		triangle.corners[1].uv = surface.vertices[i1].uv;
		triangle.corners[2].uv = surface.vertices[i2].uv;
		triangle.corners[0].normal_seed = surface.vertices[i0].normal;
		triangle.corners[1].normal_seed = surface.vertices[i1].normal;
		triangle.corners[2].normal_seed = surface.vertices[i2].normal;

		if (triangle.corners[0].group == triangle.corners[1].group
			|| triangle.corners[1].group == triangle.corners[2].group
			|| triangle.corners[2].group == triangle.corners[0].group)
		{
			continue;
		}

		const vec3 &p0 = group_positions[triangle.corners[0].group];
		const vec3 &p1 = group_positions[triangle.corners[1].group];
		const vec3 &p2 = group_positions[triangle.corners[2].group];
		if (simplify_triangle_area2(p0, p1, p2) <= area_epsilon)
			continue;

		triangles.push_back(triangle);
	}
}

static void simplify_build_snapshot(const std::vector<vec3> &group_positions,
	const std::vector<SimplifyTriangle> &triangles, float sharp_cosine, double area_epsilon,
	double feature_weight, SimplifySnapshot &snapshot)
{
	const int groups_count = static_cast<int>(group_positions.size());

	snapshot.triangle_normals.assign(triangles.size(), vec3_zero);
	snapshot.triangle_area2.assign(triangles.size(), 0.0f);
	snapshot.group_triangles.assign(groups_count, std::vector<int>());
	snapshot.neighbors.assign(groups_count, std::vector<int>());
	snapshot.boundary_vertices.assign(groups_count, false);
	snapshot.feature_vertices.assign(groups_count, false);
	snapshot.quadrics.assign(groups_count, SimplifyQuadric());
	snapshot.edges.clear();
	snapshot.edges.reserve(triangles.size() * 3);

	for (int triangle_index = 0; triangle_index < static_cast<int>(triangles.size());
		++triangle_index)
	{
		const SimplifyTriangle &triangle = triangles[triangle_index];
		const int g0 = triangle.corners[0].group;
		const int g1 = triangle.corners[1].group;
		const int g2 = triangle.corners[2].group;

		if (g0 < 0 || g1 < 0 || g2 < 0 || g0 >= groups_count || g1 >= groups_count
			|| g2 >= groups_count)
			continue;

		const vec3 &p0 = group_positions[g0];
		const vec3 &p1 = group_positions[g1];
		const vec3 &p2 = group_positions[g2];
		const vec3 face = cross(p1 - p0, p2 - p0);
		const float area2 = length(face);
		if (area2 <= area_epsilon)
			continue;

		const vec3 normal = face / area2;
		snapshot.triangle_normals[triangle_index] = normal;
		snapshot.triangle_area2[triangle_index] = area2;
		snapshot.group_triangles[g0].push_back(triangle_index);
		snapshot.group_triangles[g1].push_back(triangle_index);
		snapshot.group_triangles[g2].push_back(triangle_index);

		const double plane_d = -dot(normal, p0);
		const double weight = std::max(static_cast<double>(area2) * 0.5, 1e-12);
		snapshot.quadrics[g0].addPlane(normal, plane_d, weight);
		snapshot.quadrics[g1].addPlane(normal, plane_d, weight);
		snapshot.quadrics[g2].addPlane(normal, plane_d, weight);

		simplify_add_edge(snapshot.edges, g0, g1, triangle_index, g2);
		simplify_add_edge(snapshot.edges, g1, g2, triangle_index, g0);
		simplify_add_edge(snapshot.edges, g2, g0, triangle_index, g1);
	}

	for (auto it = snapshot.edges.begin(); it != snapshot.edges.end(); ++it)
	{
		const SimplifyEdgeKey &key = it->first;
		SimplifyEdgeInfo &info = it->second;

		snapshot.neighbors[key.a].push_back(key.b);
		snapshot.neighbors[key.b].push_back(key.a);

		if (info.face_count == 1)
		{
			info.boundary = true;
			snapshot.boundary_vertices[key.a] = true;
			snapshot.boundary_vertices[key.b] = true;
			snapshot.feature_vertices[key.a] = true;
			snapshot.feature_vertices[key.b] = true;
		}
		else if (info.face_count == 2)
		{
			const vec3 &n0 = snapshot.triangle_normals[info.triangles[0]];
			const vec3 &n1 = snapshot.triangle_normals[info.triangles[1]];
			if (length2(n0) > Consts::EPS && length2(n1) > Consts::EPS
				&& dot(n0, n1) < sharp_cosine)
			{
				info.sharp = true;
				snapshot.feature_vertices[key.a] = true;
				snapshot.feature_vertices[key.b] = true;
			}
		}
	}

	for (int i = 0; i < groups_count; ++i)
	{
		std::vector<int> &neighbors = snapshot.neighbors[i];
		std::sort(neighbors.begin(), neighbors.end());
		neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
	}

	for (auto it = snapshot.edges.begin(); it != snapshot.edges.end(); ++it)
	{
		const SimplifyEdgeKey &key = it->first;
		const SimplifyEdgeInfo &info = it->second;
		if (!info.boundary && !info.sharp)
			continue;

		vec3 edge_dir = group_positions[key.b] - group_positions[key.a];
		const float edge_len = length(edge_dir);
		if (edge_len <= area_epsilon)
			continue;
		edge_dir /= edge_len;

		const int planes_count = info.boundary ? 1 : 2;
		for (int plane_index = 0; plane_index < planes_count; ++plane_index)
		{
			const int triangle_index = info.triangles[plane_index];
			if (triangle_index < 0)
				continue;

			const vec3 &triangle_normal = snapshot.triangle_normals[triangle_index];
			if (length2(triangle_normal) <= Consts::EPS)
				continue;

			const vec3 constraint_normal =
				simplify_safe_normalize(cross(edge_dir, triangle_normal), vec3_zero);
			if (length2(constraint_normal) <= Consts::EPS)
				continue;

			const double plane_d = -dot(constraint_normal, group_positions[key.a]);
			const double weight = feature_weight * std::max(static_cast<double>(edge_len), 1e-8);
			snapshot.quadrics[key.a].addPlane(constraint_normal, plane_d, weight);
			snapshot.quadrics[key.b].addPlane(constraint_normal, plane_d, weight);
		}
	}
}

static bool simplify_passes_link_condition(
	const SimplifySnapshot &snapshot, const SimplifyEdgeCandidate &candidate)
{
	const SimplifyEdgeKey key(candidate.a, candidate.b);
	auto it = snapshot.edges.find(key);
	if (it == snapshot.edges.end())
		return false;

	const SimplifyEdgeInfo &info = it->second;
	if (info.face_count < 1 || info.face_count > 2)
		return false;

	std::vector<int> expected;
	expected.reserve(2);
	for (int i = 0; i < std::min(info.face_count, 2); ++i)
	{
		if (info.opposite[i] >= 0)
			expected.push_back(info.opposite[i]);
	}
	std::sort(expected.begin(), expected.end());
	expected.erase(std::unique(expected.begin(), expected.end()), expected.end());

	const std::vector<int> &neighbors_a = snapshot.neighbors[candidate.a];
	const std::vector<int> &neighbors_b = snapshot.neighbors[candidate.b];

	std::vector<int> intersection;
	intersection.reserve(std::min(neighbors_a.size(), neighbors_b.size()));
	size_t ia = 0;
	size_t ib = 0;
	while (ia < neighbors_a.size() && ib < neighbors_b.size())
	{
		if (neighbors_a[ia] == neighbors_b[ib])
		{
			intersection.push_back(neighbors_a[ia]);
			++ia;
			++ib;
		}
		else if (neighbors_a[ia] < neighbors_b[ib])
		{
			++ia;
		}
		else
		{
			++ib;
		}
	}

	if (intersection.size() != expected.size())
		return false;

	for (size_t i = 0; i < expected.size(); ++i)
	{
		if (intersection[i] != expected[i])
			return false;
	}

	return true;
}

static void simplify_collect_incident_triangles(
	const SimplifySnapshot &snapshot, int group_a, int group_b, std::vector<int> &out)
{
	out.clear();
	out.reserve(
		snapshot.group_triangles[group_a].size() + snapshot.group_triangles[group_b].size());

	const std::vector<int> &tris_a = snapshot.group_triangles[group_a];
	const std::vector<int> &tris_b = snapshot.group_triangles[group_b];

	for (size_t i = 0; i < tris_a.size(); ++i)
		out.push_back(tris_a[i]);

	for (size_t i = 0; i < tris_b.size(); ++i)
	{
		const int triangle_index = tris_b[i];
		if (std::find(out.begin(), out.end(), triangle_index) == out.end())
			out.push_back(triangle_index);
	}
}

static bool simplify_is_valid_collapse(const SimplifyEdgeCandidate &candidate,
	const std::vector<SimplifyTriangle> &triangles, const std::vector<vec3> &group_positions,
	const SimplifySnapshot &snapshot, double area_epsilon)
{
	if (candidate.a < 0 || candidate.b < 0
		|| candidate.a >= static_cast<int>(group_positions.size())
		|| candidate.b >= static_cast<int>(group_positions.size()))
	{
		return false;
	}

	const bool boundary_a = snapshot.boundary_vertices[candidate.a];
	const bool boundary_b = snapshot.boundary_vertices[candidate.b];
	if (boundary_a != boundary_b)
		return false;
	if ((boundary_a || boundary_b) && !candidate.boundary)
		return false;

	const bool feature_a = snapshot.feature_vertices[candidate.a];
	const bool feature_b = snapshot.feature_vertices[candidate.b];
	if (!candidate.boundary && !candidate.sharp && (feature_a || feature_b))
		return false;

	if (!simplify_passes_link_condition(snapshot, candidate))
		return false;

	std::vector<int> incident_triangles;
	simplify_collect_incident_triangles(snapshot, candidate.a, candidate.b, incident_triangles);

	for (size_t i = 0; i < incident_triangles.size(); ++i)
	{
		const int triangle_index = incident_triangles[i];
		const SimplifyTriangle &triangle = triangles[triangle_index];

		int collapsed_corners = 0;
		vec3 positions[3];
		for (int corner = 0; corner < 3; ++corner)
		{
			const int group = triangle.corners[corner].group;
			if (group == candidate.a || group == candidate.b)
			{
				positions[corner] = candidate.target_position;
				collapsed_corners++;
			}
			else
			{
				positions[corner] = group_positions[group];
			}
		}

		// Triangles adjacent to the collapsed edge disappear.
		if (collapsed_corners >= 2)
			continue;

		const vec3 face = cross(positions[1] - positions[0], positions[2] - positions[0]);
		const float new_area2 = length(face);
		if (new_area2 <= area_epsilon)
			return false;

		const vec3 new_normal = face / new_area2;
		const vec3 &old_normal = snapshot.triangle_normals[triangle_index];
		if (length2(old_normal) > Consts::EPS && dot(new_normal, old_normal) <= 0.05f)
			return false;
	}

	return true;
}

static void simplify_build_candidates(const std::vector<vec3> &group_positions,
	const SimplifySnapshot &snapshot, std::vector<SimplifyEdgeCandidate> &candidates)
{
	candidates.clear();
	candidates.reserve(snapshot.edges.size());

	for (auto it = snapshot.edges.begin(); it != snapshot.edges.end(); ++it)
	{
		const SimplifyEdgeKey &key = it->first;
		const SimplifyEdgeInfo &info = it->second;
		if (info.face_count < 1 || info.face_count > 2)
			continue;

		SimplifyEdgeCandidate candidate;
		candidate.a = key.a;
		candidate.b = key.b;
		candidate.face_count = info.face_count;
		candidate.boundary = info.boundary;
		candidate.sharp = info.sharp;
		candidate.target_position = simplify_compute_collapse_position(snapshot.quadrics[key.a],
			snapshot.quadrics[key.b], group_positions[key.a], group_positions[key.b],
			candidate.boundary || candidate.sharp);

		SimplifyQuadric merged = snapshot.quadrics[key.a];
		merged.add(snapshot.quadrics[key.b]);
		candidate.cost = merged.evaluate(candidate.target_position);
		if (!std::isfinite(candidate.cost))
			continue;

		candidates.push_back(candidate);
	}

	std::sort(candidates.begin(), candidates.end(),
		[](const SimplifyEdgeCandidate &lhs, const SimplifyEdgeCandidate &rhs) {
			if (lhs.cost != rhs.cost)
				return lhs.cost < rhs.cost;
			if (lhs.a != rhs.a)
				return lhs.a < rhs.a;
			return lhs.b < rhs.b;
		});
}

static void simplify_apply_collapses(std::vector<vec3> &group_positions,
	std::vector<SimplifyTriangle> &triangles, const std::vector<SimplifyEdgeCandidate> &selected,
	double area_epsilon)
{
	if (selected.empty())
		return;

	const int groups_count = static_cast<int>(group_positions.size());
	std::vector<int> remap(groups_count, -1);
	std::vector<vec3> next_positions = group_positions;

	for (int i = 0; i < groups_count; ++i)
		remap[i] = i;

	for (size_t i = 0; i < selected.size(); ++i)
	{
		const SimplifyEdgeCandidate &candidate = selected[i];
		remap[candidate.b] = candidate.a;
		next_positions[candidate.a] = candidate.target_position;
	}

	std::vector<SimplifyTriangle> updated_triangles;
	updated_triangles.reserve(triangles.size());

	for (size_t triangle_index = 0; triangle_index < triangles.size(); ++triangle_index)
	{
		SimplifyTriangle triangle = triangles[triangle_index];
		for (int corner = 0; corner < 3; ++corner)
		{
			const int group = triangle.corners[corner].group;
			triangle.corners[corner].group = remap[group];
		}

		const int g0 = triangle.corners[0].group;
		const int g1 = triangle.corners[1].group;
		const int g2 = triangle.corners[2].group;
		if (g0 == g1 || g1 == g2 || g2 == g0)
			continue;

		const vec3 &p0 = next_positions[g0];
		const vec3 &p1 = next_positions[g1];
		const vec3 &p2 = next_positions[g2];
		if (simplify_triangle_area2(p0, p1, p2) <= area_epsilon)
			continue;

		updated_triangles.push_back(triangle);
	}

	std::vector<int> compact(groups_count, -1);
	std::vector<vec3> compact_positions;
	compact_positions.reserve(group_positions.size());

	for (size_t triangle_index = 0; triangle_index < updated_triangles.size(); ++triangle_index)
	{
		SimplifyTriangle &triangle = updated_triangles[triangle_index];
		for (int corner = 0; corner < 3; ++corner)
		{
			const int old_group = triangle.corners[corner].group;
			if (compact[old_group] < 0)
			{
				compact[old_group] = static_cast<int>(compact_positions.size());
				compact_positions.push_back(next_positions[old_group]);
			}
			triangle.corners[corner].group = compact[old_group];
		}
	}

	group_positions.swap(compact_positions);
	triangles.swap(updated_triangles);
}

static void simplify_rebuild_surface(MeshBuilder::Surface &surface,
	const std::vector<vec3> &group_positions, const std::vector<SimplifyTriangle> &triangles,
	double uv_epsilon, double normal_epsilon)
{
	surface.vertices.clear();
	surface.indices.clear();

	if (group_positions.empty() || triangles.empty())
		return;

	std::unordered_map<SimplifyOutputVertexKey, int, SimplifyOutputVertexKeyHash> output_map;
	output_map.reserve(triangles.size() * 3);

	std::vector<SimplifyOutputVertex> output_vertices;
	output_vertices.reserve(triangles.size() * 2);
	surface.indices.reserve(static_cast<int>(triangles.size() * 3));

	for (size_t triangle_index = 0; triangle_index < triangles.size(); ++triangle_index)
	{
		const SimplifyTriangle &triangle = triangles[triangle_index];
		int output_indices[3] = {-1, -1, -1};

		for (int corner = 0; corner < 3; ++corner)
		{
			const SimplifyCorner &src = triangle.corners[corner];
			vec3 normal_seed = simplify_safe_normalize(src.normal_seed, vec3(0.0f, 0.0f, 1.0f));

			SimplifyOutputVertexKey key;
			key.group = src.group;
			key.uv_x = simplify_quantize(src.uv.x, uv_epsilon);
			key.uv_y = simplify_quantize(src.uv.y, uv_epsilon);
			key.n_x = simplify_quantize(normal_seed.x, normal_epsilon);
			key.n_y = simplify_quantize(normal_seed.y, normal_epsilon);
			key.n_z = simplify_quantize(normal_seed.z, normal_epsilon);

			auto it = output_map.find(key);
			if (it == output_map.end())
			{
				SimplifyOutputVertex out;
				out.vertex.position = group_positions[src.group];
				out.vertex.uv = src.uv;
				out.vertex.normal = normal_seed;
				out.fallback_normal = normal_seed;

				const int new_index = static_cast<int>(output_vertices.size());
				output_vertices.push_back(out);
				output_map[key] = new_index;
				output_indices[corner] = new_index;
			}
			else
			{
				output_indices[corner] = it->second;
			}
		}

		surface.indices.append(output_indices[0]);
		surface.indices.append(output_indices[1]);
		surface.indices.append(output_indices[2]);

		const vec3 &p0 = group_positions[triangle.corners[0].group];
		const vec3 &p1 = group_positions[triangle.corners[1].group];
		const vec3 &p2 = group_positions[triangle.corners[2].group];
		const vec3 face = cross(p1 - p0, p2 - p0);
		if (length2(face) > Consts::EPS)
		{
			output_vertices[output_indices[0]].normal_sum += face;
			output_vertices[output_indices[1]].normal_sum += face;
			output_vertices[output_indices[2]].normal_sum += face;
		}
	}

	surface.vertices.reserve(static_cast<int>(output_vertices.size()));
	for (size_t i = 0; i < output_vertices.size(); ++i)
	{
		SimplifyOutputVertex &out = output_vertices[i];
		out.vertex.normal = simplify_safe_normalize(out.normal_sum, out.fallback_normal);
		surface.vertices.append(out.vertex);
	}
}

}	 // namespace

MeshBuilder::MeshBuilder()
{
	// make one default surface to support
	// instant using addTriangle()/addQuad() function
	surfaces.append();
}

void MeshBuilder::clear()
{
	surfaces.clear();
}

void MeshBuilder::load(const char *file_name_mesh)
{
	MeshPtr mesh = Mesh::create();
	if (mesh->load(file_name_mesh))
		load(mesh, true);
}

void MeshBuilder::save(const char *file_name_mesh) const
{
	MeshPtr mesh = getMesh();
	mesh->save(file_name_mesh);
}

void MeshBuilder::load(const Unigine::MeshPtr &mesh, bool remap_cvertices)
{
	clear();
	append(mesh, remap_cvertices);
}

void MeshBuilder::load(const Unigine::ObjectMeshStaticPtr &object, bool remap_cvertices)
{
	clear();
	append(object, remap_cvertices);
}

void MeshBuilder::append(const Unigine::MeshPtr &mesh, bool remap_cvertices)
{
	for (int surface = 0; surface < mesh->getNumSurfaces(); surface++)
	{
		Surface &s = surfaces.append();
		s.name = mesh->getSurfaceName(surface);

		if (remap_cvertices)
			mesh->remapCVertex(surface);

		for (int i = 0; i < mesh->getNumVertex(surface); i++)
		{
			Surface::Vertex &v = s.vertices.append();
			v.position = mesh->getVertex(i, surface);
			v.normal = mesh->getTangent(i, surface).getNormal();
			v.uv = mesh->getTexCoord0(i, surface);
		}

		for (int i = 0; i < mesh->getNumIndices(surface); i++)
			s.indices.append(mesh->getIndex(i, surface));
	}
}

void MeshBuilder::append(const Unigine::ObjectMeshStaticPtr &object, bool remap_cvertices)
{
	// load mesh to RAM
	object->loadForceRAM();

	// get mesh from ObjectMeshStatic
	MeshPtr mesh = Mesh::create();
	object->getCopyMeshRAM(mesh);

	// append to MeshBuilder
	int surfaces_count = surfaces.size();
	append(mesh, remap_cvertices);

	// append object's parameters
	for (int i = 0; i < object->getNumSurfaces(); i++)
	{
		auto &s = surfaces[surfaces_count + i];
		s.enabled = object->isEnabled(i);
		s.viewport_mask = object->getViewportMask(i);
		s.shadow_mask = object->getShadowMask(i);
		s.lighting_mode = object->getLightingMode(i);
		s.cast_proj_and_omni_shadows = object->getCastShadow(i);
		s.cast_world_shadows = object->getCastWorldShadow(i);
		s.cast_env_probe_shadows = object->getCastEnvProbeShadow(i);
		s.visibility = vec2(object->getMinVisibleDistance(i), object->getMaxVisibleDistance(i));
		s.fade = vec2(object->getMinFadeDistance(i), object->getMaxFadeDistance(i));
		s.parent = ivec2(object->getMinParent(i), object->getMaxParent(i));
		s.intersection = object->getIntersection(i);
		s.intersection_mask = object->getIntersectionMask(i);
		s.collision = object->getCollision(i);
		s.collision_mask = object->getCollisionMask(i);
		s.physics_intersection = object->getPhysicsIntersection(i);
		s.physics_intersection_mask = object->getPhysicsIntersectionMask(i);
		s.sound_occlusion = object->getSoundOcclusion(i);
		s.sound_occlusion_mask = object->getSoundOcclusionMask(i);
		s.physics_friction = object->getPhysicsFriction(i);
		s.physics_restitution = object->getPhysicsRestitution(i);
		s.material = object->getMaterial(i);
	}
}

void MeshBuilder::save(const Unigine::MeshPtr &mesh, bool optimize) const
{
	mesh->clear();

	for (int s = 0; s < surfaces.size(); s++)
	{
		auto &surface = surfaces[s];
		int id = mesh->addSurface(surface.name);
		for (int i = 0; i < surface.vertices.size(); i++)
		{
			const Surface::Vertex &v = surface.vertices[i];
			mesh->addVertex(v.position, id);
			mesh->addNormal(v.normal, id);
			mesh->addTexCoord0(v.uv, id);
		}

		for (int i = 0; i < surface.indices.size(); i++)
			mesh->addIndex(surface.indices[i], id);
	}

	if (optimize)
		mesh->optimizeIndices(Mesh::VERTEX_CACHE | Mesh::BACK_TO_FRONT);
	mesh->createTangents();
	mesh->createCollisionData();
}

void MeshBuilder::save(
	const char *file_name_mesh, const Unigine::ObjectMeshStaticPtr &object, bool optimize) const
{
	// save mesh to file
	MeshPtr mesh = getMesh(optimize);
	mesh->save(file_name_mesh);
	UGUID mesh_guid = FileSystem::addVirtualFile(file_name_mesh);

	// fill ObjectMeshStatic
	object->setMeshProceduralMode(ObjectMeshStatic::PROCEDURAL_MODE_DISABLE);
	object->setMeshPath(FileSystem::guidToPath(mesh_guid));
	object->loadForceRAM();
	for (int i = 0; i < surfaces.size(); i++)
	{
		auto &s = surfaces[i];
		object->setEnabled(s.enabled, i);
		object->setViewportMask(s.viewport_mask, i);
		object->setShadowMask(s.shadow_mask, i);
		object->setLightingMode(s.lighting_mode, i);
		object->setCastShadow(s.cast_proj_and_omni_shadows, i);
		object->setCastWorldShadow(s.cast_world_shadows, i);
		object->setCastEnvProbeShadow(s.cast_env_probe_shadows, i);
		object->setMinVisibleDistance(s.visibility.x, i);
		object->setMaxVisibleDistance(s.visibility.y, i);
		object->setMinFadeDistance(s.fade.x, i);
		object->setMaxFadeDistance(s.fade.y, i);
		object->setMinParent(s.parent.x, i);
		object->setMaxParent(s.parent.y, i);
		object->setIntersection(s.intersection, i);
		object->setIntersectionMask(s.intersection_mask, i);
		object->setCollision(s.collision, i);
		object->setCollisionMask(s.collision_mask, i);
		object->setPhysicsIntersection(s.physics_intersection, i);
		object->setPhysicsIntersectionMask(s.physics_intersection_mask, i);
		object->setSoundOcclusion(s.sound_occlusion, i);
		object->setSoundOcclusionMask(s.sound_occlusion_mask, i);
		object->setPhysicsFriction(s.physics_friction, i);
		object->setPhysicsRestitution(s.physics_restitution, i);
		object->setMaterial(s.material, i);
	}
}

Unigine::MeshPtr MeshBuilder::getMesh(bool optimize) const
{
	MeshPtr mesh = Mesh::create();
	save(mesh, optimize);
	return mesh;
}

void MeshBuilder::addTriangle(const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec2 &uv0,
	const vec2 &uv1, const vec2 &uv2, int surface)
{
	vec3 n = calcNormal(p0, p1, p2);

	auto &s = surfaces[surface];
	int index = s.vertices.size();

	Surface::Vertex &v0 = s.vertices.append();
	v0.position = p0;
	v0.normal = n;
	v0.uv = uv0;

	Surface::Vertex &v1 = s.vertices.append();
	v1.position = p1;
	v1.normal = n;
	v1.uv = uv1;

	Surface::Vertex &v2 = s.vertices.append();
	v2.position = p2;
	v2.normal = n;
	v2.uv = uv2;

	s.indices.append(index);
	s.indices.append(index + 1);
	s.indices.append(index + 2);
}

void MeshBuilder::addQuad(const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec3 &p3,
	const vec2 &uv0, const vec2 &uv1, const vec2 &uv2, const vec2 &uv3, int surface)
{
	vec3 n = calcNormal(p0, p1, p2);

	auto &s = surfaces[surface];
	int index = s.vertices.size();

	Surface::Vertex &v0 = s.vertices.append();
	v0.position = p0;
	v0.normal = n;
	v0.uv = uv0;

	Surface::Vertex &v1 = s.vertices.append();
	v1.position = p1;
	v1.normal = n;
	v1.uv = uv1;

	Surface::Vertex &v2 = s.vertices.append();
	v2.position = p2;
	v2.normal = n;
	v2.uv = uv2;

	Surface::Vertex &v3 = s.vertices.append();
	v3.position = p3;
	v3.normal = n;
	v3.uv = uv3;

	s.indices.append(index + 0);
	s.indices.append(index + 1);
	s.indices.append(index + 2);
	s.indices.append(index + 0);
	s.indices.append(index + 2);
	s.indices.append(index + 3);
}

void MeshBuilder::removeVertex(int vertex_index, int surface)
{
	auto &s = surfaces[surface];

	// remove all faces connected to this vertex
	for (int i = s.indices.size() - 3; i >= 0; i -= 3)
	{
		if (s.indices[i + 0] == vertex_index || s.indices[i + 1] == vertex_index
			|| s.indices[i + 2] == vertex_index)
		{
			s.indices.remove(i, 3);
		}
	}

	// remove vertex
	s.vertices.remove(vertex_index);

	// fix indices array
	for (int j = 0; j < s.indices.size(); j++)
		if (s.indices[j] > vertex_index)
			s.indices[j]--;
}

void MeshBuilder::removeVertices(const Vector<int> &vertex_indices, int surface)
{
	auto &s = surfaces[surface];
	if (vertex_indices.empty() || s.vertices.empty())
		return;

	std::vector<bool> remove_vertex(static_cast<size_t>(s.vertices.size()), false);
	bool has_vertices_to_remove = false;
	for (int i = 0; i < vertex_indices.size(); ++i)
	{
		const int vertex_index = vertex_indices[i];
		if (vertex_index < 0 || vertex_index >= s.vertices.size())
			continue;

		if (!remove_vertex[vertex_index])
		{
			remove_vertex[vertex_index] = true;
			has_vertices_to_remove = true;
		}
	}

	if (!has_vertices_to_remove)
		return;

	std::vector<int> remap(static_cast<size_t>(s.vertices.size()), -1);
	Vector<Surface::Vertex> new_vertices;
	new_vertices.reserve(s.vertices.size());
	for (int i = 0; i < s.vertices.size(); ++i)
	{
		if (remove_vertex[i])
			continue;

		remap[i] = new_vertices.size();
		new_vertices.append(s.vertices[i]);
	}

	Vector<int> new_indices;
	new_indices.reserve(s.indices.size());
	for (int i = 0; i + 2 < s.indices.size(); i += 3)
	{
		const int i0 = s.indices[i + 0];
		const int i1 = s.indices[i + 1];
		const int i2 = s.indices[i + 2];
		if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= s.vertices.size() || i1 >= s.vertices.size()
			|| i2 >= s.vertices.size())
		{
			continue;
		}
		if (remove_vertex[i0] || remove_vertex[i1] || remove_vertex[i2])
			continue;

		new_indices.append(remap[i0]);
		new_indices.append(remap[i1]);
		new_indices.append(remap[i2]);
	}

	s.vertices = new_vertices;
	s.indices = new_indices;
}

void MeshBuilder::removeVerticesWithoutTriangles(int surface)
{
	auto &s = surfaces[surface];
	Vector<bool> used;
	used.resize(false, s.vertices.size());
	for (int i = 0; i < s.indices.size(); i++)
		used[s.indices[i]] = true;
	for (int i = s.vertices.size() - 1; i >= 0; --i)
	{
		if (!used[i])
		{
			// remove vertex
			s.vertices.remove(i);

			// fix indices array
			for (int j = 0; j < s.indices.size(); j++)
				if (s.indices[j] > i)
					s.indices[j]--;
		}
	}
}

void MeshBuilder::splitEdge(const vec3 &pos0, const vec3 &pos1, int surface)
{
	auto posMatch = [](const vec3 &a, const vec3 &b) { return length2(a - b) <= Consts::EPS; };

	auto make_key = [](int a, int b) -> uint64_t {
		if (a > b)
			std::swap(a, b);
		return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
	};

	auto &s = surfaces[surface];

	std::unordered_map<uint64_t, int> midpoint_cache;

	int original_count = s.indices.size();
	for (int i = original_count - 3; i >= 0; i -= 3)
	{
		int i0 = s.indices[i], i1 = s.indices[i + 1], i2 = s.indices[i + 2];

		// Find which edge of this triangle matches (pos0, pos1)
		int a = -1, b = -1, c = -1;

		if ((posMatch(s.vertices[i0].position, pos0) && posMatch(s.vertices[i1].position, pos1))
			|| (posMatch(s.vertices[i0].position, pos1) && posMatch(s.vertices[i1].position, pos0)))
		{
			a = i0;
			b = i1;
			c = i2;
		}
		else if ((posMatch(s.vertices[i1].position, pos0)
					 && posMatch(s.vertices[i2].position, pos1))
				 || (posMatch(s.vertices[i1].position, pos1)
					 && posMatch(s.vertices[i2].position, pos0)))
		{
			a = i1;
			b = i2;
			c = i0;
		}
		else if ((posMatch(s.vertices[i2].position, pos0)
					 && posMatch(s.vertices[i0].position, pos1))
				 || (posMatch(s.vertices[i2].position, pos1)
					 && posMatch(s.vertices[i0].position, pos0)))
		{
			a = i2;
			b = i0;
			c = i1;
		}
		else
			continue;

		// Get or create midpoint vertex
		uint64_t key = make_key(a, b);
		int mid_idx;
		auto it = midpoint_cache.find(key);
		if (it != midpoint_cache.end())
		{
			mid_idx = it->second;
		}
		else
		{
			Surface::Vertex mid;
			mid.position = (s.vertices[a].position + s.vertices[b].position) * 0.5f;
			mid.normal = normalize(s.vertices[a].normal + s.vertices[b].normal);
			mid.uv = (s.vertices[a].uv + s.vertices[b].uv) * 0.5f;
			mid_idx = s.vertices.size();
			s.vertices.append(mid);
			midpoint_cache[key] = mid_idx;
		}

		// Replace triangle [a, b, c] with [a, mid, c]
		s.indices[i] = a;
		s.indices[i + 1] = mid_idx;
		s.indices[i + 2] = c;

		// Add triangle [mid, b, c]
		s.indices.append(mid_idx);
		s.indices.append(b);
		s.indices.append(c);
	}
}

void MeshBuilder::collapseEdge(const vec3 &pos0, const vec3 &pos1, int surface)
{
	auto posMatch = [](const vec3 &a, const vec3 &b) { return length2(a - b) <= Consts::EPS; };

	auto &s = surfaces[surface];

	vec3 midpoint = (pos0 + pos1) * 0.5f;

	// Move all vertices at pos0 or pos1 to midpoint
	for (int i = 0; i < s.vertices.size(); i++)
	{
		if (posMatch(s.vertices[i].position, pos0) || posMatch(s.vertices[i].position, pos1))
			s.vertices[i].position = midpoint;
	}

	// Remove degenerate triangles (two or more vertices at the same position)
	for (int i = s.indices.size() - 3; i >= 0; i -= 3)
	{
		const vec3 &p0 = s.vertices[s.indices[i]].position;
		const vec3 &p1 = s.vertices[s.indices[i + 1]].position;
		const vec3 &p2 = s.vertices[s.indices[i + 2]].position;
		if (posMatch(p0, p1) || posMatch(p1, p2) || posMatch(p2, p0))
		{
			s.indices.remove(i + 2);
			s.indices.remove(i + 1);
			s.indices.remove(i);
		}
	}

	removeVerticesWithoutTriangles(surface);
}

Unigine::Vector<Unigine::Math::ivec3> MeshBuilder::extrudeIsland(
	const Unigine::Vector<ivec3> &tris, float height, EXTRUDE_MODE mode, int surface)
{
	if (tris.empty())
		return {};

	auto &s = surfaces[surface];

	// special case: individual local normals
	if (mode == EXTRUDE_MODE::INDIVIDUAL_LOCAL_NORMALS)
	{
		Vector<ivec3> return_new_top_faces;
		Vector<vec3> tris_normals;
		Vector<bool> tris_used;
		tris_normals.allocate(tris.size());
		tris_used.allocate(tris.size());
		for (const ivec3 &tri : tris)
		{
			const vec3 &p0 = s.vertices[tri.x].position;
			const vec3 &p1 = s.vertices[tri.y].position;
			const vec3 &p2 = s.vertices[tri.z].position;
			tris_normals.append(normalize(cross(p1 - p0, p2 - p0)));
			tris_used.append(false);
		}
		// combine same normals and call extrudeIsland() several times
		for (int i = 0; i < tris.size(); i++)
		{
			if (tris_used[i])
				continue;
			Vector<ivec3> tris_part;
			tris_part.append(tris[i]);
			vec3 n = tris_normals[i];
			tris_used[i] = true;
			for (int j = i + 1; j < tris.size(); j++)
			{
				if (!tris_used[j] && getAngle(tris_normals[j], n) < 1.0f /*eps*/)
				{
					tris_part.append(tris[j]);
					tris_used[j] = true;
				}
			}
			return_new_top_faces.append(
				extrudeIsland(tris_part, height, MeshBuilder::EXTRUDE_MODE::LOCAL_NORMAL));
		}
		return return_new_top_faces;
	}

	// --- Step 0: Build per-vertex extrusion directions based on island face normals only
	HashMap<int, vec3> extrude_dirs;	// vertex index -> accumulated direction
	vec3 avg_normal;
	int faces_count = 0;
	for (const ivec3 &tri : tris)
	{
		const vec3 &p0 = s.vertices[tri.x].position;
		const vec3 &p1 = s.vertices[tri.y].position;
		const vec3 &p2 = s.vertices[tri.z].position;

		vec3 face_n = normalize(cross(p1 - p0, p2 - p0));
		// Accumulate face normal for each vertex of this triangle
		extrude_dirs[tri.x] = extrude_dirs[tri.x] + face_n;
		extrude_dirs[tri.y] = extrude_dirs[tri.y] + face_n;
		extrude_dirs[tri.z] = extrude_dirs[tri.z] + face_n;

		avg_normal += face_n;
		faces_count++;
	}
	avg_normal = normalize(avg_normal / itof(faces_count));

	// --- Step 1: Collect all unique vertex indices from the island
	HashSet<int> used_vertices;
	for (const ivec3 &tri : tris)
	{
		used_vertices.insert(tri.x);
		used_vertices.insert(tri.y);
		used_vertices.insert(tri.z);
	}

	// --- Step 2: Duplicate vertices with offset along vertex normals
	HashMap<int, int> index_map;	// old to new
	for (auto &it : used_vertices)
	{
		int old_idx = it.key;
		const Surface::Vertex &v = s.vertices[old_idx];
		Surface::Vertex new_v;

		// Default direction is original vertex normal (for fallback)
		vec3 dir = v.normal;

		if (mode == EXTRUDE_MODE::AVERAGE_NORMAL)
		{
			dir = avg_normal;
		}
		else if (mode == EXTRUDE_MODE::LOCAL_NORMAL)
		{
			// If we have an accumulated direction from island faces, use it instead
			auto dir_it = extrude_dirs.find(old_idx);
			if (dir_it != extrude_dirs.end())
			{
				vec3 d = dir_it->data;
				if (d.x != 0.0f || d.y != 0.0f || d.z != 0.0f)
					dir = normalize(d);
			}
		}

		new_v.position = v.position + dir * height;	   // move along island-based direction
		new_v.normal = v.normal;					   // shading normal stays as original for now
		new_v.uv = v.uv;							   // top UV stays the same

		int new_idx = s.vertices.size();
		s.vertices.append(new_v);
		index_map[old_idx] = new_idx;
	}

	// --- Step 3: Remove old island triangles from the index buffer
	// (we must find and erase them by matching all three indices)
	for (const ivec3 &tri : tris)
	{
		for (int i = 0; i < s.indices.size(); i += 3)
		{
			int a = s.indices[i + 0];
			int b = s.indices[i + 1];
			int c = s.indices[i + 2];

			if ((a == tri.x && b == tri.y && c == tri.z) || (a == tri.y && b == tri.z && c == tri.x)
				|| (a == tri.z && b == tri.x && c == tri.y))
			{
				s.indices.remove(i + 2);
				s.indices.remove(i + 1);
				s.indices.remove(i + 0);
				i -= 3;
				break;
			}
		}
	}

	// --- Step 4: Create new top faces (extruded)
	Vector<ivec3> return_new_top_faces;
	for (const ivec3 &tri : tris)
	{
		int ni0 = index_map[tri.x];
		int ni1 = index_map[tri.y];
		int ni2 = index_map[tri.z];
		s.indices.append(ni0);
		s.indices.append(ni1);
		s.indices.append(ni2);
		return_new_top_faces.append(ivec3(ni0, ni1, ni2));
	}

	// --- Step 5: Find boundary edges to build side walls
	struct Edge
	{
		int a, b;
		bool operator==(const Edge &o) const noexcept { return a == o.a && b == o.b; }
	};

	struct EdgeHash
	{
		std::size_t operator()(const Edge &e) const noexcept
		{
			return (std::size_t(e.a) * 73856093u) ^ (std::size_t(e.b) * 19349663u);
		}
	};

	std::unordered_map<Edge, int, EdgeHash> edge_use_count;

	for (const ivec3 &tri : tris)
	{
		Edge edges[3] = {{tri.x, tri.y}, {tri.y, tri.z}, {tri.z, tri.x}};
		for (const Edge &e : edges)
		{
			Edge opposite{e.b, e.a};
			if (edge_use_count.erase(opposite) == 0)
				edge_use_count[e] = 1;
		}
	}

	// --- Step 6: Build side quads with proper normals and UVs
	for (auto &kv : edge_use_count)
	{
		const Edge &e = kv.first;
		int i0 = e.a;
		int i1 = e.b;

		int ni0 = index_map[i0];
		int ni1 = index_map[i1];

		// Positions for bottom and top vertices
		vec3 p0_bottom = s.vertices[i0].position;
		vec3 p1_bottom = s.vertices[i1].position;
		vec3 p0_top = s.vertices[ni0].position;
		vec3 p1_top = s.vertices[ni1].position;

		// Compute normal for the side face
		// Triangle (p0_bottom, p1_bottom, p1_top) defines the side plane
		vec3 side_n = normalize(cross(p1_bottom - p0_bottom, p1_top - p0_bottom));

		// Create four new vertices for this side quad
		// Bottom 0, Bottom 1, Top 1, Top 0
		Surface::Vertex v0, v1, v2, v3;

		// Set positions
		v0.position = p0_bottom;
		v1.position = p1_bottom;
		v2.position = p1_top;
		v3.position = p0_top;

		// Set normals (flat side normal)
		v0.normal = side_n;
		v1.normal = side_n;
		v2.normal = side_n;
		v3.normal = side_n;

		// Set UVs in local 0..1 space
		// u: along edge, v: along height
		v0.uv = Unigine::Math::vec2(0.0f, 0.0f);
		v1.uv = Unigine::Math::vec2(1.0f, 0.0f);
		v2.uv = Unigine::Math::vec2(1.0f, 1.0f);
		v3.uv = Unigine::Math::vec2(0.0f, 1.0f);

		// Append new vertices to the mesh
		int idx_v0 = s.vertices.size();
		s.vertices.append(v0);
		int idx_v1 = s.vertices.size();
		s.vertices.append(v1);
		int idx_v2 = s.vertices.size();
		s.vertices.append(v2);
		int idx_v3 = s.vertices.size();
		s.vertices.append(v3);

		// Add two triangles for the side quad (v0, v1, v2, v3)
		// Winding chosen so that side_n points outward
		s.indices.append(idx_v0);
		s.indices.append(idx_v1);
		s.indices.append(idx_v2);

		s.indices.append(idx_v0);
		s.indices.append(idx_v2);
		s.indices.append(idx_v3);
	}

	// --- Step 7: Recompute normals for top faces
	for (const ivec3 &tri : tris)
	{
		int ni0 = index_map[tri.x];
		int ni1 = index_map[tri.y];
		int ni2 = index_map[tri.z];

		vec3 p0 = s.vertices[ni0].position;
		vec3 p1 = s.vertices[ni1].position;
		vec3 p2 = s.vertices[ni2].position;
		vec3 n = normalize(cross(p1 - p0, p2 - p0));

		s.vertices[ni0].normal = n;
		s.vertices[ni1].normal = n;
		s.vertices[ni2].normal = n;
	}

	return return_new_top_faces;
}

void MeshBuilder::removeTriangle(int triangle_index, int surface)
{
	auto &s = surfaces[surface];
	s.indices.remove(triangle_index, 3);
	removeVerticesWithoutTriangles(surface);
}

void MeshBuilder::removeTriangles(const Unigine::Vector<int> &tris_indices, int surface)
{
	auto &s = surfaces[surface];
	Vector<bool> to_remove;
	to_remove.resize(false, s.indices.size());
	for (int i = 0; i < tris_indices.size(); i++)
	{
		int t = tris_indices[i];
		to_remove[t + 0] = true;
		to_remove[t + 1] = true;
		to_remove[t + 2] = true;
	}
	for (int i = s.indices.size() - 1; i >= 0; --i)
	{
		if (to_remove[i])
			s.indices.remove(i);
	}
	removeVerticesWithoutTriangles(surface);
}

void MeshBuilder::transform(const Unigine::Math::Mat4 &trs, int surface)
{
	auto &s = surfaces[surface];
	mat4 trs_f = mat4(trs);
	mat4 normal_matrix = transpose(inverse(trs_f));
	for (int i = 0; i < s.vertices.size(); i++)
	{
		s.vertices[i].position = vec3(trs_f * vec4(s.vertices[i].position, 1.0f));
		s.vertices[i].normal = normalize(vec3(normal_matrix * vec4(s.vertices[i].normal, 0.0f)));
	}
}

void MeshBuilder::translate(const Unigine::Math::vec3 &offset, int surface)
{
	auto &s = surfaces[surface];
	for (int i = 0; i < s.vertices.size(); i++)
		s.vertices[i].position += offset;
}

void MeshBuilder::rotate(const Unigine::Math::quat &rotation, int surface)
{
	auto &s = surfaces[surface];
	for (int i = 0; i < s.vertices.size(); i++)
	{
		s.vertices[i].position = rotation * s.vertices[i].position;
		s.vertices[i].normal = rotation * s.vertices[i].normal;
	}
}

void MeshBuilder::scale(const Unigine::Math::vec3 &scale, int surface)
{
	auto &s = surfaces[surface];
	for (int i = 0; i < s.vertices.size(); i++)
		s.vertices[i].position *= scale;
}

BoundBox MeshBuilder::getBoundBox() const
{
	BoundBox bb;
	for (int i = 0; i < surfaces.size(); i++)
	{
		auto &s = surfaces[i];
		for (int j = 0; j < s.vertices.size(); j++)
		{
			auto &v = s.vertices[j];
			bb.expand(v.position);
		}
	}
	return bb;
}

BoundSphere MeshBuilder::getBoundSphere() const
{
	BoundSphere bs;
	Vector<vec3> points;
	for (int i = 0; i < surfaces.size(); i++)
	{
		auto &s = surfaces[i];
		for (int j = 0; j < s.vertices.size(); j++)
			points.append(s.vertices[j].position);
	}
	bs.set(points.get(), points.size(), points.size() < 2048);
	return bs;
}

void MeshBuilder::setPivot(const Mat4 &mesh_transform, const Mat4 &pivot_transform)
{
	Mat4 pivot_inverse_transform = inverse(pivot_transform);
	for (int i = 0; i < surfaces.size(); i++)
		transform(pivot_inverse_transform * mesh_transform, i);
}

void MeshBuilder::simplify(float quality)
{
	quality = Math::clamp(quality, 0.0f, 1.0f);

	if (quality <= 0.0f)
	{
		clear();
		return;
	}
	else if (quality >= 1.0f)
		return;

	int num_surfaces = surfaces.size();
	if (num_surfaces == 0)
		return;

	const double uv_epsilon = 1e-6;
	const double normal_quantization = 1e-3;
	const float sharp_cosine = static_cast<float>(std::cos(40.0 * Consts::DEG2RAD));
	const int max_passes = 64;

	for (int surface_index = 0; surface_index < num_surfaces; ++surface_index)
	{
		Surface &surface = surfaces[surface_index];
		const int original_triangle_count = surface.indices.size() / 3;
		if (original_triangle_count <= 0 || surface.vertices.size() < 3)
			continue;

		int target_triangle_count =
			static_cast<int>(std::ceil(static_cast<double>(original_triangle_count) * quality));
		target_triangle_count = std::max(1, target_triangle_count);
		target_triangle_count = std::min(original_triangle_count, target_triangle_count);
		if (target_triangle_count >= original_triangle_count)
			continue;

		vec3 bounds_min = surface.vertices[0].position;
		vec3 bounds_max = surface.vertices[0].position;
		for (int vertex_index = 1; vertex_index < surface.vertices.size(); ++vertex_index)
		{
			const vec3 &p = surface.vertices[vertex_index].position;
			bounds_min.x = std::min(bounds_min.x, p.x);
			bounds_min.y = std::min(bounds_min.y, p.y);
			bounds_min.z = std::min(bounds_min.z, p.z);
			bounds_max.x = std::max(bounds_max.x, p.x);
			bounds_max.y = std::max(bounds_max.y, p.y);
			bounds_max.z = std::max(bounds_max.z, p.z);
		}

		const float diagonal = std::max(length(bounds_max - bounds_min), 1.0f);
		const double weld_epsilon = std::max(1e-7, static_cast<double>(diagonal) * 1e-8);
		const double area_epsilon =
			std::max(1e-12, static_cast<double>(diagonal) * static_cast<double>(diagonal) * 1e-12);
		const double feature_weight = std::max(16.0, static_cast<double>(diagonal) * 2.0);

		std::vector<vec3> group_positions;
		std::vector<SimplifyTriangle> triangles;
		simplify_build_input(surface, weld_epsilon, area_epsilon, group_positions, triangles);
		if (triangles.empty())
		{
			surface.vertices.clear();
			surface.indices.clear();
			continue;
		}

		// Degenerate source triangles may be discarded during preprocessing.
		target_triangle_count = std::min(target_triangle_count, static_cast<int>(triangles.size()));

		for (int pass = 0;
			pass<max_passes &&static_cast<int>(triangles.size())> target_triangle_count; ++pass)
		{
			SimplifySnapshot snapshot;
			simplify_build_snapshot(
				group_positions, triangles, sharp_cosine, area_epsilon, feature_weight, snapshot);

			std::vector<SimplifyEdgeCandidate> candidates;
			simplify_build_candidates(group_positions, snapshot, candidates);
			if (candidates.empty())
				break;

			const int triangles_to_remove =
				static_cast<int>(triangles.size()) - target_triangle_count;
			int estimated_removed = 0;
			std::vector<bool> locked(group_positions.size(), false);
			std::vector<SimplifyEdgeCandidate> selected;
			selected.reserve(std::max(static_cast<size_t>(8), candidates.size() / 8));

			for (size_t candidate_index = 0; candidate_index < candidates.size(); ++candidate_index)
			{
				const SimplifyEdgeCandidate &candidate = candidates[candidate_index];
				if (estimated_removed >= triangles_to_remove)
					break;
				if (locked[candidate.a] || locked[candidate.b])
					continue;
				if (!simplify_is_valid_collapse(
						candidate, triangles, group_positions, snapshot, area_epsilon))
				{
					continue;
				}

				selected.push_back(candidate);
				locked[candidate.a] = true;
				locked[candidate.b] = true;
				estimated_removed += std::min(candidate.face_count, 2);
			}

			if (selected.empty())
				break;

			simplify_apply_collapses(group_positions, triangles, selected, area_epsilon);
		}

		simplify_rebuild_surface(
			surface, group_positions, triangles, uv_epsilon, normal_quantization);
	}
}

void MeshBuilder::merge(const Surface &src_surface, Surface &dest_surface)
{
	int index_offset = dest_surface.vertices.size();

	for (int i = 0; i < src_surface.vertices.size(); i++)
		dest_surface.vertices.append(src_surface.vertices[i]);

	for (int i = 0; i < src_surface.indices.size(); i++)
		dest_surface.indices.append(src_surface.indices[i] + index_offset);
}

void MeshBuilder::mergeSurfacesWithSameMaterials()
{
	for (int i = 0; i < surfaces.size(); i++)
	{
		for (int j = surfaces.size() - 1; j > i; j--)
		{
			if (surfaces[i].material == surfaces[j].material)
			{
				merge(surfaces[j], surfaces[i]);
				surfaces.remove(j);
			}
		}
	}
}

vec3 MeshBuilder::calcNormal(const vec3 &p0, const vec3 &p1, const vec3 &p2) const
{
	return normalize(cross(p1 - p0, p2 - p0));
}
