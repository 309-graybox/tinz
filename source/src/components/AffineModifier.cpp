#include "AffineModifier.h"
#include "utils/Utils.h"

#include <UnigineComponentSystem.h>
#include <UnigineGame.h>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(AffineModifier);

void AffineModifier::init()
{
	_anim_states.resize(anims.size());
	for (int i = 0; i < anims.size(); ++i)
	{
		NodePtr t = anims[i]->target;
		FLOGERR(t, "target is not set : %s", node->getName());
		// FLOGERR(anims[i]->pivot, "pivot is not set: %s",  node->getName());
		auto &s = _anim_states[i];
		s.t = 0.0f;
		if (t)
		{
			s.rest_pos = t->getWorldPosition();
			s.rest_rot = t->getWorldRotation();
		}
	}
}

void AffineModifier::update()
{
	if (!_need_update)
		return;

	const float dt = Game::getIFps();
	int finish = 0;

	for (int i = 0; i < anims.size() && i < _anim_states.size(); ++i)
	{
		auto &cfg = anims[i];
		auto &st = _anim_states[i];
		if (!st._need_update)
		{
			++finish;
			continue;
		}

		NodePtr target = cfg->target;
		if (!target)
		{
			++finish;
			continue;
		}

		const float aim = _open ? 1.0f : 0.0f;
		const float rate = _open ? (float)cfg->speed : (float)cfg->damping;
		st.t = lerp(st.t, aim, saturate(rate * dt));

		NodePtr pivot = cfg->pivot ? cfg->pivot : target;
		Vec3 pivot_pos;
		quat pivot_rot;
		if (pivot == target)
		{
			pivot_pos = st.rest_pos;
			pivot_rot = st.rest_rot;
		}
		else
		{
			pivot_pos = pivot->getWorldPosition();
			pivot_rot = pivot->getWorldRotation();
		}

		const float current_angle = st.t * (float)cfg->angle;
		const Vec3 current_offset = Vec3(cfg->offset) * st.t;

		quat rot = quat_identity;
		vec3 axis_local = vec3(cfg->axis);
		if (length2(axis_local) > 1e-8f)
		{
			const vec3 axis_world = pivot_rot * normalize(axis_local);
			rot = quat(axis_world, current_angle);
		}

		const vec3 to_target = vec3(st.rest_pos - pivot_pos);
		const Vec3 new_pos = pivot_pos + Vec3(rot * to_target) + current_offset;
		const quat new_rot = rot * st.rest_rot;

		auto new_trans = Mat4(new_rot, new_pos);

		if (compare(st.t, aim))
		{
			++finish;
			st._need_update = false;
		}

		target->setWorldTransform(new_trans);
	}

	if (finish == _anim_states.size())
	{
		_need_update = false;
	}
}

void AffineModifier::setOpen(bool open)
{ 
	_need_update = true;
	_open = open;

	for (auto &state : _anim_states)
	{
		state._need_update = true;
	}
};