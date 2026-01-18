#pragma once
#include <UnigineNode.h>

struct CameraContext
{
	Unigine::NodePtr target;
	Unigine::NodePtr camera_node;

	int collision_mask = 0xffffffff;
};
