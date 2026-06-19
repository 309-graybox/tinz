#pragma once
#include "utils/PropertyParameter.h"
#include <UnigineComponentSystem.h>
#include <UnigineNodes.h>
#include <UnigineObjects.h>
#include <UnigineWorld.h>

class CharacterMovement;

class FootIK: public Unigine::ComponentBase
{
	struct Leg;
	struct Probe;

public:
	COMPONENT_DEFINE(FootIK, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_POST_UPDATE(postUpdate)

	PROPERTY(String, jointPrefix, "mixamorig:", Tooltip("Prefix prepended to the joint names below (Mixamo rigs use \"mixamorig:\")"))
	PROPERTY(String, hipsJoint, "Hips", Tooltip("Pelvis/root joint name (without prefix) — dropped to let the legs reach uneven ground"))
	PROPERTY(String, leftUpLeg, "LeftUpLeg", Tooltip("Left thigh joint name (without prefix)"))
	PROPERTY(String, leftKnee, "LeftLeg", Tooltip("Left knee joint name"))
	PROPERTY(String, leftFoot, "LeftFoot", Tooltip("Left ankle joint name"))
	PROPERTY(String, rightUpLeg, "RightUpLeg", Tooltip("Right thigh joint name"))
	PROPERTY(String, rightKnee, "RightLeg", Tooltip("Right knee joint name"))
	PROPERTY(String, rightFoot, "RightFoot", Tooltip("Right ankle joint name"))
	PROPERTY_SWITCH(forwardAxis, 3, "-Z,Z,-Y,Y,-X,X", Tooltip("Skinned-mesh axis the character faces. The knees bend toward it; if the knees bend backward, flip this"))
	PROPERTY_ND(Node, movement, Tooltip("Node with CharacterMovement (defaults to a search on this node / its parents)"))
	PROPERTY(Mask, groundMask, ~0, Tooltip("Intersection mask for the ground rays"))
	PROPERTY(Float, traceUp, 0.5f, Tooltip("How far above the ankle the ground ray starts, m"))
	PROPERTY(Float, traceDown, 0.6f, Tooltip("How far below the ankle the ground ray reaches, m"))
	PROPERTY(Float, footHeight, 0.12f, Tooltip("Ankle height above the sole; foot target = ground hit + this, m"))
	PROPERTY(Float, maxPelvisDrop, 0.5f, Tooltip("Max the pelvis may drop so a low foot can reach the ground, m"))
	PROPERTY(Toggle, activeInIdle, 1, Tooltip("Run foot IK while the character is idle"))
	PROPERTY(Toggle, activeInMove, 0, Tooltip("Run foot IK while the character is moving"))
	PROPERTY(Toggle, activeInSlide, 0, Tooltip("Run foot IK while the character is sliding"))
	PROPERTY(Float, weight, 1.0f, Tooltip("Foot IK strength [0..1]"))
	PROPERTY(Float, turnSpeed, 15.0f, Tooltip("Exponential smoothing speed of the IK (1/s). Also how fast the IK fades in/out on state changes"))

private:
	void init();
	void postUpdate();

private:
	bool stateAllowed() const;

	void resolveLeg(Leg &leg, const char *upleg, const char *knee, const char *foot);
	Probe probeLeg(const Leg &leg, const Unigine::Math::vec3 &up) const;
	void applyPelvisDrop(float drop, const Unigine::Math::vec3 &up);
	void solveLeg(Leg &leg, const Unigine::Math::Vec3 &target, float w, float ts);

private:
	struct Leg
	{
		int upleg = -1;
		int knee = -1;
		int foot = -1;
		float l1 = 0.0f;
		float l2 = 0.0f;
		Unigine::Math::quat smoothed_hip;
		Unigine::Math::quat smoothed_knee;
		bool smoothed_init = false;
		bool valid() const { return upleg >= 0 && knee >= 0 && foot >= 0; }
	};

	struct Probe
	{
		bool grounded = false;
		Unigine::Math::Vec3 foot_p;
		Unigine::Math::Vec3 target;
		float lift = 0.0f;
	};

	Unigine::NodeSkeletonPosePtr _pose;
	Unigine::ObjectMeshSkinnedPtr _skinned;
	CharacterMovement *_movement = nullptr;
	int _hips = -1;
	Leg _left;
	Leg _right;
	float _pelvis_smoothed = 0.0f;
};
