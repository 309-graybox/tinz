#pragma once
#include <UnigineHashMap.h>
#include <UnigineString.h>
#include <UnigineVector.h>
#include <UnigineInput.h>

class EIKey;

class EIKey
{
public:
	enum class TYPE
	{
		INVALID = 0,
		MOUSE_BUTTON = 1 << 30,
		MOUSE_AXIS = (1 << 29) | (1 << 31),
		KEYBOARD_KEY = 1 << 29,
		GAMEPAD_BUTTON = 1 << 28,
		GAMEPAD_AXIS = (1 << 27) | (1 << 31)
	};
	enum MOUSE_AXIS
	{
		MOUSE_AXIS_X,
		MOUSE_AXIS_Y,
		MOUSE_AXIS_SCROLL_X,
		MOUSE_AXIS_SCROLL_Y
	};

	static constexpr int kTypeMask = 0xff000000;

	EIKey(int k = 0)
		: _k(k)
	{
	}
	EIKey(const char *name)
		: _k()
	{
		auto it = _stringToKey.find(name);
		if (it != _stringToKey.end())
			_k = it->data._k;
	}

	Unigine::String getName() const noexcept
	{
		static Unigine::String unknown = "Unknown";
		auto it = _keyToString.find(_k);
		return it != _keyToString.end() ? it->data : unknown;
	}

	TYPE getType() const noexcept { return (TYPE)(_k & kTypeMask); }
	int getNativeValue() const noexcept { return _k & ~kTypeMask; }
	int getPlainValue() const noexcept { return _k; }

	bool isValid() const noexcept { return _keys.contains(_k); }
	bool isMouseButton() const noexcept { return getType() == TYPE::MOUSE_BUTTON; }
	bool isMouseAxis() const noexcept { return getType() == TYPE::MOUSE_AXIS; }
	bool isKeyboardKey() const noexcept { return getType() == TYPE::KEYBOARD_KEY; }
	bool isGamepadButton() const noexcept { return getType() == TYPE::GAMEPAD_BUTTON; }
	bool isGamepadAxis() const noexcept { return getType() == TYPE::GAMEPAD_AXIS; }

	bool isAxis() const noexcept { return _k & (1 << 31); }

	float getValue(int device = 0) const noexcept
	{
		using namespace Unigine;

		auto value = getNativeValue();
		switch (getType())
		{
			case TYPE::MOUSE_BUTTON:
				return Input::isMouseButtonPressed((Input::MOUSE_BUTTON)value);
			case TYPE::MOUSE_AXIS:
			{
				switch (value)
				{
					case MOUSE_AXIS_X:
						return Input::getMouseDeltaPosition().x;
					case MOUSE_AXIS_Y:
						return Input::getMouseDeltaPosition().y;
					case MOUSE_AXIS_SCROLL_X:
						return Input::getMouseWheelHorizontal();
					case MOUSE_AXIS_SCROLL_Y:
						return Input::getMouseWheel();
					default:
						// TODO
						// UNIGINE_ASSERT(false && "Imposible MOUSE_AXIS");
						return 0.0f;
				}
			}
			case TYPE::KEYBOARD_KEY:
				return Input::isKeyPressed((Input::KEY)value);
			case TYPE::GAMEPAD_BUTTON:
			{
				auto gamepad = Input::getGamePad(device);
				return gamepad ? gamepad->isButtonPressed((Input::GAMEPAD_BUTTON)value) : 0.0f;
			}
			case TYPE::GAMEPAD_AXIS:
			{
				auto gamepad = Input::getGamePad(device);
				switch (value)
				{
					case Input::GAMEPAD_AXIS::GAMEPAD_AXIS_LEFT_X:
						return gamepad->getAxesLeft().x;
					case Input::GAMEPAD_AXIS::GAMEPAD_AXIS_LEFT_Y:
						return gamepad->getAxesLeft().y;
					case Input::GAMEPAD_AXIS::GAMEPAD_AXIS_RIGHT_X:
						return gamepad->getAxesRight().x;
					case Input::GAMEPAD_AXIS::GAMEPAD_AXIS_RIGHT_Y:
						return gamepad->getAxesRight().y;
					case Input::GAMEPAD_AXIS::GAMEPAD_AXIS_LEFT_TRIGGER:
						return gamepad->getTriggerLeft();
					case Input::GAMEPAD_AXIS::GAMEPAD_AXIS_RIGHT_TRIGGER:
						return gamepad->getTriggerRight();
					default:
						// TODO
						// UNIGINE_ASSERT(false && "Imposible GAMEPAD_AXIS");
						return 0.0f;
				}
			}
			default:
				// TODO
				// UNIGINE_ASSERT(false && "Imposible TYPE");
				return 0.0f;
		}
	}

