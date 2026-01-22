#include "PathFromNodesGenerator.h"
#include "utils/Utils.h"

REGISTER_COMPONENT(PathFromNodesGenerator)

using namespace Unigine;
using namespace Unigine::Math;

void PathFromNodesGenerator::init()
{
	FLOGERR(!compare(targetSpeed, 0.0f), "invalid zero speed\n");
	FLOGERR(points.size() != 0, "invalid path len 0\n");

	auto targetNode = World::loadNode(target);
	FLOGERR(targetNode, "can not load target node %s\n", target.get());

	auto path = Path::create();

	float time_sum = 0.0f;

	auto prevPoint = points.get(0)->getWorldPosition();
	for (int i = 0; i < points.size(); i++)
	{
		auto curPoint = points.get(i)->getWorldPosition();
		float dist = distance(curPoint, prevPoint);
		prevPoint = curPoint;

		time_sum += dist / targetSpeed;

		path->addFrame();
		path->setFramePosition(i, points.get(i)->getWorldPosition());
		path->setFrameTime(i, time_sum);
	}

	String savePath = pathDirectory + node->getName() + ".path";

	FLOGERR(path->save(savePath), "can not save path \"%s\" for %i node\n", savePath.get(), node->getID());

	_worldPath = WorldTransformPath::create(savePath, 0);
	_worldPath->addChild(targetNode);
	_worldPath->setSpeed(1.f);
	_worldPath->setLoop(1);
	_worldPath->play();
}

void PathFromNodesGenerator::shutdown()
{
	_worldPath.deleteLater();
}
