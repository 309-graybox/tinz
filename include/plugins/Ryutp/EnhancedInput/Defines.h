#pragma once

#define FMT(...) Unigine::String::format(__VA_ARGS__)

#define STRINGIZE(arg) STRINGIZE1(arg)
#define STRINGIZE1(arg) STRINGIZE2(arg)
#define STRINGIZE2(arg) #arg

#define CONCATENATE(arg1, arg2) CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2) CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2) arg1##arg2

#define EXPAND(x) x
#define FOR_EACH_1(what, x) what(x)
#define FOR_EACH_2(what, x, ...) what(x) EXPAND(FOR_EACH_1(what, __VA_ARGS__))
#define FOR_EACH_3(what, x, ...) what(x) EXPAND(FOR_EACH_2(what, __VA_ARGS__))
#define FOR_EACH_4(what, x, ...) what(x) EXPAND(FOR_EACH_3(what, __VA_ARGS__))
#define FOR_EACH_5(what, x, ...) what(x) EXPAND(FOR_EACH_4(what, __VA_ARGS__))
#define FOR_EACH_6(what, x, ...) what(x) EXPAND(FOR_EACH_5(what, __VA_ARGS__))
#define FOR_EACH_7(what, x, ...) what(x) EXPAND(FOR_EACH_6(what, __VA_ARGS__))
#define FOR_EACH_8(what, x, ...) what(x) EXPAND(FOR_EACH_7(what, __VA_ARGS__))
#define FOR_EACH_9(what, x, ...) what(x) EXPAND(FOR_EACH_8(what, __VA_ARGS__))
#define FOR_EACH_10(what, x, ...) what(x) EXPAND(FOR_EACH_9(what, __VA_ARGS__))
#define FOR_EACH_11(what, x, ...) what(x) EXPAND(FOR_EACH_10(what, __VA_ARGS__))
#define FOR_EACH_12(what, x, ...) what(x) EXPAND(FOR_EACH_11(what, __VA_ARGS__))
#define FOR_EACH_13(what, x, ...) what(x) EXPAND(FOR_EACH_12(what, __VA_ARGS__))
#define FOR_EACH_14(what, x, ...) what(x) EXPAND(FOR_EACH_13(what, __VA_ARGS__))
#define FOR_EACH_15(what, x, ...) what(x) EXPAND(FOR_EACH_14(what, __VA_ARGS__))
#define FOR_EACH_16(what, x, ...) what(x) EXPAND(FOR_EACH_15(what, __VA_ARGS__))

#ifdef _MSC_VER // Microsoft compilers

#define GET_ARG_COUNT(...) INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(x) x
#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0_, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count

#else // Non-Microsoft compilers

#define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count

#endif

#define FOR_EACH_(N, what, x, ...) CONCATENATE(FOR_EACH_, N)(what, x, __VA_ARGS__)
#define FOR_EACH(what, x, ...) FOR_EACH_(GET_ARG_COUNT(x, __VA_ARGS__), what, x, __VA_ARGS__)

template <class T>
class Enum;


#define ENUM_IMPL(name, x, ...)                                                                       \
	template <>                                                                                       \
	class Enum<name>                                                                                  \
	{                                                                                                 \
	public:                                                                                           \
		static constexpr const char *toString(int value)                                              \
		{                                                                                             \
			switch (value)                                                                            \
			{                                                                                         \
				FOR_EACH(_ENUM_TO_STRING_CASE, x, __VA_ARGS__)                                        \
				default: return "Unknown";                                                            \
			}                                                                                         \
		}                                                                                             \
		static constexpr const char *toString(name value)                                             \
		{                                                                                             \
			switch (value)                                                                            \
			{                                                                                         \
				FOR_EACH(_ENUM_TO_STRING_CASE, x, __VA_ARGS__)                                        \
				default: return "Unknown";                                                            \
			}                                                                                         \
		}                                                                                             \
		static name fromString(const char *s)                                                         \
		{                                                                                             \
			FOR_EACH(_ENUM_FROM_STRING_IF, x, __VA_ARGS__)                                            \
			return x;                                                                                 \
		}                                                                                             \
		static constexpr const char *StringSwitch = FOR_EACH(_ENUM_TO_STRING_SWITCH, x, __VA_ARGS__); \
		static constexpr const char *StringItems[] = {FOR_EACH(_ENUM_STRING_ITEMS, x, __VA_ARGS__)};  \
		static constexpr name Items[] = {FOR_EACH(_ENUM_ITEMS, x, __VA_ARGS__)};                      \
		static constexpr int Count = sizeof(Items) / sizeof(name);                                    \
	}

#define ENUM(name, x, ...) \
	enum name              \
	{                      \
		x,                 \
		__VA_ARGS__        \
	};                     \
	ENUM_IMPL(name, x, __VA_ARGS__)

#define _ENUM_ITEMS(x) x,
#define _ENUM_STRING_ITEMS(x) #x,
#define _ENUM_TO_STRING_SWITCH(x) #x ","

#define _ENUM_TO_STRING_CASE(x) \
	case x: return STRINGIZE(x);

#define _ENUM_FROM_STRING_IF(x)       \
	if (strcmp(s, STRINGIZE(x)) == 0) \
	{                                 \
		return x;                     \
	}


#define ENUM_FLAG_IMPL(name)                                                     \
	inline name operator|(name a, name b)                                        \
	{                                                                            \
		using und_t = std::underlying_type_t<name>;                              \
		return static_cast<name>(static_cast<und_t>(a) | static_cast<und_t>(b)); \
	}                                                                            \
	inline name operator&(name a, name b)                                        \
	{                                                                            \
		using und_t = std::underlying_type_t<name>;                              \
		return static_cast<name>(static_cast<und_t>(a) & static_cast<und_t>(b)); \
	}                                                                            \
	inline name operator^(name a, name b)                                        \
	{                                                                            \
		using und_t = std::underlying_type_t<name>;                              \
		return static_cast<name>(static_cast<und_t>(a) ^ static_cast<und_t>(b)); \
	}                                                                            \
	inline name operator~(name a)                                                \
	{                                                                            \
		using und_t = std::underlying_type_t<name>;                              \
		return static_cast<name>(~static_cast<und_t>(a));                        \
	}                                                                            \
	inline name &operator|=(name &a, name b)                                     \
	{                                                                            \
		return a = a | b;                                                        \
	}                                                                            \
	inline name &operator&=(name &a, name b)                                     \
	{                                                                            \
		return a = a & b;                                                        \
	}                                                                            \
	inline name &operator^=(name &a, name b)                                     \
	{                                                                            \
		return a = a ^ b;                                                        \
	}
