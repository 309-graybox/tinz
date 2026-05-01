#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineMaterial.h>
#include <UnigineMathLib.h>
#include <UnigineVector.h>


// Base for hover-detectable, outline-able diegetic menu components.
// Subclasses override onInit/onUpdate hooks; init/update themselves are owned
// by this base so the outline+hover machinery is consistent.
class MenuInteractive : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(MenuInteractive, Unigine::ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

	PROP_GROUP("Hover")
	PROP_PARAM(String, hoverSound, "", "Hover Sound")

	bool isHovered() const noexcept { return _hovered; }
	const Unigine::NodePtr &getNode() const noexcept { return node; }

	virtual void setHovered(bool on);
	void setOutlineEnabled(bool on);

protected:
	virtual void onInit() {}
	virtual void onUpdate() {}

	Unigine::Math::Vec3 _rest_pos = Unigine::Math::Vec3_zero;

private:
	struct SurfaceMat
	{
		Unigine::MaterialPtr mat;
		int aux_state_idx = -1;
	};

	void init();
	void update();

	void collect_surfaces(const Unigine::NodePtr &n);

	Unigine::Vector<SurfaceMat> _surfaces;
	bool _hovered = false;
};

