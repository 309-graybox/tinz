#pragma once
#include <UnigineComponentSystem.h>

class StairGenerator: public Unigine::ComponentBase
{
public:
    COMPONENT_DEFINE(StairGenerator,Unigine::ComponentBase)
    COMPONENT_INIT(init)

    PROP_GROUP("Stair Parameters")
    PROP_PARAM(Float, stepHight, 0.3f)
    PROP_PARAM(Float, stepWidth, 2.f)
    PROP_PARAM(Float, stepLength, 1.f)
    PROP_PARAM(Int, stepCount, 2)

    PROP_GROUP("File System")
    PROP_PARAM(String, meshSavePath)
    PROP_PARAM(String, stairNodeSavePath)


private:
    void init();

private:
    Unigine::MeshPtr step_mesh_ptr = nullptr;
    Unigine::Vector<Unigine::ObjectMeshStaticPtr> stairs {nullptr};
};

