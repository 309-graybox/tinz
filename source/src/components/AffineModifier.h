#include <UnigineComponentSystem.h>

struct AffineStruct : public Unigine::ComponentStruct
{
	PROP_PARAM(Node, target, "Target")
	PROP_PARAM(Node, pivot, "Pivot", "Optional rotation pivot (null = target's own origin)")
	PROP_PARAM(Vec3, axis, Unigine::Math::vec3(0.0f, 0.0f, 1.0f), "Axis", "Rotation axis in pivot's local space")
	PROP_PARAM(Float, angle, 0.0f, "Angle", "Hover rotation, degrees (0 = no rotation)")
	PROP_PARAM(Vec3, offset)
	PROP_PARAM(Float, speed, 8.0f, "Speed", "Approach rate toward open pose")
	PROP_PARAM(Float, damping, 8.0f, "Damping", "Return rate to rest pose")
};

class AffineModifier: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(AffineModifier, Unigine::ComponentBase)
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

	PROP_PARAM(Node, settings, "Settings", "SoundSettings node; falls back to self, then defaults")

	PROP_ARRAY_STRUCT(AffineStruct, anims);

private:
	void init();
	void update();

public:
	void setOpen(bool open) noexcept { 
		_need_update = true;
		_open = open;
	};

private:
	struct AnimState
	{
		Unigine::Math::Vec3 rest_pos = Unigine::Math::Vec3_zero;
		Unigine::Math::quat rest_rot;
		float t = 0.0f;
	};

	void update_active_state();

	bool _open = false;
	bool _need_update = false;

	Unigine::Vector<AnimState> _anim_states;
};
