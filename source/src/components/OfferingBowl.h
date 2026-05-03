#pragma once

#include "Interactable.h"

class Inventory;

struct BowlRequirementInfo: Unigine::ComponentStruct
{
	PROP_PARAM(String, typeId)
	PROP_PARAM(Int, requiredCount, 1)
};

struct BowlVisualNodeInfo: Unigine::ComponentStruct
{
	PROP_PARAM(String, typeId)
	PROP_PARAM(File, sourceNode)
};

class OfferingBowl: public Interactable
{
public:
	COMPONENT_DEFINE(OfferingBowl, Interactable)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_ARRAY_STRUCT(BowlRequirementInfo, requirements)
	PROP_ARRAY_STRUCT(BowlVisualNodeInfo, visuals)

	PROP_PARAM(Float, transferInterval, 0.12f, "", "Seconds between draining one item")
	PROP_PARAM(Float, flightDuration, 0.45f, "", "Seconds for one visual node to fly into the bowl")
	PROP_PARAM(Float, arcHeight, 1.4f, "", "Arc height of flying visual nodes")
	PROP_PARAM(Float, arrivalRadius, 0.25f, "", "Random radius around bowl center for visual arrivals")

	PROP_PARAM(Toggle, requireVisualNode, false, "", "If true, item is drained only when matching visual node exists")
	PROP_PARAM(Toggle, lockWhenFilled, true, "", "Disable interactions when all requirements are satisfied")

	PROP_PARAM(String, soundDrain, "", "Drain Sound", "SoundManager event id or direct audio path")
	PROP_PARAM(String, soundFilled, "", "Filled Sound", "SoundManager event id or direct audio path")

	bool canInteract(const Unigine::NodePtr &interactor) const override;
	bool isFilled() const noexcept;
	float getFillProgress01() const noexcept;

protected:
	void onInteract(const Unigine::NodePtr &interactor) override;

private:
	struct RequirementState
	{
		Unigine::String type_id;
		int required = 0;
		int deposited = 0;
	};

	struct VisualState
	{
		Unigine::String type_id;
		Unigine::NodePtr node;
		bool used = false;
	};

	struct FlightState
	{
		Unigine::NodePtr node;
		Unigine::Math::Vec3 start = Unigine::Math::Vec3_zero;
		Unigine::Math::Vec3 end = Unigine::Math::Vec3_zero;
		float duration = 0.0f;
		float timer = 0.0f;
	};

	void init();
	void update();

	Inventory *resolveInventory(const Unigine::NodePtr &interactor) const;
	bool hasAnyTransferable(const Inventory *inventory) const;
	int pickRequirementToDrain(const Inventory *inventory) const;
	bool drainOne(Inventory *inventory);

	int findFreeVisual(const char *type_id) const;
	void launchVisual(int visual_index);
	void updateFlights(float dt);
	void stopDraining();

	int getTotalRequired() const;
	int getTotalDeposited() const;

private:
	Unigine::Vector<RequirementState> _requirements_state;
	Unigine::Vector<VisualState> _visual_state;
	Unigine::Vector<FlightState> _flights;

	Unigine::NodePtr _drain_source;
	float _drain_timer = 0.0f;
	bool _draining = false;
	bool _was_filled = false;
};
