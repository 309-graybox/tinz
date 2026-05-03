#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>

class Interactable: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Interactable, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Float, range, 3.0f)
	PROP_PARAM(Float, interactHoldTime, 0.0f) // 0 = tap, >0 = hold seconds
	PROP_PARAM(String, interactPromptText)    // UI can use this; empty = default

	bool isInteractionReady() const noexcept { return _state == State::Idle; }
	bool isInteracting() const noexcept { return _state == State::Interact; }
	bool isHovered() const noexcept { return _hovered; }
	float getInteractProgress01() const noexcept;

	// Driven by PlayerInteraction.
	void beginHover(const Unigine::NodePtr &interactor);
	void tickHover(const Unigine::NodePtr &interactor);
	void endHover(const Unigine::NodePtr &interactor);
	void startInteract(const Unigine::NodePtr &interactor);
	void cancelInteract();

	virtual bool canInteract(const Unigine::NodePtr &interactor) const;

	Unigine::EventInvoker<Unigine::NodePtr> &eventHoverStarted() noexcept { return _event_hover_started; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventHoverEnded() noexcept { return _event_hover_ended; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventInteractStarted() noexcept { return _event_interact_started; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventInteractCancelled() noexcept { return _event_interact_cancelled; }
	Unigine::EventInvoker<Unigine::NodePtr> &eventInteracted() noexcept { return _event_interacted; }
	Unigine::EventInvoker<Interactable *> &eventDestroyed() noexcept { return _event_destroyed; }

protected:
	// Placeholders for game-specific feedback and behavior.
	virtual void onHoverStarted(const Unigine::NodePtr &interactor);
	virtual void onHover(const Unigine::NodePtr &interactor);
	virtual void onHoverEnded(const Unigine::NodePtr &interactor);
	virtual void onInteractStarted(const Unigine::NodePtr &interactor);
	virtual void onInteractCancelled(const Unigine::NodePtr &interactor);
	virtual void onInteract(const Unigine::NodePtr &interactor);

	void notifyDestroyed();

private:
	enum class State
	{
		Idle,
		Interact,
	};

	void init();
	void update();
	void tickInteract(float dt);
	void completeInteract(const Unigine::NodePtr &interactor);

	State _state = State::Idle;
	bool _hovered = false;
	Unigine::NodePtr _hovered_by;
	Unigine::NodePtr _target_interactor;
	float _interact_timer = 0.0f;

	Unigine::EventInvoker<Unigine::NodePtr> _event_hover_started;
	Unigine::EventInvoker<Unigine::NodePtr> _event_hover_ended;
	Unigine::EventInvoker<Unigine::NodePtr> _event_interact_started;
	Unigine::EventInvoker<Unigine::NodePtr> _event_interact_cancelled;
	Unigine::EventInvoker<Unigine::NodePtr> _event_interacted;
	Unigine::EventInvoker<Interactable *> _event_destroyed;
};