	friend bool operator==(const EIKey &a, const EIKey &b) { return a._k == b._k; }
	friend bool operator!=(const EIKey &a, const EIKey &b) { return a._k != b._k; }
	friend bool operator>(const EIKey &a, const EIKey &b) { return a._k > b._k; }
	friend bool operator>=(const EIKey &a, const EIKey &b) { return a._k >= b._k; }
	friend bool operator<(const EIKey &a, const EIKey &b) { return a._k < b._k; }
	friend bool operator<=(const EIKey &a, const EIKey &b) { return a._k <= b._k; }

	static const Unigine::HashMap<Unigine::String, EIKey> &getStringToKeyMap() noexcept
	{
		return _stringToKey;
	}

	static const Unigine::HashMap<int, Unigine::String> &getKeyToStringMap() noexcept
	{
		return _keyToString;
	}

	static const Unigine::Vector<EIKey> &getKeys() noexcept { return _keys; }
	static const Unigine::Vector<Unigine::String> &getKeysNames() noexcept { return _names; }

private:
	friend EIKey createKey(int v, const char *name);

private:
	int _k;
	static inline Unigine::HashMap<Unigine::String, EIKey> _stringToKey = {};
	static inline Unigine::HashMap<int, Unigine::String> _keyToString = {};
	static inline Unigine::Vector<EIKey> _keys = {};
	static inline Unigine::Vector<Unigine::String> _names = {};
};

inline EIKey createKey(int v, const char *name)
{
	EIKey::_stringToKey[name] = v;
	EIKey::_keyToString[v] = name;
	EIKey::_keys.append(v);
	EIKey::_names.append(name);
	return EIKey(v);
}

inline EIKey makeKey(Unigine::Input::MOUSE_BUTTON v, const char *name)
{
	return createKey(v | (int)EIKey::TYPE::MOUSE_BUTTON, name);
}
inline EIKey makeKey(EIKey::MOUSE_AXIS v, const char *name)
{
	return createKey(v | (int)EIKey::TYPE::MOUSE_AXIS, name);
}
inline EIKey makeKey(Unigine::Input::KEY v, const char *name)
{
	return createKey(v | (int)EIKey::TYPE::KEYBOARD_KEY, name);
}
inline EIKey makeKey(Unigine::Input::GAMEPAD_BUTTON v, const char *name)
{
	return createKey(v | (int)EIKey::TYPE::GAMEPAD_BUTTON, name);
}
inline EIKey makeKey(Unigine::Input::GAMEPAD_AXIS v, const char *name)
{
	return createKey(v | (int)EIKey::TYPE::GAMEPAD_AXIS, name);
}

