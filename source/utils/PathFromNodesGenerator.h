#ifndef PATHFROMNODESGENERATOR_H
#define PATHFROMNODESGENERATOR_H

#include <UnigineComponentSystem.h>
#include <UnigineWorlds.h>

class PathFromNodesGenerator : public Unigine::ComponentBase
{
    COMPONENT_DEFINE(PathFromNodesGenerator,ComponentBase)
    COMPONENT_INIT(init)
    COMPONENT_SHUTDOWN(shutdown)

    PROP_PARAM(File, target)
    PROP_PARAM(Float, targetSpeed)
    PROP_PARAM(String, pathDirectory)
    PROP_ARRAY(Node, points)

private:

    void init();
    void shutdown();

    Unigine::PathPtr path = nullptr;
    Unigine::WorldTransformPathPtr world_path = nullptr;
};

#endif // PATHFROMENODESGENERATOR_H
