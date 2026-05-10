#include "components/combat/Hurtbox.h"
#include "components/Entity.h"

#include <UnigineVisualizer.h>

REGISTER_COMPONENT(Hurtbox)

using namespace Unigine;
using namespace Unigine::Math;

namespace
{
// File-local registry storage. Returned by reference from
// Hurtbox::registry(). Lives for the life of the process, but holds raw
// non-owning pointers — every Hurtbox component must remove itself on
// shutdown. Pointer-only entries are fine (sizeof, no destruction order
// concerns).
Vector<Hurtbox *> &mutableRegistry()
{
	static Vector<Hurtbox *> instance;
	return instance;
}
} // namespace

const Vector<Hurtbox *> &Hurtbox::registry()
{
	return mutableRegistry();
}

float Hurtbox::enclosingRadius() const noexcept
{
	const float r = max(radius.get(), 0.0f);
	if (getShape() == Shape::Capsule)
		return r + max(capsuleHeight.get(), 0.0f) * 0.5f;
	return r;
}

void Hurtbox::init()
{
	_entity = nullptr;
	_entity_resolved = false;

	if (!_registered)
	{
		mutableRegistry().append(this);
		_registered = true;
	}
}

void Hurtbox::shutdown()
{
	if (!_registered)
		return;

	auto &reg = mutableRegistry();
	const int idx = reg.findIndex(this);
	if (idx >= 0)
		reg.remove(idx);
	_registered = false;
}

void Hurtbox::update()
{
	if (!debugDraw)
		return;

	const Mat4 t = node->getWorldTransform();
	const vec4 color(0.2f, 1.0f, 1.0f, 1.0f);

	switch (getShape())
	{
		case Shape::Sphere:
			Visualizer::renderSphere(max(radius.get(), 0.0f), translate(t.getTranslate()), color);
			break;

		case Shape::Capsule:
		{
			// Visualizer::renderCapsule renders along the transform's local Z;
			// rotate the viz frame so its Z column points along the chosen
			// capsule axis of `t`.
			Mat4 viz = t;
			switch ((int)capsuleAxis)
			{
				case 0: viz = t * rotateY(Scalar(90.0)); break;   // local X
				case 1: viz = t * rotateX(Scalar(-90.0)); break;  // local Y
				default: break;                                    // local Z
			}
			Visualizer::renderCapsule(max(radius.get(), 0.0f), max(capsuleHeight.get(), 0.0f), viz, color);
			break;
		}
	}
}

void Hurtbox::getCapsuleSegment(Vec3 &a, Vec3 &b) const
{
	const Vec3 c = node->getWorldPosition();

	if (getShape() == Shape::Sphere)
	{
		a = c;
		b = c;
		return;
	}

	const float h = max(capsuleHeight.get(), 0.0f);
	if (h <= 0.0f)
	{
		a = c;
		b = c;
		return;
	}

	// Axis = chosen local axis (X/Y/Z) in world space, normalised so
	// radius/height stay in world-space meters even under uniform scale.
	const Mat4 t = node->getWorldTransform();
	Vec3 axis;
	switch ((int)capsuleAxis)
	{
		case 0:  axis = t.getAxisX(); break;
		case 1:  axis = t.getAxisY(); break;
		default: axis = t.getAxisZ(); break;
	}
	const float al = (float)length(axis);
	if (al < 1e-6f)
	{
		a = c;
		b = c;
		return;
	}
	axis /= Scalar(al);

	a = c - axis * Scalar(h * 0.5f);
	b = c + axis * Scalar(h * 0.5f);
}

Entity *Hurtbox::getEntity()
{
	if (_entity_resolved)
		return _entity;

	_entity = getComponentInParent<Entity>(node);
	_entity_resolved = true;
	return _entity;
}
