#include "MeshCreators.h"

using namespace Unigine;
using namespace Math;

void MeshCreators::addBox(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	const Unigine::Math::vec3 &size, int surface, int collision_data_flags)
{
	const vec3 vertex[8] = {
		vec3(-0.5f, -0.5f, -0.5f),
		vec3(0.5f, -0.5f, -0.5f),
		vec3(-0.5f, 0.5f, -0.5f),
		vec3(0.5f, 0.5f, -0.5f),
		vec3(-0.5f, -0.5f, 0.5f),
		vec3(0.5f, -0.5f, 0.5f),
		vec3(-0.5f, 0.5f, 0.5f),
		vec3(0.5f, 0.5f, 0.5f),
	};
	const vec3 normals[6] = {
		vec3(1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f),
		vec3(0.0f, 0.0f, -1.0f),
	};
	const vec2 texcoords[4] = {
		vec2(1.0f, 1.0f),
		vec2(0.0f, 1.0f),
		vec2(0.0f, 0.0f),
		vec2(1.0f, 0.0f),
	};
	static const int cindices[6][4] = {
		{3, 1, 5, 7},
		{0, 2, 6, 4},
		{2, 3, 7, 6},
		{1, 0, 4, 5},
		{6, 7, 5, 4},
		{0, 1, 3, 2},
	};
	static const int indices[6] = {
		0,
		3,
		2,
		2,
		1,
		0,
	};

	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			int index = indices[j];
			mesh->addVertex(pos + vertex[cindices[i][index]] * size, surface);
			mesh->addNormal(normals[i], surface);
			if (i == 4)
			{
				mesh->addTexCoord0(vec2_one - texcoords[index], surface);
				mesh->addTexCoord1(vec2_one - texcoords[index], surface);
			}
			else
			{
				mesh->addTexCoord0(texcoords[index], surface);
				mesh->addTexCoord1(texcoords[index], surface);
			}
		}
	}

	mesh->createIndices(surface);
	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addPlane(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	float width, float height, float step, int surface, int collision_data_flags)

