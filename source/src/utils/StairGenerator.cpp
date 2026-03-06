#include "StairGenerator.h"

using namespace Unigine;
using namespace Unigine::Math;

REGISTER_COMPONENT(StairGenerator)

void StairGenerator::init()
{
    vec3 mesh_dimensions = vec3(stepLength.get(),stepWidth.get(),stepHight.get());

    String mesh_name = String::format("box_mesh(%.1f,%.1f,%.1f)",stepLength.get(),stepWidth.get(), stepHight.get());
    String node_name = String::format("stair(%.1f,%.1f,%.1f)",stepLength.get(),stepWidth.get(), stepHight.get());
    String mesh_path = meshSavePath.get() + mesh_name + String(".mesh");
    String node_path = stairNodeSavePath.get() + node_name + String(".node");

    stairs.reserve(stepCount.get());

    step_mesh_ptr = Mesh::create();
    step_mesh_ptr->addBoxSurface
        (
            mesh_name,
            mesh_dimensions
        );
    step_mesh_ptr->save(mesh_path);

    NodePtr stair_node = NodeDummy::create();
    stair_node->setName(node_name);

    for (int var = 0; var < 10; ++var) {
        ObjectMeshStaticPtr new_mesh = ObjectMeshStatic::create(mesh_path);
        BodyDummyPtr dummy = BodyDummy::create(new_mesh);
        ShapeBoxPtr shape = ShapeBox::create(dummy,mesh_dimensions);
        new_mesh->setName(String::format(mesh_name + "(%i)",var));
        new_mesh->setParent(stair_node);
        new_mesh->setPosition(Vec3(stepLength.get()*var,0,stepHight.get()*var));
    }

    World::saveNode(node_path,stair_node);

    stair_node.deleteLater();
}
