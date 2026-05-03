#pragma once

#include "Interactable.h"
#include <UnigineMaterial.h>
#include <UnigineWidgets.h>
#include <UnigineGui.h>
#include <UnigineWidgets.h>

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
	COMPONENT_SHUTDOWN(shutdown);

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

	PROP_PARAM(Node, playerEnd, "final");
	PROP_PARAM(Node, head) 
	PROP_PARAM(File, black);
	PROP_PARAM(Float, blackscreen_time, 5.0f)
	PROP_PARAM(Float, eye_time, 5.0f);
	PROP_PARAM(File, sound_final);
	PROP_PARAM(File, font);
	PROP_PARAM(Int, fontSize, 24)

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
	void shutdown();

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
	int getRequiredByType(const char *type_id) const;
	int getDepositedByType(const char *type_id) const;
	void ensureSoulProgressUi();
	void updateSoulProgressUi();
	void shutdownSoulProgressUi();

	void end(float ifps);
	void triggerEnd();

private:
	Unigine::Vector<RequirementState> _requirements_state;
	Unigine::Vector<VisualState> _visual_state;
	Unigine::Vector<FlightState> _flights;

	Unigine::NodePtr _drain_source;
	float _drain_timer = 0.0f;
	bool _draining = false;
	bool _was_filled = false;
	bool _end = false;
	bool _thanks = false;

	bool _blackscreen = false;
	Unigine::MaterialPtr _mat;
	Unigine::WidgetPtr _label;
	int _emission = -1;
	float _eye_timer = 0;
	Unigine::Math::vec4 _color;
	bool _has_soul_requirement = false;

	Unigine::GuiPtr _gui;
	Unigine::WidgetLabelPtr _soul_progress_label;
};
