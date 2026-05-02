#include "Entity.h"
#include "utils/Utils.h"

#include <UnigineGame.h>

REGISTER_COMPONENT(Entity)

using namespace Unigine;
using namespace Unigine::Math;

bool Entity::isInvulnerable() const noexcept
{
	return Game::getTime() < _invulnerable_until;
}

bool Entity::takeDamage(const DamageInfo &damageInfo)
{
	const float amount = !isInf(damageInfo.amount.get()) ? _hp : damageInfo.amount;
	const bool receivesDamage = amount > 0.0f;
	if (isDead())
	{
		Log::message("%s damage ignored: already dead, amount: %.2f\n", node->getName(), amount);
		return false;
	}

	if (receivesDamage && isInvulnerable())
	{
		Log::message("%s damage ignored: invulnerable, amount: %.2f, time left: %.2f\n",
			node->getName(), amount, max(_invulnerable_until - Game::getTime(), 0.0f));
		return false;
	}

	const float old_hp = _hp;
	_hp = max(0.0f, _hp - amount);
	if (receivesDamage)
		_invulnerable_until = Game::getTime() + max(invulnerabilityTime.get(), 0.0f);

	Log::message("%s hp changed: %.2f -> %.2f, amount: %.2f, invulnerability: %.2f\n",
		node->getName(), old_hp, _hp, amount, receivesDamage ? max(invulnerabilityTime.get(), 0.0f) : 0.0f);

	if (isDead())
	{
		Log::message("%s died\n", node->getName());
		updateDeathStates();
		_event_died.run(this);
		if (!persistOnDeath)
			node.deleteLater();
	}

	return !compare(old_hp, _hp);
}

void Entity::revive()
{
	_hp = max_hp;
	_invulnerable_until = 0.0f;

	updateDeathStates();
}

void Entity::init()
{
	_hp = max_hp;
	_invulnerable_until = 0.0f;

	auto rig = node->getObjectBodyRigid();
	if (rig)
	{
		_init_gravity = rig->isGravity();
		_init_damping = rig->getLinearDamping();
		_init_mass = rig->getMass();
	}

	convertTo(enable_on_death, _enableOnDeath);
	convertTo(disable_on_death, _disableOnDeath);

	int n = mat_float4_params.size();
	for (int i = 0; i < n; ++i)
	{
		auto &param = mat_float4_params[i];

		auto target = node;
		if (param->apply_to_target)
			target = param->target;

		auto obj = checked_ptr_cast<Object>(target);

		if (!obj)
			continue;

		auto mat = obj->getMaterial(param->surface);
		if (!mat || mat->findParameter(param->param) == -1)
			continue;

		auto el = param;

		param->material = mat;
		vec4 v = mat->getParameterFloat4(param->param);
		vec4 dv = param->death_value;
		param->init_value = v;

		_matFloatParams.append(param);
	}
}

void Entity::update()
{
	if (has_hp && isDead())
	{
		if (_death_time > Consts::EPS && Game::getTime() - _death_time >= deleteTimer)
			node.deleteLater();
	}
}

void Entity::shutdown()
{
}

void Entity::updateDeathStates()
{
	bool dead = isDead();
	setEnabledArr(_enableOnDeath, dead);
	setEnabledArr(_disableOnDeath, !dead);
	applyMatParams(_matFloatParams, dead);

	if (dead)
		_death_time = Game::getTime();

	auto rig = node->getObjectBodyRigid();
	if (rig)
	{
		rig->setGravity(dead || _init_gravity);
		rig->getEventContactEnter().setEnabled(!dead);
		rig->getEventContactLeave().setEnabled(!dead);
		rig->getEventContacts().setEnabled(!dead);
		rig->setLinearDamping(dead ? 1.5f : _init_damping);
		if (!rig->isShapeBased())
			rig->setMass(dead ? 0.1f : _init_mass);
	}
}

void Entity::setEnabledArr(const Unigine::Vector<Unigine::NodePtr> &arr, bool enabled)
{
	for (const auto &n : arr)
	{
		if (n)
			n->setEnabled(enabled);
	}
}

void Entity::applyMatParams(const Unigine::Vector<MaterialFloat4ParamInfo> &params, bool dead)
{
	for (const auto &param : params)
	{
		param.material->setParameterFloat4(param.param, dead ? param.death_value : param.init_value);
	}
}
