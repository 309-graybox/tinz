#pragma once
#include <UnigineComponentSystem.h>

class Ability: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Ability, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(String, description)
	PROP_PARAM(File, iconFile)

	PROP_PARAM(Float, cooldown)
	PROP_PARAM(Float, currentCooldown)

	bool isInCooldown() const;

	virtual bool canBeUsed() const { return !isInCooldown(); }

	const Unigine::ImagePtr &getIcon() const noexcept { return _icon; }

	void use();

protected:
	virtual void useImpl() {}

private:
	void init();
	void update();

private:
	Unigine::ImagePtr _icon;
};
