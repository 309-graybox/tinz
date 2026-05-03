#include "MenuButton.h"
#include "audio/SoundManager.h"

#include <UnigineGame.h>
#include <UnigineMathLibCommon.h>

REGISTER_COMPONENT(MenuButton);

using namespace Unigine;
using namespace Math;

void MenuButton::onInit()
{
	_anim_states.resize(hoverAnims.size());
	for (int i = 0; i < hoverAnims.size(); ++i)
	{
		NodePtr t = hoverAnims[i]->target;
		auto &s = _anim_states[i];
		s.t = 0.0f;
		if (t)
		{
			s.rest_pos = t->getWorldPosition();
			s.rest_rot = t->getWorldRotation();
		}
	}

	// for (int i = 0; i < hoverToggleNodes.size(); ++i)
	// {
	// 	if (NodePtr n = hoverToggleNodes.get(i))
	// 		n->setEnabled(false);
	// }
}

void MenuButton::setHovered(bool on, bool play_sound)
{
	MenuInteractive::setHovered(on, play_sound);
	update_active_state();
}

void MenuButton::onUpdate()
{
	if (!node)
		return;

	const float dt = Game::getIFps();
	const bool open = isHovered() || _pressed;

	for (int i = 0; i < hoverAnims.size() && i < _anim_states.size(); ++i)
	{
		auto &cfg = hoverAnims[i];
		auto &st = _anim_states[i];

		NodePtr target = cfg->target;
		if (!target)
			continue;

		const float aim = open ? 1.0f : 0.0f;
		const float rate = open ? (float)cfg->speed : (float)cfg->damping;
		st.t = lerp(st.t, aim, saturate(rate * dt));

		NodePtr pivot = cfg->pivot ? cfg->pivot.get() : target;
		const Vec3 pivot_pos = pivot->getWorldPosition();
		const quat pivot_rot = pivot->getWorldRotation();

		const float current_angle = st.t * (float)cfg->angle;
		const Vec3 current_offset = Vec3(cfg->offset.get()) * st.t;

		quat rot;
		vec3 axis_local = vec3(cfg->axis.get());
		if (length2(axis_local) > 1e-8f)
		{
			const vec3 axis_world = pivot_rot * normalize(axis_local);
			rot = quat(axis_world, current_angle);
		}

		const vec3 to_target = vec3(st.rest_pos - pivot_pos);
		const Vec3 new_pos = pivot_pos + Vec3(rot * to_target) + current_offset;
		const quat new_rot = rot * st.rest_rot;

		target->setWorldPosition(new_pos);
		target->setWorldRotation(new_rot);
	}
}

void MenuButton::press()
{
	if (_pressed)
		return;
	_pressed = true;
	update_active_state();

	const char *sfx = clickSound.get();
	if (sfx && *sfx)
		audio::SoundManager::play2D(sfx);
}

void MenuButton::release()
{
	if (!_pressed)
		return;
	_pressed = false;
	update_active_state();
}

void MenuButton::update_active_state()
{
	const bool active = isHovered() || _pressed;
	for (int i = 0; i < hoverToggleNodes.size(); ++i)
	{
		if (NodePtr n = hoverToggleNodes[i])
		{
			n->setEnabled(active);
		}
	}
}
