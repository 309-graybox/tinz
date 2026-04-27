#pragma once

class MovementContext;

// If the state changes, you need to call init on the next state before returning its index.
enum MovementStateIndex : int {
	NONE = -1,
	IDLE = 0,
	MOVE,
	TURN,
	COUNT
};

class MovementState
{
public:
	virtual ~MovementState() = default;
	virtual void onEnter(MovementContext&) {};
	virtual void onExit(MovementContext&) {};
	virtual MovementStateIndex update(MovementContext &, float ifps) = 0;
	virtual bool canJump() const { return true; }
	virtual const char *name() const = 0;
};