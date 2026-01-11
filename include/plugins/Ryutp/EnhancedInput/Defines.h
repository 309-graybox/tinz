#pragma once
#include <UnigineString.h>

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
#define FOR_EACH_17(what, x, ...) what(x) EXPAND(FOR_EACH_16(what, __VA_ARGS__))
#define FOR_EACH_18(what, x, ...) what(x) EXPAND(FOR_EACH_17(what, __VA_ARGS__))
#define FOR_EACH_19(what, x, ...) what(x) EXPAND(FOR_EACH_18(what, __VA_ARGS__))
#define FOR_EACH_20(what, x, ...) what(x) EXPAND(FOR_EACH_19(what, __VA_ARGS__))
#define FOR_EACH_21(what, x, ...) what(x) EXPAND(FOR_EACH_20(what, __VA_ARGS__))
#define FOR_EACH_22(what, x, ...) what(x) EXPAND(FOR_EACH_21(what, __VA_ARGS__))
#define FOR_EACH_23(what, x, ...) what(x) EXPAND(FOR_EACH_22(what, __VA_ARGS__))
#define FOR_EACH_24(what, x, ...) what(x) EXPAND(FOR_EACH_23(what, __VA_ARGS__))
#define FOR_EACH_25(what, x, ...) what(x) EXPAND(FOR_EACH_24(what, __VA_ARGS__))
#define FOR_EACH_26(what, x, ...) what(x) EXPAND(FOR_EACH_25(what, __VA_ARGS__))
#define FOR_EACH_27(what, x, ...) what(x) EXPAND(FOR_EACH_26(what, __VA_ARGS__))
#define FOR_EACH_28(what, x, ...) what(x) EXPAND(FOR_EACH_27(what, __VA_ARGS__))
#define FOR_EACH_29(what, x, ...) what(x) EXPAND(FOR_EACH_28(what, __VA_ARGS__))
#define FOR_EACH_30(what, x, ...) what(x) EXPAND(FOR_EACH_29(what, __VA_ARGS__))
#define FOR_EACH_31(what, x, ...) what(x) EXPAND(FOR_EACH_30(what, __VA_ARGS__))
#define FOR_EACH_32(what, x, ...) what(x) EXPAND(FOR_EACH_31(what, __VA_ARGS__))
#define FOR_EACH_33(what, x, ...) what(x) EXPAND(FOR_EACH_32(what, __VA_ARGS__))
#define FOR_EACH_34(what, x, ...) what(x) EXPAND(FOR_EACH_33(what, __VA_ARGS__))
#define FOR_EACH_35(what, x, ...) what(x) EXPAND(FOR_EACH_34(what, __VA_ARGS__))
#define FOR_EACH_36(what, x, ...) what(x) EXPAND(FOR_EACH_35(what, __VA_ARGS__))
#define FOR_EACH_37(what, x, ...) what(x) EXPAND(FOR_EACH_36(what, __VA_ARGS__))
#define FOR_EACH_38(what, x, ...) what(x) EXPAND(FOR_EACH_37(what, __VA_ARGS__))
#define FOR_EACH_39(what, x, ...) what(x) EXPAND(FOR_EACH_38(what, __VA_ARGS__))
#define FOR_EACH_40(what, x, ...) what(x) EXPAND(FOR_EACH_39(what, __VA_ARGS__))
#define FOR_EACH_41(what, x, ...) what(x) EXPAND(FOR_EACH_40(what, __VA_ARGS__))
#define FOR_EACH_42(what, x, ...) what(x) EXPAND(FOR_EACH_41(what, __VA_ARGS__))
#define FOR_EACH_43(what, x, ...) what(x) EXPAND(FOR_EACH_42(what, __VA_ARGS__))
#define FOR_EACH_44(what, x, ...) what(x) EXPAND(FOR_EACH_43(what, __VA_ARGS__))
#define FOR_EACH_45(what, x, ...) what(x) EXPAND(FOR_EACH_44(what, __VA_ARGS__))
#define FOR_EACH_46(what, x, ...) what(x) EXPAND(FOR_EACH_45(what, __VA_ARGS__))
#define FOR_EACH_47(what, x, ...) what(x) EXPAND(FOR_EACH_46(what, __VA_ARGS__))
#define FOR_EACH_48(what, x, ...) what(x) EXPAND(FOR_EACH_47(what, __VA_ARGS__))
#define FOR_EACH_49(what, x, ...) what(x) EXPAND(FOR_EACH_48(what, __VA_ARGS__))
#define FOR_EACH_50(what, x, ...) what(x) EXPAND(FOR_EACH_49(what, __VA_ARGS__))
#define FOR_EACH_51(what, x, ...) what(x) EXPAND(FOR_EACH_50(what, __VA_ARGS__))
#define FOR_EACH_52(what, x, ...) what(x) EXPAND(FOR_EACH_51(what, __VA_ARGS__))
#define FOR_EACH_53(what, x, ...) what(x) EXPAND(FOR_EACH_52(what, __VA_ARGS__))
#define FOR_EACH_54(what, x, ...) what(x) EXPAND(FOR_EACH_53(what, __VA_ARGS__))
#define FOR_EACH_55(what, x, ...) what(x) EXPAND(FOR_EACH_54(what, __VA_ARGS__))
#define FOR_EACH_56(what, x, ...) what(x) EXPAND(FOR_EACH_55(what, __VA_ARGS__))
#define FOR_EACH_57(what, x, ...) what(x) EXPAND(FOR_EACH_56(what, __VA_ARGS__))
#define FOR_EACH_58(what, x, ...) what(x) EXPAND(FOR_EACH_57(what, __VA_ARGS__))
#define FOR_EACH_59(what, x, ...) what(x) EXPAND(FOR_EACH_58(what, __VA_ARGS__))
#define FOR_EACH_60(what, x, ...) what(x) EXPAND(FOR_EACH_59(what, __VA_ARGS__))
#define FOR_EACH_61(what, x, ...) what(x) EXPAND(FOR_EACH_60(what, __VA_ARGS__))
#define FOR_EACH_62(what, x, ...) what(x) EXPAND(FOR_EACH_61(what, __VA_ARGS__))
#define FOR_EACH_63(what, x, ...) what(x) EXPAND(FOR_EACH_62(what, __VA_ARGS__))
#define FOR_EACH_64(what, x, ...) what(x) EXPAND(FOR_EACH_63(what, __VA_ARGS__))
#define FOR_EACH_65(what, x, ...) what(x) EXPAND(FOR_EACH_64(what, __VA_ARGS__))
#define FOR_EACH_66(what, x, ...) what(x) EXPAND(FOR_EACH_65(what, __VA_ARGS__))
#define FOR_EACH_67(what, x, ...) what(x) EXPAND(FOR_EACH_66(what, __VA_ARGS__))
#define FOR_EACH_68(what, x, ...) what(x) EXPAND(FOR_EACH_67(what, __VA_ARGS__))
#define FOR_EACH_69(what, x, ...) what(x) EXPAND(FOR_EACH_68(what, __VA_ARGS__))
#define FOR_EACH_70(what, x, ...) what(x) EXPAND(FOR_EACH_69(what, __VA_ARGS__))

#ifdef _MSC_VER // Microsoft compilers

#define GET_ARG_COUNT(...) INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(x) x
#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#else // Non-Microsoft compilers

#define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ##__VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

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
