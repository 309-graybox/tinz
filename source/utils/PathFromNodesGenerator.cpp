#include "PathFromNodesGenerator.h"

REGISTER_COMPONENT(PathFromNodesGenerator)

using namespace Unigine;
using namespace Unigine::Math;

void PathFromNodesGenerator::init()
{
	if (points.size() == 0)
		return;

	path = Path::create();


	Unigine::Vector<float> time_stamps(points.size());
	time_stamps[0] = 0.f;
	float time_sum = 0.f;

	for (int i = 0; i < points.size() - 1; i++)
	{
		float dist = Math::length(points.get(i + 1)->getWorldPosition() - points.get(i)->getWorldPosition());
		float deltaTime = dist / targetSpeed;

		time_sum += deltaTime;
		time_stamps[i + 1] = time_sum;
	}

	for (int i = 0; i < points.size(); i++)
	{
		path->addFrame();
		path->setFramePosition(i, points.get(i)->getWorldPosition());

		path->setFrameTime(i, time_stamps[i]);
	}

	String node_name = node->getName();

	String path_path = pathDirectory + node_name + ".path";

	path->save(path_path);

	world_path = WorldTransformPath::create(path_path, 0);
	world_path->addChild(World::loadNode(target));
	world_path->setSpeed(1.f);
	world_path->setLoop(1);
	world_path->play();
}

void PathFromNodesGenerator::shutdown()
{
	world_path.deleteLater();
}
