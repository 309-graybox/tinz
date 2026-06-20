#pragma once
#include <UnigineString.h>
#include <UnigineComponentSystem.h>

#define NO_VAR_OPT(Name) \
	struct Name          \
	{                    \
	}

#define SINGLE_VAR_OPT(Name, Type) \
	struct Name                    \
	{                              \
		Type v;                    \
		Name(Type s)               \
			: v(s)                 \
		{                          \
		}                          \
	}
#define TWO_VAR_OPT(Name, Type) \
	struct Name                 \
	{                           \
		Type v1;                \
		Type v2;                \
		Name(Type s1, Type s2)  \
			: v1(s1)            \
			, v2(s2)            \
		{                       \
		}                       \
	}

SINGLE_VAR_OPT(Title, const char *);
SINGLE_VAR_OPT(Tooltip, const char *);
SINGLE_VAR_OPT(Group, const char *);
NO_VAR_OPT(Hidden);
NO_VAR_OPT(EditorOnly);
SINGLE_VAR_OPT(Min, double);
SINGLE_VAR_OPT(Max, double);
TWO_VAR_OPT(Range, double);
NO_VAR_OPT(Log10);
NO_VAR_OPT(Expand);
NO_VAR_OPT(MinExpand);
NO_VAR_OPT(MaxExpand);
SINGLE_VAR_OPT(Filter, const char *);

#undef TWO_VAR_OPT
#undef SINGLE_VAR_OPT
#undef NO_VAR_OP

struct ParamOpts
{
	const char *title = nullptr;
	const char *tooltip = nullptr;
	const char *group = nullptr;

	// Args
	enum FLAG
	{
		HAS_MIN = 1,
		HAS_MAX = 2,
		ARG_HIDDEN = 4,
		ARG_EDITOR_ONLY = 8,

		ARG_LOG10 = 16,
		ARG_MIN_EXPAND = 32,
		ARG_MAX_EXPAND = 64,
		ARG_EXPAND = ARG_MIN_EXPAND | ARG_MAX_EXPAND,
		FLAG_MASK = ARG_LOG10 | ARG_EXPAND,
	};

	double min;
	double max;
	int8_t flags = 0;
	const char *filter = nullptr;

	void apply(Title t) { title = t.v; }
	void apply(Tooltip t) { tooltip = t.v; }
	void apply(Group t) { group = t.v; }
	void apply(Hidden t) { flags |= ARG_HIDDEN; }
	void apply(EditorOnly t) { flags |= ARG_EDITOR_ONLY; }
	void apply(Min t)
	{
		min = t.v;
		flags |= HAS_MIN;
	}
	void apply(Max t)
	{
		max = t.v;
		flags |= HAS_MAX;
	}
	void apply(Range t)
	{
		apply(Min(t.v1));
		apply(Max(t.v2));
	}
	void apply(Filter t) { filter = t.v; }
	void apply(Log10 t) { flags |= ARG_LOG10; }
	void apply(Expand t) { flags |= ARG_EXPAND; }
	void apply(MinExpand t) { flags |= ARG_MIN_EXPAND; }
	void apply(MaxExpand t) { flags |= ARG_MAX_EXPAND; }

	Unigine::String args() const
	{
		Unigine::String s;
		if (flags & ARG_HIDDEN)
			s += "hidden=1;";
		if (flags & ARG_EDITOR_ONLY)
			s += "editor_only=1;";
		if (flags & HAS_MIN)
			s += "min=" + Unigine::String::ftoa(min) + ';';
		if (flags & HAS_MAX)
			s += "max=" + Unigine::String::ftoa(max) + ';';

		if (filter && *filter)
		{
			s += filter;
			s += ';';
		}

		if (flags & FLAG_MASK)
		{
			s += "flags=";
			if (flags & ARG_LOG10)
				s += "log10,";
			if (flags & ARG_EXPAND)
			{
				s += "expand";
			} else
			{
				if (flags & ARG_MIN_EXPAND)
					s += "min_expand,";
				if (flags & ARG_MAX_EXPAND)
					s += "max_expand,";
			}

			if (s.last() == ',')
				s.remove(s.size() - 1);

			s += ';';
		}

		return s;
	}
};

namespace detail
{
inline void makeOptsImpl(ParamOpts &o)
{
}

template <class Tag, class... Tags>
inline void makeOptsImpl(ParamOpts &o, Tag tag, Tags... tags)
{
	o.apply(tag);
	makeOptsImpl(o, tags...);
}
} // namespace detail

template <class... Tags>
inline ParamOpts makeOpts(Tags... tags)
{
	ParamOpts o;
	detail::makeOptsImpl(o, tags...);
	return o;
}

#define _TAKE_PROPERTY_ARGS(...)                                      \
	[] {                                                              \
		static Unigine::String args = ::makeOpts(__VA_ARGS__).args(); \
		return args.empty() ? nullptr : args.get();                   \
	}()

#define UNWRAP_PROPERTY_OPTIONS(...)     \
	::makeOpts(__VA_ARGS__).title,       \
		::makeOpts(__VA_ARGS__).tooltip, \
		::makeOpts(__VA_ARGS__).group,   \
		_TAKE_PROPERTY_ARGS(__VA_ARGS__)

// NOTE: a DEFAULT containing commas must be wrapped in parentheses, i.e. a
// constructor call — brace-init lists do NOT shield commas from the
// preprocessor (only `()` does), so `{0,0,0.1}` gets split into separate args.
//   WRONG: PROPERTY(Vec3, offset, {0.0f, 0.0f, 0.1f})
//   RIGHT: PROPERTY(Vec3, offset, Unigine::Math::vec3(0.0f, 0.0f, 0.1f))
#define PROPERTY(TYPE, NAME, DEFAULT, ...) Unigine::ComponentVariable##TYPE NAME{this, #NAME, DEFAULT, UNWRAP_PROPERTY_OPTIONS(__VA_ARGS__)};
#define PROPERTY_ND(TYPE, NAME, ...) Unigine::ComponentVariable##TYPE NAME{this, #NAME, UNWRAP_PROPERTY_OPTIONS(__VA_ARGS__)};
#define PROPERTY_SWITCH(NAME, DEFAULT, ITEMS, ...) Unigine::ComponentVariableSwitch NAME{this, #NAME, DEFAULT, ITEMS, UNWRAP_PROPERTY_OPTIONS(__VA_ARGS__)};
// Structs/arrays take no default value; their ctor takes the struct/element
// type name. Arg order mirrors the built-in PROP_STRUCT / PROP_ARRAY macros:
//   struct: (component, type_name, name, ...)   array: (component, name, type_name, ...)
#define PROPERTY_STRUCT(TYPE, NAME, ...) Unigine::ComponentVariableStruct<TYPE> NAME{this, #TYPE, #NAME, UNWRAP_PROPERTY_OPTIONS(__VA_ARGS__)};
#define PROPERTY_ARRAY(TYPE, NAME, ...) Unigine::ComponentVariableArray<Unigine::ComponentVariable##TYPE> NAME{this, #NAME, #TYPE, UNWRAP_PROPERTY_OPTIONS(__VA_ARGS__)};
#define PROPERTY_ARRAY_STRUCT(TYPE, NAME, ...) Unigine::ComponentVariableArray<Unigine::ComponentVariableStruct<TYPE>> NAME{this, #NAME, #TYPE, UNWRAP_PROPERTY_OPTIONS(__VA_ARGS__)};