namespace Key
{
static inline const EIKey Invalid = createKey(0, "Invalid");
static inline const EIKey MouseLeft = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_LEFT, "MouseLeft");
static inline const EIKey MouseMiddle = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_MIDDLE, "MouseMiddle");
static inline const EIKey MouseRight = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_RIGHT, "MouseRight");
static inline const EIKey MouseDclick = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_DCLICK, "MouseDclick");
static inline const EIKey MouseAux0 = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_AUX_0, "MouseAux0");
static inline const EIKey MouseAux1 = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_AUX_1, "MouseAux1");
static inline const EIKey MouseAux2 = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_AUX_2, "MouseAux2");
static inline const EIKey MouseAux3 = makeKey(Unigine::Input::MOUSE_BUTTON::MOUSE_BUTTON_AUX_3, "MouseAux3");
static inline const EIKey MouseX = makeKey(EIKey::MOUSE_AXIS::MOUSE_AXIS_X, "MouseX");
static inline const EIKey MouseY = makeKey(EIKey::MOUSE_AXIS::MOUSE_AXIS_Y, "MouseY");
static inline const EIKey MouseScrollX = makeKey(EIKey::MOUSE_AXIS::MOUSE_AXIS_SCROLL_X, "MouseScrollX");
static inline const EIKey MouseScrollY = makeKey(EIKey::MOUSE_AXIS::MOUSE_AXIS_SCROLL_Y, "MouseScrollY");
static inline const EIKey Esc = makeKey(Unigine::Input::KEY::KEY_ESC, "Esc");
static inline const EIKey F1 = makeKey(Unigine::Input::KEY::KEY_F1, "F1");
static inline const EIKey F2 = makeKey(Unigine::Input::KEY::KEY_F2, "F2");
static inline const EIKey F3 = makeKey(Unigine::Input::KEY::KEY_F3, "F3");
static inline const EIKey F4 = makeKey(Unigine::Input::KEY::KEY_F4, "F4");
static inline const EIKey F5 = makeKey(Unigine::Input::KEY::KEY_F5, "F5");
static inline const EIKey F6 = makeKey(Unigine::Input::KEY::KEY_F6, "F6");
static inline const EIKey F7 = makeKey(Unigine::Input::KEY::KEY_F7, "F7");
static inline const EIKey F8 = makeKey(Unigine::Input::KEY::KEY_F8, "F8");
static inline const EIKey F9 = makeKey(Unigine::Input::KEY::KEY_F9, "F9");
static inline const EIKey F10 = makeKey(Unigine::Input::KEY::KEY_F10, "F10");
static inline const EIKey F11 = makeKey(Unigine::Input::KEY::KEY_F11, "F11");
static inline const EIKey F12 = makeKey(Unigine::Input::KEY::KEY_F12, "F12");
static inline const EIKey Printscreen = makeKey(Unigine::Input::KEY::KEY_PRINTSCREEN, "Printscreen");
static inline const EIKey ScrollLock = makeKey(Unigine::Input::KEY::KEY_SCROLL_LOCK, "ScrollLock");
static inline const EIKey Pause = makeKey(Unigine::Input::KEY::KEY_PAUSE, "Pause");
static inline const EIKey BackQuote = makeKey(Unigine::Input::KEY::KEY_BACK_QUOTE, "BackQuote");
static inline const EIKey D1 = makeKey(Unigine::Input::KEY::KEY_DIGIT_1, "D1");
static inline const EIKey D2 = makeKey(Unigine::Input::KEY::KEY_DIGIT_2, "D2");
static inline const EIKey D3 = makeKey(Unigine::Input::KEY::KEY_DIGIT_3, "D3");
static inline const EIKey D4 = makeKey(Unigine::Input::KEY::KEY_DIGIT_4, "D4");
static inline const EIKey D5 = makeKey(Unigine::Input::KEY::KEY_DIGIT_5, "D5");
static inline const EIKey D6 = makeKey(Unigine::Input::KEY::KEY_DIGIT_6, "D6");
static inline const EIKey D7 = makeKey(Unigine::Input::KEY::KEY_DIGIT_7, "D7");
static inline const EIKey D8 = makeKey(Unigine::Input::KEY::KEY_DIGIT_8, "D8");
static inline const EIKey D9 = makeKey(Unigine::Input::KEY::KEY_DIGIT_9, "D9");
static inline const EIKey D0 = makeKey(Unigine::Input::KEY::KEY_DIGIT_0, "D0");
static inline const EIKey Minus = makeKey(Unigine::Input::KEY::KEY_MINUS, "Minus");
static inline const EIKey Equals = makeKey(Unigine::Input::KEY::KEY_EQUALS, "Equals");
static inline const EIKey Backspace = makeKey(Unigine::Input::KEY::KEY_BACKSPACE, "Backspace");
static inline const EIKey Tab = makeKey(Unigine::Input::KEY::KEY_TAB, "Tab");
static inline const EIKey Q = makeKey(Unigine::Input::KEY::KEY_Q, "Q");
static inline const EIKey W = makeKey(Unigine::Input::KEY::KEY_W, "W");
static inline const EIKey E = makeKey(Unigine::Input::KEY::KEY_E, "E");
static inline const EIKey R = makeKey(Unigine::Input::KEY::KEY_R, "R");
static inline const EIKey T = makeKey(Unigine::Input::KEY::KEY_T, "T");
static inline const EIKey Y = makeKey(Unigine::Input::KEY::KEY_Y, "Y");
static inline const EIKey U = makeKey(Unigine::Input::KEY::KEY_U, "U");
static inline const EIKey I = makeKey(Unigine::Input::KEY::KEY_I, "I");
static inline const EIKey O = makeKey(Unigine::Input::KEY::KEY_O, "O");
static inline const EIKey P = makeKey(Unigine::Input::KEY::KEY_P, "P");
static inline const EIKey LeftBracket = makeKey(Unigine::Input::KEY::KEY_LEFT_BRACKET, "LeftBracket");
static inline const EIKey RightBracket = makeKey(Unigine::Input::KEY::KEY_RIGHT_BRACKET, "RightBracket");
static inline const EIKey Enter = makeKey(Unigine::Input::KEY::KEY_ENTER, "Enter");
static inline const EIKey CapsLock = makeKey(Unigine::Input::KEY::KEY_CAPS_LOCK, "CapsLock");
static inline const EIKey A = makeKey(Unigine::Input::KEY::KEY_A, "A");
static inline const EIKey S = makeKey(Unigine::Input::KEY::KEY_S, "S");
static inline const EIKey D = makeKey(Unigine::Input::KEY::KEY_D, "D");
static inline const EIKey F = makeKey(Unigine::Input::KEY::KEY_F, "F");
static inline const EIKey G = makeKey(Unigine::Input::KEY::KEY_G, "G");
static inline const EIKey H = makeKey(Unigine::Input::KEY::KEY_H, "H");
static inline const EIKey J = makeKey(Unigine::Input::KEY::KEY_J, "J");
static inline const EIKey K = makeKey(Unigine::Input::KEY::KEY_K, "K");
static inline const EIKey L = makeKey(Unigine::Input::KEY::KEY_L, "L");
static inline const EIKey Semicolon = makeKey(Unigine::Input::KEY::KEY_SEMICOLON, "Semicolon");
static inline const EIKey Quote = makeKey(Unigine::Input::KEY::KEY_QUOTE, "Quote");
static inline const EIKey BackSlash = makeKey(Unigine::Input::KEY::KEY_BACK_SLASH, "BackSlash");
static inline const EIKey LeftShift = makeKey(Unigine::Input::KEY::KEY_LEFT_SHIFT, "LeftShift");
static inline const EIKey Less = makeKey(Unigine::Input::KEY::KEY_LESS, "Less");
static inline const EIKey Z = makeKey(Unigine::Input::KEY::KEY_Z, "Z");
static inline const EIKey X = makeKey(Unigine::Input::KEY::KEY_X, "X");
static inline const EIKey C = makeKey(Unigine::Input::KEY::KEY_C, "C");
static inline const EIKey V = makeKey(Unigine::Input::KEY::KEY_V, "V");
static inline const EIKey B = makeKey(Unigine::Input::KEY::KEY_B, "B");
static inline const EIKey N = makeKey(Unigine::Input::KEY::KEY_N, "N");
static inline const EIKey M = makeKey(Unigine::Input::KEY::KEY_M, "M");
static inline const EIKey Comma = makeKey(Unigine::Input::KEY::KEY_COMMA, "Comma");
static inline const EIKey Dot = makeKey(Unigine::Input::KEY::KEY_DOT, "Dot");
static inline const EIKey Slash = makeKey(Unigine::Input::KEY::KEY_SLASH, "Slash");
static inline const EIKey RightShift = makeKey(Unigine::Input::KEY::KEY_RIGHT_SHIFT, "RightShift");
static inline const EIKey LeftCtrl = makeKey(Unigine::Input::KEY::KEY_LEFT_CTRL, "LeftCtrl");
static inline const EIKey LeftCmd = makeKey(Unigine::Input::KEY::KEY_LEFT_CMD, "LeftCmd");
static inline const EIKey LeftAlt = makeKey(Unigine::Input::KEY::KEY_LEFT_ALT, "LeftAlt");
static inline const EIKey Space = makeKey(Unigine::Input::KEY::KEY_SPACE, "Space");
static inline const EIKey RightAlt = makeKey(Unigine::Input::KEY::KEY_RIGHT_ALT, "RightAlt");
static inline const EIKey RightCmd = makeKey(Unigine::Input::KEY::KEY_RIGHT_CMD, "RightCmd");
static inline const EIKey Menu = makeKey(Unigine::Input::KEY::KEY_MENU, "Menu");
static inline const EIKey RightCtrl = makeKey(Unigine::Input::KEY::KEY_RIGHT_CTRL, "RightCtrl");
static inline const EIKey Insert = makeKey(Unigine::Input::KEY::KEY_INSERT, "Insert");
static inline const EIKey Delete = makeKey(Unigine::Input::KEY::KEY_DELETE, "Delete");
static inline const EIKey Home = makeKey(Unigine::Input::KEY::KEY_HOME, "Home");
static inline const EIKey End = makeKey(Unigine::Input::KEY::KEY_END, "End");
static inline const EIKey Pgup = makeKey(Unigine::Input::KEY::KEY_PGUP, "Pgup");
static inline const EIKey Pgdown = makeKey(Unigine::Input::KEY::KEY_PGDOWN, "Pgdown");
static inline const EIKey Up = makeKey(Unigine::Input::KEY::KEY_UP, "Up");
static inline const EIKey Left = makeKey(Unigine::Input::KEY::KEY_LEFT, "Left");
static inline const EIKey Down = makeKey(Unigine::Input::KEY::KEY_DOWN, "Down");
static inline const EIKey Right = makeKey(Unigine::Input::KEY::KEY_RIGHT, "Right");
static inline const EIKey NumLock = makeKey(Unigine::Input::KEY::KEY_NUM_LOCK, "NumLock");
static inline const EIKey NumpadDivide = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIVIDE, "NumpadDivide");
static inline const EIKey NumpadMultiply = makeKey(Unigine::Input::KEY::KEY_NUMPAD_MULTIPLY, "NumpadMultiply");
static inline const EIKey NumpadMinus = makeKey(Unigine::Input::KEY::KEY_NUMPAD_MINUS, "NumpadMinus");
static inline const EIKey NumpadD7 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_7, "NumpadD7");
static inline const EIKey NumpadD8 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_8, "NumpadD8");
static inline const EIKey NumpadD9 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_9, "NumpadD9");
static inline const EIKey NumpadPlus = makeKey(Unigine::Input::KEY::KEY_NUMPAD_PLUS, "NumpadPlus");
static inline const EIKey NumpadD4 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_4, "NumpadD4");
static inline const EIKey NumpadD5 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_5, "NumpadD5");
static inline const EIKey NumpadD6 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_6, "NumpadD6");
static inline const EIKey NumpadD1 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_1, "NumpadD1");
static inline const EIKey NumpadD2 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_2, "NumpadD2");
static inline const EIKey NumpadD3 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_3, "NumpadD3");
static inline const EIKey NumpadEnter = makeKey(Unigine::Input::KEY::KEY_NUMPAD_ENTER, "NumpadEnter");
static inline const EIKey NumpadD0 = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DIGIT_0, "NumpadD0");
static inline const EIKey NumpadDot = makeKey(Unigine::Input::KEY::KEY_NUMPAD_DOT, "NumpadDot");
static inline const EIKey GamepadA = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_A, "GamepadA");
static inline const EIKey GamepadB = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_B, "GamepadB");
static inline const EIKey GamepadX = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_X, "GamepadX");
static inline const EIKey GamepadY = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_Y, "GamepadY");
static inline const EIKey GamepadBack = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_BACK, "GamepadBack");
static inline const EIKey GamepadStart = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_START, "GamepadStart");
static inline const EIKey GamepadDpadUp = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_DPAD_UP, "GamepadDpadUp");
static inline const EIKey GamepadDpadDown = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_DPAD_DOWN, "GamepadDpadDown");
static inline const EIKey GamepadDpadLeft = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_DPAD_LEFT, "GamepadDpadLeft");
static inline const EIKey GamepadDpadRight = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_DPAD_RIGHT,
	"GamepadDpadRight");
