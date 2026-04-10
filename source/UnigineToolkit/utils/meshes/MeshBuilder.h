#pragma once
#include <UnigineMaterial.h>
#include <UnigineMathLib.h>
#include <UnigineMesh.h>
#include <UnigineObjects.h>
#include <UnigineString.h>

struct MeshBuilder
{
	// data
	struct Surface
	{
		Unigine::String name = "surface";

		// mesh
		struct Vertex
		{
			Unigine::Math::vec3 position;
			Unigine::Math::vec3 normal;
			Unigine::Math::vec2 uv;
		};
		Unigine::Vector<Vertex> vertices;
		Unigine::Vector<int> indices;	 // every 3 indices = 1 triangle

		// object
		bool enabled = true;
		int viewport_mask = 1;
		int shadow_mask = 1;
		Unigine::Object::SURFACE_LIGHTING_MODE lighting_mode =
			Unigine::Object::SURFACE_LIGHTING_MODE::SURFACE_LIGHTING_MODE_STATIC;
		bool cast_proj_and_omni_shadows = true;
		bool cast_world_shadows = true;
		bool cast_env_probe_shadows = true;
		Unigine::Math::vec2 visibility =
			Unigine::Math::vec2(-Unigine::Math::Consts::INF, Unigine::Math::Consts::INF);
		Unigine::Math::vec2 fade;
		Unigine::Math::ivec2 parent = Unigine::Math::ivec2(1, 1);
		bool intersection = true;
		int intersection_mask = 1;
		bool collision = true;
		int collision_mask = 1;
		bool physics_intersection = true;
		int physics_intersection_mask = 1;
		float sound_occlusion = 0;
		int sound_occlusion_mask = 1;
		float physics_friction = 0.5f;
		float physics_restitution = 0.5f;
		Unigine::MaterialPtr material;
	};
	Unigine::Vector<Surface> surfaces;

	MeshBuilder();

	// save/load .mesh files
	void clear();
	void load(const char *file_name_mesh);
	void save(const char *file_name_mesh) const;
	void load(const Unigine::MeshPtr &src_mesh, bool remap_cvertices = true);
	void load(const Unigine::ObjectMeshStaticPtr &src_object, bool remap_cvertices = true);
	void append(const Unigine::MeshPtr &src_mesh, bool remap_cvertices = true);
	void append(const Unigine::ObjectMeshStaticPtr &src_object, bool remap_cvertices = true);
	void save(const Unigine::MeshPtr &dst_mesh, bool optimize = true) const;
	void save(const char *file_name_mesh, const Unigine::ObjectMeshStaticPtr &dst_object,
		bool optimize = true) const;
	Unigine::MeshPtr getMesh(bool optimize = true) const;

	// create
	void addTriangle(const Unigine::Math::vec3 &p0, const Unigine::Math::vec3 &p1,
		const Unigine::Math::vec3 &p2, const Unigine::Math::vec2 &uv0 = Unigine::Math::vec2(0, 0),
		const Unigine::Math::vec2 &uv1 = Unigine::Math::vec2(1, 0),
		const Unigine::Math::vec2 &uv2 = Unigine::Math::vec2(1, 1), int surface = 0);

	void addQuad(const Unigine::Math::vec3 &p0, const Unigine::Math::vec3 &p1,
		const Unigine::Math::vec3 &p2, const Unigine::Math::vec3 &p3,
		const Unigine::Math::vec2 &uv0 = Unigine::Math::vec2(0, 0),
		const Unigine::Math::vec2 &uv1 = Unigine::Math::vec2(1, 0),
		const Unigine::Math::vec2 &uv2 = Unigine::Math::vec2(1, 1),
		const Unigine::Math::vec2 &uv3 = Unigine::Math::vec2(0, 1), int surface = 0);

	// vertex editing
	void removeVertex(int vertex_index, int surface = 0);
	void removeVertices(const Unigine::Vector<int> &vertex_indices, int surface = 0);
	void removeVerticesWithoutTriangles(int surface = 0);

	// edge editing
	void splitEdge(
		const Unigine::Math::vec3 &pos0, const Unigine::Math::vec3 &pos1, int surface = 0);
	void collapseEdge(
		const Unigine::Math::vec3 &pos0, const Unigine::Math::vec3 &pos1, int surface = 0);

	// face editing
	enum struct EXTRUDE_MODE { AVERAGE_NORMAL, LOCAL_NORMAL, INDIVIDUAL_LOCAL_NORMALS };
	Unigine::Vector<Unigine::Math::ivec3> extrudeIsland(
		const Unigine::Vector<Unigine::Math::ivec3> &tris, float height,
		EXTRUDE_MODE mode = EXTRUDE_MODE::LOCAL_NORMAL, int surface = 0);
	void removeTriangle(int triangle_index, int surface = 0);
	void removeTriangles(const Unigine::Vector<int> &tris_indices, int surface = 0);

	// surface editing
	void transform(const Unigine::Math::Mat4 &trs, int surface = 0);
	void translate(const Unigine::Math::vec3 &offset, int surface = 0);
	void rotate(const Unigine::Math::quat &rotation, int surface = 0);
	void scale(const Unigine::Math::vec3 &scale, int surface = 0);

	// mesh editing
	Unigine::Math::BoundBox getBoundBox() const;
	Unigine::Math::BoundSphere getBoundSphere() const;
	void setPivot(
		const Unigine::Math::Mat4 &mesh_transform, const Unigine::Math::Mat4 &pivot_transform);
	void simplify(float quality = 0.5f);
	void merge(const Surface &src_surface, Surface &dest_surface);
	void mergeSurfacesWithSameMaterials();

	// helpers
	Unigine::Math::vec3 calcNormal(const Unigine::Math::vec3 &p0, const Unigine::Math::vec3 &p1,
		const Unigine::Math::vec3 &p2) const;
};