{
	int width_step = max(Math::ftoi(Math::ceil(width / step)), 1);
	int height_step = max(Math::ftoi(Math::ceil(height / step)), 1);

	// vertices
	for (int i = 0; i <= height_step; i++)
	{
		float y = Math::itof(i) / height_step;
		for (int j = 0; j <= width_step; j++)
		{
			float x = Math::itof(j) / width_step;
			mesh->addVertex(pos + vec3(width * (x - 0.5f), height * (y - 0.5f), 0.0f), surface);
			mesh->addTexCoord0(vec2(x, 1.0f - y), surface);
		}
	}

	// indices
	for (int i = 0; i < height_step; i++)
	{
		for (int j = 0; j < width_step; j++)
		{
			int index = i * (width_step + 1) + j;
			if ((i & 0x01) ^ (j & 0x01))
			{
				mesh->addIndex(index, surface);
				mesh->addIndex(index + 1, surface);
				mesh->addIndex(index + width_step + 1, surface);
				mesh->addIndex(index + width_step + 2, surface);
				mesh->addIndex(index + width_step + 1, surface);
				mesh->addIndex(index + 1, surface);
			}
			else
			{
				mesh->addIndex(index, surface);
				mesh->addIndex(index + 1, surface);
				mesh->addIndex(index + width_step + 2, surface);
				mesh->addIndex(index + width_step + 2, surface);
				mesh->addIndex(index + width_step + 1, surface);
				mesh->addIndex(index, surface);
			}
		}
	}

	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addSphere(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	float radius, int stacks, int slices, int surface, int collision_data_flags)
{
	// vertices
	for (int i = 0; i <= stacks; i++)
	{
		float v = Math::itof(i) / stacks;
		float phi = Consts::PI * v - Consts::PI05;

		float cos_phi, sin_phi;
		Math::sincos(phi, sin_phi, cos_phi);

		for (int j = 0; j <= slices; j++)
		{
			float u = Math::itof(j) / slices;
			float theta = Consts::PI2 * u;

			float cos_theta, sin_theta;
			Math::sincos(theta, sin_theta, cos_theta);

			vec3 normal = vec3(cos_phi * cos_theta, -cos_phi * sin_theta, sin_phi);
			vec3 binormal = vec3(sin_phi * cos_theta, -sin_phi * sin_theta, -cos_phi);
			vec3 tangent = normalize(cross(normal, binormal));
			vec3 vertex = normal * radius;
			vec2 texcoord = vec2(1.0f - u, 1.0f - v);
			mesh->addVertex(pos + vertex, surface);
			mesh->addTangent(orthoTangent(tangent, binormal, normal), surface);
			mesh->addTexCoord0(texcoord, surface);
		}
	}

	// indices
	for (int i = 0; i < stacks; i++)
	{
		for (int j = 0; j < slices; j++)
		{
			int offset = (slices + 1) * i + j;
			if (i != 0)
			{
				mesh->addIndex(offset, surface);
				mesh->addIndex(offset + slices + 1, surface);
				mesh->addIndex(offset + 1, surface);
			}
			if (i != stacks - 1)
			{
				mesh->addIndex(offset + slices + 2, surface);
				mesh->addIndex(offset + 1, surface);
				mesh->addIndex(offset + slices + 1, surface);
			}
		}
	}

	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addEllipsoid(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
	int stacks, int slices, int surface, int collision_data_flags)
{
	// vertices
	for (int i = 0; i <= stacks; i++)
	{
		float v = Math::itof(i) / stacks;
		float phi = Consts::PI * v - Consts::PI05;

		float cos_phi, sin_phi;
		Math::sincos(phi, sin_phi, cos_phi);

		for (int j = 0; j <= slices; j++)
		{
			float u = Math::itof(j) / slices;
			float theta = Consts::PI2 * u;

			float cos_theta, sin_theta;
			Math::sincos(theta, sin_theta, cos_theta);

			vec3 normal = vec3(cos_phi * cos_theta, -cos_phi * sin_theta, sin_phi);
			vec3 binormal = vec3(sin_phi * cos_theta, -sin_phi * sin_theta, -cos_phi);
			vec3 tangent = normalize(cross(normal, binormal));
			vec3 vertex = normal * 0.5f;
			vec2 texcoord = vec2(1.0f - u, 1.0f - v);
			mesh->addVertex(transform * vertex, surface);
			mesh->addTangent(orthoTangent(tangent, binormal, normal), surface);
			mesh->addTexCoord0(texcoord, surface);
		}
	}

	// indices
	for (int i = 0; i < stacks; i++)
	{
		for (int j = 0; j < slices; j++)
		{
			int offset = (slices + 1) * i + j;
			if (i != 0)
			{
				mesh->addIndex(offset, surface);
				mesh->addIndex(offset + slices + 1, surface);
				mesh->addIndex(offset + 1, surface);
			}
			if (i != stacks - 1)
			{
				mesh->addIndex(offset + slices + 2, surface);
				mesh->addIndex(offset + 1, surface);
				mesh->addIndex(offset + slices + 1, surface);
			}
		}
	}

	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addCapsule(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	float radius, float height, int stacks, int slices, int surface, int collision_data_flags)
{
	// vertices
	for (int i = 0; i <= stacks; i++)
	{
		float v = Math::itof(i) / stacks;
		float phi = Consts::PI * v - Consts::PI05;

		float cos_phi, sin_phi;
		Math::sincos(phi, sin_phi, cos_phi);

		for (int j = 0; j <= slices; j++)
		{
			float u = Math::itof(j) / slices;
			float theta = Consts::PI2 * u;

			float cos_theta, sin_theta;
			Math::sincos(theta, sin_theta, cos_theta);

			vec3 normal = vec3(cos_phi * cos_theta, -cos_phi * sin_theta, sin_phi);
			vec3 binormal = vec3(sin_phi * cos_theta, -sin_phi * sin_theta, -cos_phi);
			vec3 tangent = normalize(cross(normal, binormal));
			vec3 vertex = normal * radius;
			vec2 texcoord = vec2(1.0f - u, 1.0f - v);
			vertex.z += Math::sign(normal.z) * height * 0.5f;
			mesh->addVertex(pos + vertex, surface);
			mesh->addTangent(orthoTangent(tangent, binormal, normal), surface);
			mesh->addTexCoord0(texcoord, surface);
		}
	}

	// indices
	for (int i = 0; i < stacks; i++)
	{
		for (int j = 0; j < slices; j++)
		{
			int offset = (slices + 1) * i + j;
			if (i != 0)
			{
				mesh->addIndex(offset, surface);
				mesh->addIndex(offset + slices + 1, surface);
				mesh->addIndex(offset + 1, surface);
			}
			if (i != stacks - 1)
			{
				mesh->addIndex(offset + slices + 2, surface);
				mesh->addIndex(offset + 1, surface);
				mesh->addIndex(offset + slices + 1, surface);
			}
		}
	}

	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addCapsule(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
	int stacks, int slices, int surface, int collision_data_flags)
{
	using namespace Unigine;
	using namespace Unigine::Math;

	// Extract center and basis vectors from transform.
	// We treat transform as position + rotated, scaled axes.
	vec3 center = vec3(transform * vec4(0.0f, 0.0f, 0.0f, 1.0f));
	vec3 axis_x = vec3(transform * vec4(1.0f, 0.0f, 0.0f, 0.0f));
	vec3 axis_y = vec3(transform * vec4(0.0f, 1.0f, 0.0f, 0.0f));
	vec3 axis_z = vec3(transform * vec4(0.0f, 0.0f, 1.0f, 0.0f));

	float scale_x = length(axis_x);
	float scale_y = length(axis_y);
	float scale_z = length(axis_z);

	// Normalized axes (orientation only).
	vec3 dir_x = (scale_x > 0.0f) ? axis_x / scale_x : vec3(1.0f, 0.0f, 0.0f);
	vec3 dir_y = (scale_y > 0.0f) ? axis_y / scale_y : vec3(0.0f, 1.0f, 0.0f);
	vec3 dir_z = (scale_z > 0.0f) ? axis_z / scale_z : vec3(0.0f, 0.0f, 1.0f);

	// Total half height of capsule along its axis.
	float half_height = scale_z * 0.5f;

	// Axial radius of spherical caps (before horizontal stretching):
	// min(size.x, size.y) * 0.5
	float radius_axis = Math::min(scale_x, scale_y) * 0.5f;

	// Clamp by height so caps always fit inside total height.
	radius_axis = Math::min(radius_axis, half_height);

	if (radius_axis <= Consts::EPS)
		return;

	// Horizontal radii (ellipse in XY plane fully fills size.x and size.y).
	float radius_x = scale_x * 0.5f;
	float radius_y = scale_y * 0.5f;

	// Half height of cylinder part along capsule axis.
	float half_cyl_height = half_height - radius_axis;
	if (half_cyl_height < 0.0f)
		half_cyl_height = 0.0f;	   // capsule degenerates to ellipsoid when too short

	// Transform local direction to world direction using orientation only.
	auto transform_dir_from_local = [&](const vec3 &d) -> vec3 {
		vec3 w = dir_x * d.x + dir_y * d.y + dir_z * d.z;
		return normalize(w);
	};

	// ------------------------------
	// VERTICES
	// ------------------------------
	for (int i = 0; i <= stacks; i++)
	{
		float v = Math::itof(i) / stacks;
		float phi = Consts::PI * v - Consts::PI05;

		float sin_phi, cos_phi;
		Math::sincos(phi, sin_phi, cos_phi);

		for (int j = 0; j <= slices; j++)
		{
			float u = Math::itof(j) / slices;
			float theta = Consts::PI2 * u;

			float sin_t, cos_t;
			Math::sincos(theta, sin_t, cos_t);

			// Local spherical directions (same pattern as original code).
			vec3 normal_local = vec3(cos_phi * cos_t, -cos_phi * sin_t, sin_phi);
			vec3 binormal_local = vec3(sin_phi * cos_t, -sin_phi * sin_t, -cos_phi);

			// World-space directions using capsule orientation only.
			vec3 normal_world = transform_dir_from_local(normal_local);
			vec3 binormal_world = transform_dir_from_local(binormal_local);
			vec3 tangent_world = normalize(cross(normal_world, binormal_world));

			// Radial offset in world XY plane:
			// X and Y are scaled independently to fill size.x and size.y.
			vec3 offset_radial =
				dir_x * (normal_local.x * radius_x) + dir_y * (normal_local.y * radius_y);

			// Offset along capsule axis:
			// - normal_local.z * radius_axis gives spherical cap shape (axial radius)
			// - sign(normal_local.z) * half_cyl_height shifts top or bottom cap.
			float sign_z = (normal_local.z >= 0.0f) ? 1.0f : -1.0f;
			float z_offset = normal_local.z * radius_axis + sign_z * half_cyl_height;

			vec3 vertex_world = center + offset_radial + dir_z * z_offset;

			vec2 texcoord = vec2(1.0f - u, 1.0f - v);

			mesh->addVertex(vertex_world, surface);
			mesh->addTangent(orthoTangent(tangent_world, binormal_world, normal_world), surface);
			mesh->addTexCoord0(texcoord, surface);
		}
	}

	// ------------------------------
	// INDICES
	// ------------------------------
	for (int i = 0; i < stacks; i++)
	{
		for (int j = 0; j < slices; j++)
		{
			int offset = (slices + 1) * i + j;

			if (i != 0)
			{
				mesh->addIndex(offset, surface);
				mesh->addIndex(offset + slices + 1, surface);
				mesh->addIndex(offset + 1, surface);
			}
			if (i != stacks - 1)
			{
				mesh->addIndex(offset + slices + 2, surface);
				mesh->addIndex(offset + 1, surface);
				mesh->addIndex(offset + slices + 1, surface);
			}
		}
	}

	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addCylinder(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	float radius, float height, int stacks, int slices, int surface, int collision_data_flags)
{
	// top triangles
	for (int i = 0; i < slices; i++)
	{
		mesh->addIndex(mesh->getNumVertex(surface), surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 1, surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 2, surface);
	}
	mesh->addVertex(pos + vec3(0.0f, 0.0f, height * 0.5f), surface);
	mesh->addNormal(vec3(0.0f, 0.0f, 1.0f), surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);
	for (int i = 0; i <= slices; i++)
	{
		float u = Math::itof(i) / slices;
		float theta = Consts::PI2 * u - Consts::PI05;
		vec3 normal = vec3(Math::cos(theta), Math::sin(theta), 0.0f);
		vec3 vertex = normal * radius;
		vertex.z += height * 0.5f;
		mesh->addVertex(pos + vertex, surface);
		mesh->addNormal(vec3(0.0f, 0.0f, 1.0f), surface);
		mesh->addTexCoord0(vec2(normal.x * 0.5f + 0.5f, 0.5f - normal.y * 0.5f), surface);
	}

	// bottom triangles
	for (int i = 0; i < slices; i++)
	{
		mesh->addIndex(mesh->getNumVertex(surface), surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 1, surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 2, surface);
	}
	mesh->addVertex(pos + vec3(0.0f, 0.0f, -height * 0.5f), surface);
	mesh->addNormal(vec3(0.0f, 0.0f, -1.0f), surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);
	for (int i = 0; i <= slices; i++)
	{
		float u = Math::itof(i) / slices;
		float theta = Consts::PI2 * u - Consts::PI;
		vec3 normal = vec3(Math::sin(theta), Math::cos(theta), 0.0f);
		vec3 vertex = normal * radius;
		vertex.z -= height * 0.5f;
		mesh->addVertex(pos + vertex, surface);
		mesh->addNormal(vec3(0.0f, 0.0f, -1.0f), surface);
		mesh->addTexCoord0(vec2(normal.x * 0.5f + 0.5f, normal.y * 0.5f + 0.5f), surface);
	}

	// side triangles
	for (int i = 0; i < stacks; i++)
	{
		for (int j = 0; j < slices; j++)
		{
			int offset = mesh->getNumVertex(surface) + (slices + 1) * i + j;
			mesh->addIndex(offset, surface);
			mesh->addIndex(offset + slices + 1, surface);
			mesh->addIndex(offset + 1, surface);
			mesh->addIndex(offset + slices + 2, surface);
			mesh->addIndex(offset + 1, surface);
			mesh->addIndex(offset + slices + 1, surface);
		}
	}
	float k = Math::max(1.0f, Math::floor(Consts::PI * radius / height));
	for (int i = 0; i <= stacks; i++)
	{
		float v = Math::itof(i) / stacks;
		for (int j = 0; j <= slices; j++)
		{
			float u = Math::itof(j) / slices;
			float theta = Consts::PI2 * u - Consts::PI05;
			vec3 normal = vec3(Math::cos(theta), Math::sin(theta), 0.0f);
			vec3 vertex = normal * radius;
			vec2 texcoord = vec2(u * k, v);
			vertex.z = height * 0.5f - height * v;
			mesh->addVertex(pos + vertex, surface);
			mesh->addNormal(normal, surface);
			mesh->addTexCoord0(texcoord, surface);
		}
	}

	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addCylinder(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
	int stacks, int slices, int surface, int collision_data_flags)
{
	using namespace Unigine;
	using namespace Unigine::Math;

	// Local "unit" cylinder: radius = 0.5, height = 1.0 (-0.5..+0.5 along Z)
	const float local_radius = 0.5f;
	const float local_height = 1.0f;
	const float half_height = local_height * 0.5f;

	// Helper to transform point (local -> world)
	auto transform_point = [&](const vec3 &p) -> vec3 { return vec3(transform * vec4(p, 1.0f)); };

	// Normal matrix (for transforming normals correctly with rotation / non-uniform scale)
	mat3 normal_matrix(transform);
	normal_matrix = inverse(transpose(normal_matrix));

	auto transform_normal = [&](const vec3 &n) -> vec3 { return normalize(normal_matrix * n); };

	// -------------------------------------------------------------------------
	// TOP CAP
	// -------------------------------------------------------------------------
	int base_top = mesh->getNumVertex(surface);

	// Indices for top fan
	for (int i = 0; i < slices; i++)
	{
		mesh->addIndex(base_top + 0, surface);		  // center
		mesh->addIndex(base_top + i + 1, surface);	  // current
		mesh->addIndex(base_top + i + 2, surface);	  // next
	}

	// Center vertex (top)
	vec3 top_center_local(0.0f, 0.0f, +half_height);
	vec3 top_center_world = transform_point(top_center_local);
	vec3 top_normal_world = transform_normal(vec3(0.0f, 0.0f, 1.0f));

	mesh->addVertex(top_center_world, surface);
	mesh->addNormal(top_normal_world, surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);

	// Ring vertices (top)
	for (int i = 0; i <= slices; i++)
	{
		float u = Math::itof(i) / slices;
		float theta = Consts::PI2 * u - Consts::PI05;

		// Direction in XY plane
		vec3 dir_local(Math::cos(theta), Math::sin(theta), 0.0f);

		vec3 vertex_local = dir_local * local_radius;
		vertex_local.z = +half_height;

		vec3 vertex_world = transform_point(vertex_local);

		mesh->addVertex(vertex_world, surface);
		mesh->addNormal(top_normal_world, surface);
		mesh->addTexCoord0(vec2(dir_local.x * 0.5f + 0.5f, 0.5f - dir_local.y * 0.5f), surface);
	}

	// -------------------------------------------------------------------------
	// BOTTOM CAP
	// -------------------------------------------------------------------------
	int base_bottom = mesh->getNumVertex(surface);

	// Indices for bottom fan
	for (int i = 0; i < slices; i++)
	{
		mesh->addIndex(base_bottom + 0, surface);		 // center
		mesh->addIndex(base_bottom + i + 1, surface);	 // current
		mesh->addIndex(base_bottom + i + 2, surface);	 // next
	}

	// Center vertex (bottom)
	vec3 bottom_center_local(0.0f, 0.0f, -half_height);
	vec3 bottom_center_world = transform_point(bottom_center_local);
	vec3 bottom_normal_world = transform_normal(vec3(0.0f, 0.0f, -1.0f));

	mesh->addVertex(bottom_center_world, surface);
	mesh->addNormal(bottom_normal_world, surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);

	// Ring vertices (bottom)
	for (int i = 0; i <= slices; i++)
	{
		float u = Math::itof(i) / slices;
		float theta = Consts::PI2 * u - Consts::PI;

		// Direction in XY plane (rotated, as in original code, for UV layout)
		vec3 dir_local(Math::sin(theta), Math::cos(theta), 0.0f);

		vec3 vertex_local = dir_local * local_radius;
		vertex_local.z = -half_height;

		vec3 vertex_world = transform_point(vertex_local);

		mesh->addVertex(vertex_world, surface);
		mesh->addNormal(bottom_normal_world, surface);
		mesh->addTexCoord0(vec2(dir_local.x * 0.5f + 0.5f, dir_local.y * 0.5f + 0.5f), surface);
	}

	// -------------------------------------------------------------------------
	// SIDE TRIANGLES
	// -------------------------------------------------------------------------
	int base_side = mesh->getNumVertex(surface);

	// Indices for side quads (grid stacks x slices)
	for (int i = 0; i < stacks; i++)
	{
		for (int j = 0; j < slices; j++)
		{
			int offset = base_side + (slices + 1) * i + j;

			mesh->addIndex(offset, surface);
			mesh->addIndex(offset + slices + 1, surface);
			mesh->addIndex(offset + 1, surface);

			mesh->addIndex(offset + slices + 2, surface);
			mesh->addIndex(offset + 1, surface);
			mesh->addIndex(offset + slices + 1, surface);
		}
	}

	// Compute world-space radius and height for UV tiling factor k
	vec3 axis_x = vec3(transform * vec4(1.0f, 0.0f, 0.0f, 0.0f));
	vec3 axis_y = vec3(transform * vec4(0.0f, 1.0f, 0.0f, 0.0f));
	vec3 axis_z = vec3(transform * vec4(0.0f, 0.0f, 1.0f, 0.0f));

	float scale_x = length(axis_x);
	float scale_y = length(axis_y);
	float scale_z = length(axis_z);

	// Approximate world radius as max XY scale times local radius
	float radius_world = local_radius * Math::max(scale_x, scale_y);
	float height_world = local_height * scale_z;

	float k = Math::max(1.0f, Math::floor(Consts::PI * radius_world / height_world));

	// Side vertices
	for (int i = 0; i <= stacks; i++)
	{
		float v = Math::itof(i) / stacks;	 // 0..1 along height

		for (int j = 0; j <= slices; j++)
		{
			float u = Math::itof(j) / slices;	 // 0..1 around
			float theta = Consts::PI2 * u - Consts::PI05;

			vec3 normal_local(Math::cos(theta), Math::sin(theta), 0.0f);
			vec3 vertex_local = normal_local * local_radius;

			// Linear interpolation from +half_height to -half_height
			vertex_local.z = +half_height - v * local_height;

			vec3 vertex_world = transform_point(vertex_local);
			vec3 normal_world = transform_normal(normal_local);

			vec2 texcoord(u * k, v);

			mesh->addVertex(vertex_world, surface);
			mesh->addNormal(normal_world, surface);
			mesh->addTexCoord0(texcoord, surface);
		}
	}

	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addPrism(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	float radius_top, float radius_bottom, float height, int sides, int surface,
	int collision_data_flags)
{
	// top triangles
	for (int i = 0; i < sides; i++)
	{
		mesh->addIndex(mesh->getNumVertex(surface), surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 1, surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 2, surface);
	}
	mesh->addVertex(pos + vec3(0.0f, 0.0f, height * 0.5f), surface);
	mesh->addNormal(vec3(0.0f, 0.0f, 1.0f), surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);
	for (int i = 0; i <= sides; i++)
	{
		float u = Math::itof(i) / sides;
		float theta = Consts::PI2 * u - Consts::PI05;
		vec3 normal = vec3(Math::cos(theta), Math::sin(theta), 0.0f);
		vec3 vertex = normal * radius_top;
		vertex.z += height * 0.5f;
		mesh->addVertex(pos + vertex, surface);
		mesh->addNormal(vec3(0.0f, 0.0f, 1.0f), surface);
		mesh->addTexCoord0(vec2(normal.x * 0.5f + 0.5f, 0.5f - normal.y * 0.5f), surface);
	}

	// bottom triangles
	for (int i = 0; i < sides; i++)
	{
		mesh->addIndex(mesh->getNumVertex(surface), surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 1, surface);
		mesh->addIndex(mesh->getNumVertex(surface) + i + 2, surface);
	}
	mesh->addVertex(pos + vec3(0.0f, 0.0f, -height * 0.5f), surface);
	mesh->addNormal(vec3(0.0f, 0.0f, -1.0f), surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);
	for (int i = 0; i <= sides; i++)
	{
		float u = Math::itof(i) / sides;
		float theta = Consts::PI2 * u - Consts::PI;
		vec3 normal = vec3(Math::sin(theta), Math::cos(theta), 0.0f);
		vec3 vertex = normal * radius_bottom;
		vertex.z -= height * 0.5f;
		mesh->addVertex(pos + vertex, surface);
		mesh->addNormal(vec3(0.0f, 0.0f, -1.0f), surface);
		mesh->addTexCoord0(vec2(normal.x * 0.5f + 0.5f, normal.y * 0.5f + 0.5f), surface);
	}

	// side triangles
	for (int i = 0; i < sides; i++)
	{
		int offset = mesh->getNumVertex(surface) + i * 4;
		mesh->addIndex(offset, surface);
		mesh->addIndex(offset + 1, surface);
		mesh->addIndex(offset + 2, surface);
		mesh->addIndex(offset + 2, surface);
		mesh->addIndex(offset + 3, surface);
		mesh->addIndex(offset, surface);
	}
	float k = Math::floor(Consts::PI * max(radius_top, radius_bottom) / height);
	for (int i = 0; i <= sides; i++)
	{
		float s0 = Math::itof(i + 0) / sides;
		float s1 = Math::itof(i + 1) / sides;
		float theta_0 = Consts::PI2 * s0 - Consts::PI05;
		float theta_1 = Consts::PI2 * s1 - Consts::PI05;
		vec3 normal_0 = vec3(Math::cos(theta_0), Math::sin(theta_0), 0.0f);
		vec3 normal_1 = vec3(Math::cos(theta_1), Math::sin(theta_1), 0.0f);
		vec3 normal = normalize(normal_0 + normal_1);
		mesh->addVertex(pos + normal_0 * radius_top + vec3(0.0f, 0.0f, height * 0.5f), surface);
		mesh->addVertex(pos + normal_0 * radius_bottom - vec3(0.0f, 0.0f, height * 0.5f), surface);
		mesh->addVertex(pos + normal_1 * radius_bottom - vec3(0.0f, 0.0f, height * 0.5f), surface);
		mesh->addVertex(pos + normal_1 * radius_top + vec3(0.0f, 0.0f, height * 0.5f), surface);
		mesh->addTexCoord0(vec2(s0 * k, 0.0f), surface);
		mesh->addTexCoord0(vec2(s0 * k, 1.0f), surface);
		mesh->addTexCoord0(vec2(s1 * k, 1.0f), surface);
		mesh->addTexCoord0(vec2(s1 * k, 0.0f), surface);
		mesh->addNormal(normal, surface);
		mesh->addNormal(normal, surface);
		mesh->addNormal(normal, surface);
		mesh->addNormal(normal, surface);
	}

	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addPrism(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
	float radius_top, float radius_bottom, int sides, int surface, int collision_data_flags)
{
	using namespace Unigine;
	using namespace Unigine::Math;

	// Helper: transform point (local -> world)
	auto transform_point = [&](const vec3 &p) -> vec3 {
		// vec4 constructor: (x, y, z, w)
		return vec3(transform * vec4(p, 1.0f));
	};

	// Helper: transform normal (local -> world), taking into account scale
	// normal_matrix = inverse(transpose(upper 3x3 of transform))
	mat3 normal_matrix(transform);
	normal_matrix = inverse(transpose(normal_matrix));

	auto transform_normal = [&](const vec3 &n) -> vec3 { return normalize(normal_matrix * n); };

	const float height = 1.0f;

	// -------------------------------------------------------------------------
	// TOP CAP
	// -------------------------------------------------------------------------

	// Indices for top cap (fan from center)
	int base_top_index = mesh->getNumVertex(surface);
	for (int i = 0; i < sides; i++)
	{
		mesh->addIndex(base_top_index + 0, surface);		// center
		mesh->addIndex(base_top_index + i + 1, surface);	// current
		mesh->addIndex(base_top_index + i + 2, surface);	// next
	}

	// Center vertex (local)
	vec3 top_center_local(0.0f, 0.0f, height * 0.5f);
	vec3 top_center_world = transform_point(top_center_local);
	vec3 top_normal_local(0.0f, 0.0f, 1.0f);
	vec3 top_normal_world = transform_normal(top_normal_local);

	mesh->addVertex(top_center_world, surface);
	mesh->addNormal(top_normal_world, surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);

	// Ring vertices for top cap
	for (int i = 0; i <= sides; i++)
	{
		float u = Math::itof(i) / sides;
		float theta = Consts::PI2 * u - Consts::PI05;

		vec3 dir_local(Math::cos(theta), Math::sin(theta), 0.0f);
		vec3 vertex_local = dir_local * radius_top;
		vertex_local.z += height * 0.5f;

		vec3 vertex_world = transform_point(vertex_local);

		mesh->addVertex(vertex_world, surface);
		mesh->addNormal(top_normal_world, surface);	   // same normal for entire top cap
		mesh->addTexCoord0(vec2(dir_local.x * 0.5f + 0.5f, 0.5f - dir_local.y * 0.5f), surface);
	}

	// -------------------------------------------------------------------------
	// BOTTOM CAP
	// -------------------------------------------------------------------------

	// Indices for bottom cap (fan from center)
	int base_bottom_index = mesh->getNumVertex(surface);
	for (int i = 0; i < sides; i++)
	{
		mesh->addIndex(base_bottom_index + 0, surface);		   // center
		mesh->addIndex(base_bottom_index + i + 1, surface);	   // current
		mesh->addIndex(base_bottom_index + i + 2, surface);	   // next
	}

	// Center vertex (bottom)
	vec3 bottom_center_local(0.0f, 0.0f, -height * 0.5f);
	vec3 bottom_center_world = transform_point(bottom_center_local);
	vec3 bottom_normal_local(0.0f, 0.0f, -1.0f);
	vec3 bottom_normal_world = transform_normal(bottom_normal_local);

	mesh->addVertex(bottom_center_world, surface);
	mesh->addNormal(bottom_normal_world, surface);
	mesh->addTexCoord0(vec2(0.5f, 0.5f), surface);

	// Ring vertices for bottom cap
	for (int i = 0; i <= sides; i++)
	{
		float u = Math::itof(i) / sides;
		float theta = Consts::PI2 * u - Consts::PI;

		// This "normal" here is just direction in XY plane, not the shading normal
		vec3 dir_local(Math::sin(theta), Math::cos(theta), 0.0f);
		vec3 vertex_local = dir_local * radius_bottom;
		vertex_local.z -= height * 0.5f;

		vec3 vertex_world = transform_point(vertex_local);

		mesh->addVertex(vertex_world, surface);
		mesh->addNormal(bottom_normal_world, surface);	  // same for bottom cap
		mesh->addTexCoord0(vec2(dir_local.x * 0.5f + 0.5f, dir_local.y * 0.5f + 0.5f), surface);
	}

	// -------------------------------------------------------------------------
	// SIDE QUADS (two triangles each)
	// -------------------------------------------------------------------------

	// First add indices (each quad uses 4 vertices in order: top0, bottom0, bottom1, top1)
	int base_side_index = mesh->getNumVertex(surface);
	for (int i = 0; i < sides; i++)
	{
		int offset = base_side_index + i * 4;
		mesh->addIndex(offset + 0, surface);
		mesh->addIndex(offset + 1, surface);
		mesh->addIndex(offset + 2, surface);

		mesh->addIndex(offset + 2, surface);
		mesh->addIndex(offset + 3, surface);
		mesh->addIndex(offset + 0, surface);
	}

	// UV tiling along the side surface
	float k = Math::floor(Consts::PI * max(radius_top, radius_bottom) / height);

	// Slope of the frustum side in local space (z component of the normal)
	float slope = (radius_bottom - radius_top) / height;

	// Precompute tip normal for cone case (radius_top ~ 0)
	// Here we just use "up" as a reasonable smooth tip normal
	vec3 tip_normal_local(0.0f, 0.0f, 1.0f);
	vec3 tip_normal_world = transform_normal(tip_normal_local);

	for (int i = 0; i < sides; i++)
	{
		float s0 = Math::itof(i + 0) / sides;
		float s1 = Math::itof(i + 1) / sides;

		float theta_0 = Consts::PI2 * s0 - Consts::PI05;
		float theta_1 = Consts::PI2 * s1 - Consts::PI05;

		vec3 dir0_local(Math::cos(theta_0), Math::sin(theta_0), 0.0f);
		vec3 dir1_local(Math::cos(theta_1), Math::sin(theta_1), 0.0f);

		// Local positions of side vertices
		vec3 v0_local = dir0_local * radius_top + vec3(0.0f, 0.0f, height * 0.5f);
		vec3 v1_local = dir0_local * radius_bottom + vec3(0.0f, 0.0f, -height * 0.5f);
		vec3 v2_local = dir1_local * radius_bottom + vec3(0.0f, 0.0f, -height * 0.5f);
		vec3 v3_local = dir1_local * radius_top + vec3(0.0f, 0.0f, height * 0.5f);

		// Transform to world space
		vec3 v0_world = transform_point(v0_local);
		vec3 v1_world = transform_point(v1_local);
		vec3 v2_world = transform_point(v2_local);
		vec3 v3_world = transform_point(v3_local);

		// Smooth local normals for each angular direction
		vec3 n0_local = normalize(vec3(dir0_local.x, dir0_local.y, -slope));
		vec3 n1_local = normalize(vec3(dir1_local.x, dir1_local.y, -slope));

		vec3 n0_world = transform_normal(n0_local);
		vec3 n1_world = transform_normal(n1_local);

		// If top radius is zero (cone tip), force a single smooth tip normal
		if (Math::abs(radius_top) < Consts::EPS)
		{
			// v0 and v3 are the top vertices for this side segment
			n0_world = tip_normal_world;
			n1_world = tip_normal_world;
		}

		mesh->addVertex(v0_world, surface);	   // top at theta_0
		mesh->addVertex(v1_world, surface);	   // bottom at theta_0
		mesh->addVertex(v2_world, surface);	   // bottom at theta_1
		mesh->addVertex(v3_world, surface);	   // top at theta_1

		mesh->addTexCoord0(vec2(s0 * k, 0.0f), surface);
		mesh->addTexCoord0(vec2(s0 * k, 1.0f), surface);
		mesh->addTexCoord0(vec2(s1 * k, 1.0f), surface);
		mesh->addTexCoord0(vec2(s1 * k, 0.0f), surface);

		// v0, v1, v2, v3 normals
		mesh->addNormal(n0_world, surface);	   // v0 (top or tip)
		mesh->addNormal(n0_world, surface);	   // v1
		mesh->addNormal(n1_world, surface);	   // v2
		mesh->addNormal(n1_world, surface);	   // v3 (top or tip)
	}

	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addWedge(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
	int surface, int collision_data_flags)
{
	const vec3 v[6] = {
		vec3(-0.5f, -0.5f, -0.5f),	  // v0  x=-0.5
		vec3(-0.5f, 0.5f, -0.5f),	  // v1
		vec3(-0.5f, -0.5f, 0.5f),	  // v2
		vec3(0.5f, -0.5f, -0.5f),	  // v3  x=+0.5
		vec3(0.5f, 0.5f, -0.5f),	  // v4
		vec3(0.5f, -0.5f, 0.5f),	  // v5
	};

	auto P = [&](const vec3 &p) { return transform * vec3(p.x, p.y, p.z); };

	auto addTri = [&](const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec2 &t0,
					  const vec2 &t1, const vec2 &t2) {
		vec3 e1 = P(p1) - P(p0);
		vec3 e2 = P(p2) - P(p0);
		vec3 n = normalize(cross(e1, e2));

		mesh->addVertex(P(p0), surface);
		mesh->addNormal(n, surface);
		mesh->addTexCoord0(t0, surface);
		mesh->addTexCoord1(t0, surface);
		mesh->addVertex(P(p1), surface);
		mesh->addNormal(n, surface);
		mesh->addTexCoord0(t1, surface);
		mesh->addTexCoord1(t1, surface);
		mesh->addVertex(P(p2), surface);
		mesh->addNormal(n, surface);
		mesh->addTexCoord0(t2, surface);
		mesh->addTexCoord1(t2, surface);
	};

	auto addQuad = [&](const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec3 &p3) {
		const vec2 t0(0.0f, 0.0f), t1(1.0f, 0.0f), t2(1.0f, 1.0f), t3(0.0f, 1.0f);
		addTri(p0, p1, p2, t0, t1, t2);
		addTri(p0, p2, p3, t0, t2, t3);
	};

	addTri(v[0], v[2], v[1], vec2(0, 0), vec2(0, 1), vec2(1, 0));
	addTri(v[3], v[4], v[5], vec2(0, 0), vec2(0, 1), vec2(1, 0));
	addQuad(v[0], v[1], v[4], v[3]);
	addQuad(v[0], v[3], v[5], v[2]);
	addQuad(v[1], v[2], v[5], v[4]);

	mesh->createIndices(surface);
	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addSteps(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
	int steps, bool add_wedge, int surface, int collision_data_flags)
{
	steps = max(1, steps);

	auto P = [&](float x, float y, float z) { return transform * vec3(x, y, z); };

	auto addTri = [&](const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec2 &t0,
					  const vec2 &t1, const vec2 &t2) {
		vec3 e1 = p1 - p0;
		vec3 e2 = p2 - p0;
		vec3 n = normalize(cross(e1, e2));
		mesh->addVertex(p0, surface);
		mesh->addNormal(n, surface);
		mesh->addTexCoord0(t0, surface);
		mesh->addTexCoord1(t0, surface);
		mesh->addVertex(p1, surface);
		mesh->addNormal(n, surface);
		mesh->addTexCoord0(t1, surface);
		mesh->addTexCoord1(t1, surface);
		mesh->addVertex(p2, surface);
		mesh->addNormal(n, surface);
		mesh->addTexCoord0(t2, surface);
		mesh->addTexCoord1(t2, surface);
	};

	auto addQuad = [&](const vec3 &p0, const vec3 &p1, const vec3 &p2, const vec3 &p3) {
		const vec2 t0(0.0f, 0.0f), t1(1.0f, 0.0f), t2(1.0f, 1.0f), t3(0.0f, 1.0f);
		addTri(p0, p1, p2, t0, t1, t2);
		addTri(p0, p2, p3, t0, t2, t3);
	};

	const float dx = 1.0f / steps;
	const float dz = 1.0f / steps;

	for (int i = 0; i < steps; ++i)
	{
		const float x0 = -0.5f + i * dx;
		const float x1 = x0 + dx;
		const float z0 = -0.5f + i * dz;
		const float z1 = z0 + dz;

		// horizontal
		addQuad(P(x0, -0.5f, z1), P(x1, -0.5f, z1), P(x1, 0.5f, z1), P(x0, 0.5f, z1));	  // top
		if (!add_wedge)
			addQuad(
				P(x0, -0.5f, z0), P(x0, 0.5f, z0), P(x1, 0.5f, z0), P(x1, -0.5f, z0));	  // bottom

		// vertical
		addQuad(P(x0, -0.5f, z0), P(x0, -0.5f, z1), P(x0, 0.5f, z1), P(x0, 0.5f, z0));	  // front
		if (!add_wedge)
			addQuad(
				P(x1, -0.5f, z0), P(x1, 0.5f, z0), P(x1, 0.5f, z1), P(x1, -0.5f, z1));	  // back

		// sides
		if (!add_wedge)
		{
			addQuad(
				P(x0, -0.5f, z0), P(x1, -0.5f, z0), P(x1, -0.5f, z1), P(x0, -0.5f, z1));	// -y
			addQuad(P(x0, 0.5f, z1), P(x1, 0.5f, z1), P(x1, 0.5f, z0), P(x0, 0.5f, z0));	// +y
		}
		else
		{
			addQuad(P(x0, -0.5f, -0.5f), P(x1, -0.5f, -0.5f), P(x1, -0.5f, z1),
				P(x0, -0.5f, z1));	  // -y
			addQuad(
				P(x0, 0.5f, z1), P(x1, 0.5f, z1), P(x1, 0.5f, -0.5f), P(x0, 0.5f, -0.5f));	  // +y
		}
	}

	if (add_wedge)
	{
		// bottom
		addQuad(P(-0.5f, 0.5f, -0.5f), P(0.5f, 0.5f, -0.5f), P(0.5f, -0.5f, -0.5f),
			P(-0.5f, -0.5f, -0.5f));

		// back
		addQuad(
			P(0.5f, -0.5f, -0.5f), P(0.5f, 0.5f, -0.5f), P(0.5f, 0.5f, 0.5f), P(0.5f, -0.5f, 0.5f));
	}

	mesh->createIndices(surface);
	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addIcosahedron(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	float radius, int surface, int collision_data_flags)
{
	const float ratio = 1.618034f;
	const float scale = 0.850651f;
	const float a = scale;
	const float b = scale / ratio;
	const float c = 0.0f;

	const vec3 vertex[12] = {
		vec3(c, b, -a),
		vec3(b, a, c),
		vec3(-b, a, c),
		vec3(c, b, a),
		vec3(c, -b, a),
		vec3(-a, c, b),
		vec3(c, -b, -a),
		vec3(a, c, -b),
		vec3(a, c, b),
		vec3(-a, c, -b),
		vec3(b, -a, c),
		vec3(-b, -a, c),
	};
	const vec3 normals[20] = {
		vec3(0.000000f, 0.934172f, -0.356822f),
		vec3(0.000000f, 0.934172f, 0.356822f),
		vec3(-0.356822f, 0.000000f, 0.934172f),
		vec3(0.356822f, 0.000000f, 0.934172f),
		vec3(0.356822f, 0.000000f, -0.934172f),
		vec3(-0.356822f, -0.000000f, -0.934172f),
		vec3(0.000000f, -0.934172f, 0.356822f),
		vec3(0.000000f, -0.934172f, -0.356822f),
		vec3(-0.934172f, 0.356822f, 0.000000f),
		vec3(-0.934172f, -0.356822f, 0.000000f),
		vec3(0.934172f, 0.356822f, 0.000000f),
		vec3(0.934172f, -0.356822f, 0.000000f),
		vec3(-0.577350f, 0.577350f, 0.577350f),
		vec3(0.577350f, 0.577350f, 0.577350f),
		vec3(-0.577350f, 0.577350f, -0.577350f),
		vec3(0.577350f, 0.577350f, -0.577350f),
		vec3(-0.577350f, -0.577350f, -0.577350f),
		vec3(0.577350f, -0.577350f, -0.577350f),
		vec3(-0.577350f, -0.577350f, 0.577350f),
		vec3(0.577350f, -0.577350f, 0.577350f),
	};
	const vec2 texcoords[3] = {
		vec2(0.50f, 0.93f),
		vec2(0.0f, 0.25f),
		vec2(1.0f, 0.25f),
	};
	static const int cindices[20][3] = {
		{0, 2, 1},
		{3, 1, 2},
		{3, 5, 4},
		{3, 4, 8},
		{0, 7, 6},
		{0, 6, 9},
		{4, 11, 10},
		{6, 10, 11},
		{2, 9, 5},
		{11, 5, 9},
		{1, 8, 7},
		{10, 7, 8},
		{3, 2, 5},
		{3, 8, 1},
		{0, 9, 2},
		{0, 1, 7},
		{6, 11, 9},
		{6, 7, 10},
		{4, 5, 11},
		{4, 10, 8},
	};
	static const int indices[] = {
		0,
		1,
		2,
	};

	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			int index = indices[j];
			mesh->addVertex(pos + vertex[cindices[i][index]] * radius, surface);
			mesh->addNormal(normals[i], surface);
			mesh->addTexCoord0(texcoords[index], surface);
			mesh->addTexCoord1(texcoords[index], surface);
		}
	}

	mesh->createIndices(surface);
	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addDodecahedron(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
	float radius, int surface, int collision_data_flags)
{
	const float ratio = 1.618034f;
	const float scale = 0.577350f;
	const float a = scale * ratio;
	const float b = scale * ratio - scale;
	const float c = scale;
	const float d = 0.0f;

	const vec3 vertex[20] = {
		vec3(c, c, c),
		vec3(c, c, -c),
		vec3(c, -c, c),
		vec3(c, -c, -c),
		vec3(-c, c, c),
		vec3(-c, c, -c),
		vec3(-c, -c, c),
		vec3(-c, -c, -c),
		vec3(d, a, b),
		vec3(d, a, -b),
		vec3(d, -a, b),
		vec3(d, -a, -b),
		vec3(b, d, a),
		vec3(b, d, -a),
		vec3(-b, d, a),
		vec3(-b, d, -a),
		vec3(a, b, d),
		vec3(a, -b, d),
		vec3(-a, b, d),
		vec3(-a, -b, d),
	};
	const vec3 normals[12] = {
		vec3(-0.000000f, 0.525731f, 0.850651f),
		vec3(0.000000f, 0.525731f, -0.850651f),
		vec3(0.000000f, -0.525731f, 0.850651f),
		vec3(-0.000000f, -0.525731f, -0.850651f),
		vec3(0.850651f, -0.000000f, 0.525731f),
		vec3(0.850651f, 0.000000f, -0.525731f),
		vec3(-0.850651f, 0.000000f, 0.525731f),
		vec3(-0.850651f, -0.000000f, -0.525731f),
		vec3(0.525731f, 0.850651f, -0.000000f),
		vec3(0.525731f, -0.850651f, 0.000000f),
		vec3(-0.525731f, 0.850651f, 0.000000f),
		vec3(-0.525731f, -0.850651f, -0.000000f),
	};
	const vec2 texcoords[5] = {
		vec2(0.50f, 1.00f),
		vec2(0.98f, 0.65f),
		vec2(0.79f, 0.10f),
		vec2(0.21f, 0.10f),
		vec2(0.02f, 0.65f),
	};
	static const int cindices[12][5] = {
		{8, 4, 14, 12, 0},
		{9, 1, 13, 15, 5},
		{10, 2, 12, 14, 6},
		{11, 7, 15, 13, 3},
		{12, 2, 17, 16, 0},
		{13, 1, 16, 17, 3},
		{14, 4, 18, 19, 6},
		{15, 7, 19, 18, 5},
		{16, 1, 9, 8, 0},
		{17, 2, 10, 11, 3},
		{18, 4, 8, 9, 5},
		{19, 7, 11, 10, 6},
	};
	static const int indices[9] = {
		0,
		1,
		2,
		0,
		2,
		3,
		0,
		3,
		4,
	};

	for (int i = 0; i < 12; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			int index = indices[j];
			mesh->addVertex(pos + vertex[cindices[i][index]] * radius, surface);
			mesh->addNormal(normals[i], surface);
			mesh->addTexCoord0(texcoords[index], surface);
			mesh->addTexCoord1(texcoords[index], surface);
		}
	}

	mesh->createIndices(surface);
	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}

void MeshCreators::addWall(const Unigine::MeshPtr &mesh,
	const Unigine::Vector<Unigine::Math::vec3> &points, float width, float height, int surface,
	int collision_data_flags)
{
	int n = points.size();
	if (n < 2)
		return;

	auto dotXY = [](const vec3 &a, const vec3 &b) -> float { return a.x * b.x + a.y * b.y; };

	auto lengthXY = [](const vec3 &v) -> float { return std::sqrt(v.x * v.x + v.y * v.y); };

	auto normalizeXY = [](const vec3 &v) -> vec3 {
		float len = std::sqrt(v.x * v.x + v.y * v.y);
		if (len < 1e-6f)
			return {0.0f, 0.0f, 0.0f};
		float inv = 1.0f / len;
		// z doesn't affect in XY
		return {v.x * inv, v.y * inv, v.z * inv};
	};

	// perdendicular to left in XY-plane
	auto leftPerpXY = [](const vec3 &dir) -> vec3 {
		// dir = (dx, dy, dz) -> left = (-dy, dx, 0)
		return {-dir.y, dir.x, 0.0f};
	};

	float halfWidth = width * 0.5f;

	// 1. normals for segments
	VectorStack<vec3> segLeft(n - 1);
	for (int i = 0; i + 1 < n; ++i)
	{
		vec3 dir = normalizeXY(points[i + 1] - points[i]);
		segLeft[i] = normalizeXY(leftPerpXY(dir));
	}

	// 2. offsets for every points
	VectorStack<vec3> offsets(n);
	for (int i = 0; i < n; ++i)
	{
		vec3 nLeft{0, 0, 0};
		if (i == 0)
		{
			// first - get normal of fist segment
			nLeft = segLeft[0];
		}
		else if (i == n - 1)
		{
			// last - normal of last segment
			nLeft = segLeft[n - 2];
		}
		else
		{
			// inner: average of two normals
			vec3 n0 = segLeft[i - 1];
			vec3 n1 = segLeft[i];
			vec3 nSum = {n0.x + n1.x, n0.y + n1.y, 0.0f};

			float len = lengthXY(nSum);
			if (len < 1e-6f)
			{
				// almost a right angle or a 180 turn - fallback
				nLeft = n1;
			}
			else
			{
				nLeft = vec3(nSum.x / len, nSum.y / len, 0.0f);

				// correct width (miter)
				float cosHalf = dotXY(nLeft, n0);
				cosHalf = std::max(cosHalf, 0.1f);	  // protect from division by zero
				float scale = halfWidth / cosHalf;
				offsets[i] = nLeft * scale;
				continue;
			}
		}

		// fore endings or fallback - no miter
		offsets[i] = nLeft * halfWidth;
	}

	// 3. build left and right borders
	VectorStack<vec3> leftPts(n), rightPts(n);
	for (int i = 0; i < n; ++i)
	{
		leftPts[i] = points[i] + offsets[i];
		rightPts[i] = points[i] - offsets[i];
	}

	// 4. build polygon
	VectorStack<vec3> polygon;
	for (int i = 0; i < n; ++i)	   // right side
		polygon.push_back(rightPts[i]);
	for (int i = 0; i < n; ++i)	   // left side in reverse order
		polygon.push_back(leftPts[n - 1 - i]);

	addPolygon(mesh, polygon, height, surface, collision_data_flags);
}

namespace {
void triangulate_poly_ear(const Vector<vec3> &in_vertices, vec3 &out_normal, Vector<vec2> &out_uvs,
	Vector<ivec3> &out_tris)
{
	auto is_ear = [](const Vector<vec2> &poly, int i0, int i1, int i2,
					  const Vector<int> &indices) -> bool {
		vec2 a = poly[indices[i0]];
		vec2 b = poly[indices[i1]];
		vec2 c = poly[indices[i2]];

		// area
		float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
		if (cross <= 0.0f)
			return false;

		// check if other vertices lies inside this triangle
		for (int j = 0; j < indices.size(); j++)
		{
			if (j == i0 || j == i1 || j == i2)
				continue;
			vec2 p = poly[indices[j]];

			// baricentric coordinates
			float w1 = (a.x * (c.y - a.y) + (p.y - a.y) * (c.x - a.x) - p.x * (c.y - a.y))
					   / ((b.y - a.y) * (c.x - a.x) - (b.x - a.x) * (c.y - a.y));
			float w2 = (p.y - a.y - w1 * (b.y - a.y)) / (c.y - a.y);
			if (w1 >= 0 && w2 >= 0 && (w1 + w2) <= 1)
			{
				return false;
			}
		}
		return true;
	};

	out_uvs.clear();
	out_tris.clear();

	int n = in_vertices.size();
	if (n < 3)
		return;

	// 1. Find normal (Newton)
	out_normal = vec3_zero;
	for (int i = 0; i < n; i++)
	{
		const vec3 &cur = in_vertices[i];
		const vec3 &next = in_vertices[(i + 1) % n];
		out_normal.x += (cur.y - next.y) * (cur.z + next.z);
		out_normal.y += (cur.z - next.z) * (cur.x + next.x);
		out_normal.z += (cur.x - next.x) * (cur.y + next.y);
	}
	out_normal = normalize(out_normal);

	// 2. Local basis
	vec3 up = abs(out_normal.y) < 0.9f ? vec3(0, 1, 0) : vec3(1, 0, 0);
	vec3 tangent = normalize(cross(up, out_normal));
	vec3 bitangent = cross(out_normal, tangent);

	// 3. Project vertices to 2D (UV)
	for (int i = 0; i < n; i++)
	{
		float u = dot(in_vertices[i], tangent);
		float v = dot(in_vertices[i], bitangent);
		out_uvs.append(vec2(u, v));
	}

	// 4. Ear clipping
	Vector<int> indices;
	indices.resize(n);
	for (int i = 0; i < n; i++)
		indices[i] = i;

	while (indices.size() > 3)
	{
		bool ear_found = false;
		for (int i = 0; i < indices.size(); i++)
		{
			int i0 = (i + indices.size() - 1) % indices.size();
			int i1 = i;
			int i2 = (i + 1) % indices.size();

			if (is_ear(out_uvs, i0, i1, i2, indices))
			{
				out_tris.append(ivec3(indices[i0], indices[i1], indices[i2]));
				indices.remove(i1);
				ear_found = true;
				break;
			}
		}
		if (!ear_found)
		{
			// fallback: something going wrong
			break;
		}
	}
	if (indices.size() == 3)
	{
		out_tris.append(ivec3(indices[0], indices[1], indices[2]));
	}
}
}	 // namespace

void MeshCreators::addPolygon(const Unigine::MeshPtr &mesh,
	const Unigine::Vector<Unigine::Math::vec3> &points, float height, int surface,
	int collision_data_flags)
{
	if (points.size() < 3)
		return;

	vec3 normal = vec3_up;
	Vector<vec2> uvs;
	Vector<ivec3> tris;
	triangulate_poly_ear(points, normal, uvs, tris);

	// front
	for (int i = 0; i < points.size(); i++)
	{
		mesh->addVertex(points[i] + normal * height);
		mesh->addNormal(normal);
		mesh->addTexCoord0(uvs[i]);
	}
	for (int i = 0; i < tris.size(); i++)
	{
		mesh->addIndex(tris[i].x);
		mesh->addIndex(tris[i].y);
		mesh->addIndex(tris[i].z);
	}

	// back
	int index = mesh->getNumVertex(0);
	for (int i = 0; i < points.size(); i++)
	{
		mesh->addVertex(points[i]);
		mesh->addNormal(-normal);
		mesh->addTexCoord0(uvs[i]);
	}
	for (int i = 0; i < tris.size(); i++)
	{
		mesh->addIndex(index + tris[i].x);
		mesh->addIndex(index + tris[i].z);
		mesh->addIndex(index + tris[i].y);
	}

	// side
	index = mesh->getNumVertex(0);
	for (int i = 0; i < points.size(); i++)
	{
		vec3 p0 = points[(i - 1) < 0 ? (points.size() - 1) : (i - 1)];
		vec3 p1 = points[i];
		vec3 p2 = points[i] + normal * height;
		vec3 p3 = points[(i - 1) < 0 ? (points.size() - 1) : (i - 1)] + normal * height;
		mesh->addVertex(p0);
		mesh->addVertex(p1);
		mesh->addVertex(p2);
		mesh->addVertex(p3);
		vec3 n = normalize(cross(p1 - p0, p2 - p0));
		mesh->addNormal(n);
		mesh->addNormal(n);
		mesh->addNormal(n);
		mesh->addNormal(n);
		mesh->addTexCoord0(vec2(0, 0));
		mesh->addTexCoord0(vec2(1, 0));
		mesh->addTexCoord0(vec2(1, 1));
		mesh->addTexCoord0(vec2(0, 1));
		mesh->addIndex(index + 0);
		mesh->addIndex(index + 1);
		mesh->addIndex(index + 2);
		mesh->addIndex(index + 0);
		mesh->addIndex(index + 2);
		mesh->addIndex(index + 3);
		index += 4;
	}

	mesh->createTangents(surface);
	mesh->createCollisionData(surface, collision_data_flags);
}
