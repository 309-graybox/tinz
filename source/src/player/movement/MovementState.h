#pragma once

class MovementContext;

class MovementState
{
public:
	virtual ~MovementState() = default;
	virtual void onEnter(MovementContext&) {};
	virtual void onExit(MovementContext&) {};
	virtual MovementState *update(MovementContext &, float ifps) = 0;
	virtual const char *name() const = 0;
};