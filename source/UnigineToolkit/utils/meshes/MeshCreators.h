#pragma once
#include <UnigineMathLib.h>
#include <UnigineMesh.h>

class MeshCreators
{
public:
	static void addBox(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
		const Unigine::Math::vec3 &size, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addPlane(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos, float width,
		float height, float step, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addSphere(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
		float radius, int stacks, int slices, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addEllipsoid(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
		int stacks, int slices, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addCapsule(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
		float radius, float height, int stacks, int slices, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addCapsule(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
		int stacks, int slices, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addCylinder(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
		float radius, float height, int stacks, int slices, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addCylinder(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
		int stacks, int slices, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addPrism(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
		float radius_top, float radius_bottom, float height, int sides, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addPrism(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
		float radius_top, float radius_bottom, int sides, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addWedge(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
		int surface = 0, int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addSteps(const Unigine::MeshPtr &mesh, const Unigine::Math::mat4 &transform,
		int steps_count, bool add_wedge = false, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addIcosahedron(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
		float radius, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addDodecahedron(const Unigine::MeshPtr &mesh, const Unigine::Math::vec3 &pos,
		float radius, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addWall(const Unigine::MeshPtr &mesh,
		const Unigine::Vector<Unigine::Math::vec3> &points, float width, float height = 0,
		int surface = 0, int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);

	static void addPolygon(const Unigine::MeshPtr &mesh,
		const Unigine::Vector<Unigine::Math::vec3> &points, float height = 0, int surface = 0,
		int collision_data_flags = Unigine::Mesh::COLLISION_DATA_ALL);
};