static inline const EIKey GamepadThumbLeft = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_THUMB_LEFT,
	"GamepadThumbLeft");
static inline const EIKey GamepadThumbRight = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_THUMB_RIGHT,
	"GamepadThumbRight");
static inline const EIKey GamepadShoulderLeft = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_SHOULDER_LEFT,
	"GamepadShoulderLeft");
static inline const EIKey GamepadShoulderRight = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_SHOULDER_RIGHT,
	"GamepadShoulderRight");
static inline const EIKey GamepadGuide = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_GUIDE, "GamepadGuide");
static inline const EIKey GamepadMisc1 = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_MISC1, "GamepadMisc1");
static inline const EIKey GamepadTouchpad = makeKey(Unigine::Input::GAMEPAD_BUTTON::GAMEPAD_BUTTON_TOUCHPAD, "GamepadTouchpad");
static inline const EIKey GamepadLeftX = makeKey(Unigine::Input::GAMEPAD_AXIS::GAMEPAD_AXIS_LEFT_X, "GamepadLeftX");
static inline const EIKey GamepadLeftY = makeKey(Unigine::Input::GAMEPAD_AXIS::GAMEPAD_AXIS_LEFT_Y, "GamepadLeftY");
static inline const EIKey GamepadRightX = makeKey(Unigine::Input::GAMEPAD_AXIS::GAMEPAD_AXIS_RIGHT_X, "GamepadRightX");
static inline const EIKey GamepadRightY = makeKey(Unigine::Input::GAMEPAD_AXIS::GAMEPAD_AXIS_RIGHT_Y, "GamepadRightY");
static inline const EIKey GamepadLeftTrigger = makeKey(Unigine::Input::GAMEPAD_AXIS::GAMEPAD_AXIS_LEFT_TRIGGER,
	"GamepadLeftTrigger");
static inline const EIKey GamepadRightTrigger = makeKey(Unigine::Input::GAMEPAD_AXIS::GAMEPAD_AXIS_RIGHT_TRIGGER,
	"GamepadRightTrigger");
} // namespace Key
