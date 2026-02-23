#pragma once
#include <UnigineMathLib.h>
#include <UnigineString.h>
#include <UnigineStreams.h>
#include <UnigineComponentSystem.h>

#include <tuple>
#include <utility>

class FormatParseContext
{
public:
	using iterator = const char *;

	constexpr FormatParseContext(const char *begin, const char *end)
		: _p(begin)
		, _e(end)
	{
		UNIGINE_ASSERT(begin <= end);
	}

	constexpr FormatParseContext(const char *begin, size_t size)
		: _p(begin)
		, _e(begin + size)
	{
	}

	constexpr FormatParseContext(const char *begin)
		: _p(begin)
		, _e(begin)
	{
		while (*_e)
			++_e;
	}

	constexpr void setBegin(iterator p) { _p = p; }

	constexpr iterator begin() const noexcept { return _p; }
	constexpr iterator end() const noexcept { return _e; }

	constexpr bool isEnd() const noexcept { return _p >= _e; }
	constexpr char current() const noexcept { return _p < _e ? *_p : EOF; }
	constexpr char peek() const noexcept { return _p + 1 < _e ? *(_p + 1) : EOF; }
	constexpr void eat() noexcept { _p = std::min(_p + 1, _e); }
	constexpr void eat(size_t n) noexcept { _p = std::min(_p + n, _e); }

	constexpr bool isCurrent(char c) const noexcept { return current() == c; }
	constexpr bool isNext(char c) const noexcept { return peek() == c; }

	constexpr void skipws()
	{
		while (current() == ' ')
		{
			eat();
		}
	}

private:
	constexpr void skipToSpec()
	{
		while (_p < _e && *_p != '{' && *_p != '}')
			++_p;
	}

private:
	const char *_p;
	const char *_e;
};

class FormatContext
{
public:
	void write(char c)
	{
		_s += c;
	}

	void write(const char *str)
	{
		_s += str;
	}

	void write(const char *str, size_t n)
	{
		_s.append(str, n);
	}

	void write(char c, size_t n)
	{
		_s.reserve(_s.size() + n);
		for (size_t i = 0; i < n; ++i)
			_s += c;
	}

	Unigine::String get() const { return _s; }

private:
	Unigine::String _s;
};

template <class T>
class Formatter;

template <class T>
class FormatArg
{
private:
	Formatter<typename std::decay<T>::type> _formatter;
	typename std::decay<T>::type _value;

public:
	constexpr FormatArg(const T &value)
		: _value(value)
	{
	}

	constexpr auto parse(FormatParseContext &ctx)
	{
		return _formatter.parse(ctx);
	}

	auto format(FormatContext &ctx)
	{
		_formatter.format(_value, ctx);
	}
};

template <typename T, size_t N>
struct StaticArray
{
	T data[N] = {};
	size_t size = 0;

	constexpr StaticArray() = default;

	constexpr void push_back(const T &value)
	{
		if (size < N)
		{
			data[size] = value;
			++size;
		}
	}

	constexpr StaticArray(const StaticArray &other)
		: size(other.size)
	{
		for (size_t i = 0; i < size; ++i)
		{
			data[i] = other.data[i];
		}
	}

	constexpr const T &operator[](size_t index) const
	{
		return data[index];
	}

	constexpr T &operator[](size_t index)
	{
		return data[index];
	}
};

enum class ChunkType
{
	Text,
	Arg
};

struct FormatChunk
{
	ChunkType type;
	size_t start;
	size_t end;

	constexpr FormatChunk()
		: type(ChunkType::Text)
		, start(0)
		, end(0)
	{
	}

	constexpr FormatChunk(ChunkType t, size_t s, size_t e)
		: type(t)
		, start(s)
		, end(e)
	{
	}
};

template <size_t N, class... Args>
class FormatResult
{
private:
	const char (&_fmt)[N];
	mutable std::tuple<FormatArg<const Args &>...> _args;
	StaticArray<FormatChunk, 256> _chunks;
	size_t _num_args;

public:
	constexpr FormatResult(const char (&fmt)[N], std::tuple<FormatArg<const Args &>...> args,
		StaticArray<FormatChunk, 256> chunks, size_t num_args)
		: _fmt(fmt)
		, _args(args)
		, _chunks(chunks)
		, _num_args(num_args)
	{
	}

	void format(FormatContext &ctx) const
	{
		for (size_t i = 0; i < _chunks.size; ++i)
		{
			const auto &chunk = _chunks[i];
			if (chunk.type == ChunkType::Text)
			{
				ctx.write(_fmt + chunk.start, chunk.end - chunk.start);
			} else
			{
				format_arg(chunk.start, ctx);
			}
		}
	}

private:
	template <size_t I>
	void format_arg_impl(FormatContext &ctx, std::integral_constant<size_t, I>) const
	{
		std::get<I>(_args).format(ctx);
	}

	void format_arg_impl(FormatContext &ctx, ...) const
	{
	}

	void format_arg(size_t index, FormatContext &ctx) const
	{
		format_arg_dispatch(index, ctx, std::index_sequence_for<Args...>{});
	}

	template <size_t... Is>
	void format_arg_dispatch(size_t index, FormatContext &ctx, std::index_sequence<Is...>) const
	{
		bool formatted = false;
		(void)std::initializer_list<bool>{
			(index == Is ? (format_arg_impl(ctx, std::integral_constant<size_t, Is>{}), formatted = true, false) : false)...};
		if (!formatted)
		{
			// Handle error: index out of bounds
		}
	}
};

template <size_t N, class... Args>
constexpr auto parse_format(const char (&fmt)[N], std::tuple<FormatArg<const Args &>...> args)
{
	FormatParseContext ctx(fmt);

	StaticArray<FormatChunk, 256> chunks;
	size_t current_pos = 0;
	size_t arg_index = 0;

	for (size_t i = 0; i < N - 1; ++i)
	{
		if (fmt[i] == '{')
		{
			if (i > current_pos)
			{
				if (chunks.size >= 256)
				{
					// Error: too many chunks
					return FormatResult<N, Args...>(fmt, args, {}, 0);
				}
				chunks.push_back(FormatChunk(ChunkType::Text, current_pos, i));
			}

			++i;
			const char *end = fmt + i + 1;
			if (fmt[i] == ':')
			{
				ctx.setBegin(fmt + i + 1);
				end = parse_arg_spec(ctx, args, arg_index);
				i = end - fmt + 1;

				if (!end || *end != '}')
				{
					// Error: invalid format spec
					return FormatResult<N, Args...>(fmt, args, {}, 0);
				}

				if (chunks.size >= 256)
				{
					// Error: too many chunks
					return FormatResult<N, Args...>(fmt, args, {}, 0);
				}

				end += 1;
			}

			current_pos = end - fmt;
			chunks.push_back(FormatChunk(ChunkType::Arg, arg_index, 0));
			arg_index++;
		}
	}

	if (current_pos < N - 1)
	{
		if (chunks.size >= 256)
		{
			return FormatResult<N, Args...>(fmt, args, {}, 0);
		}
		chunks.push_back(FormatChunk(ChunkType::Text, current_pos, N - 1));
	}

	if (arg_index != sizeof...(Args))
	{
		return FormatResult<N, Args...>(fmt, args, {}, 0);
	}

	return FormatResult<N, Args...>(fmt, args, chunks, arg_index);
}

template <class... Args>
constexpr const char *parse_arg_spec(FormatParseContext &ctx, std::tuple<FormatArg<Args>...> &args, size_t arg_index)
{
	return parse_arg_spec_impl(ctx, args, arg_index, std::index_sequence_for<Args...>{});
}

template <class... Args, size_t... Is>
constexpr const char *parse_arg_spec_impl(FormatParseContext &ctx, std::tuple<FormatArg<Args>...> &args,
	size_t arg_index, std::index_sequence<Is...>)
{
	FormatParseContext::iterator result = nullptr;
	bool parsed = false;

	(void)std::initializer_list<bool>{
		(arg_index == Is ? (result = std::get<Is>(args).parse(ctx), parsed = true, false) : false)...};

	return parsed ? result : nullptr;
}

template <size_t N, class... Args>
constexpr auto make_format(const char (&fmt)[N], const Args &...args)
{
	auto formatters = std::make_tuple(FormatArg<const Args &>(args)...);
	return parse_format(fmt, formatters);
}

template <size_t N, class... Args>
auto format(const char (&fmt)[N], const Args &...args)
{
	auto format_result = make_format(fmt, args...);
	FormatContext ctx;
	format_result.format(ctx);
	return ctx.get();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%% Formatters %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class BaseWidthFormatter
{
public:
	// [[fill]align][width]
	constexpr auto parse(FormatParseContext &ctx)
	{
		auto fmt = ctx.begin();

		if (((*fmt == '<' || *fmt == '>' || *fmt == '=') ||
				(*(fmt + 1) == '<' || *(fmt + 1) == '>' || *(fmt + 1) == '=')))
		{
			if (*(fmt + 1) == '<' || *(fmt + 1) == '>' ||
				*(fmt + 1) == '=')
			{
				fill_char = *fmt;
				++fmt;
			}

			switch (*fmt)
			{
				case '<': align = 1; break;
				case '>': align = -1; break;
				case '=': align = 0; break;
				default: return fmt;
			}
			++fmt;
		}

		if (*fmt >= '0' && *fmt <= '9')
		{
			width = 0;
			while (*fmt >= '0' && *fmt <= '9')
			{
				width = width * 10 + (*fmt - '0');
				++fmt;
			}
		}

		return fmt;
	}

protected:
	void formatString(const char *s, int len, FormatContext &ctx)
	{
		if (align <= 0 && width > len)
		{
			ctx.write(fill_char, align < 0 ? width - len : (width - len) / 2);
		}

		ctx.write(s, len);

		if (align >= 0 && width > len)
		{
			ctx.write(fill_char, align > 0 ? width - len : (width - len) / 2 + (width - len) % 2);
		}
	}

protected:
	char fill_char = ' ';
	int align = 1;
	int width = 0;
};

class BaseIntFormatter: public BaseWidthFormatter
{
public:
	// [[fill]align][width][+-(space) sign (+ always, - only -, space for +)][# alternate][d|i|x|X|o|b|B radix]
	constexpr auto parse(FormatParseContext &ctx)
	{
		auto fmt = BaseWidthFormatter::parse(ctx);

		if (*fmt == '+' || *fmt == '-' || *fmt == ' ')
		{
			sign = *fmt;
			++fmt;
		}

		if (*fmt == '#')
		{
			alternate_form = true;
			++fmt;
		}

		if (*fmt == 'd' || *fmt == 'i')
		{
			base = 10;
			++fmt;
		} else if (*fmt == 'x' || *fmt == 'X')
		{
			base = 16;
			base_upper = *fmt == 'X';
			++fmt;
		} else if (*fmt == 'o')
		{
			base = 8;
			++fmt;
		} else if (*fmt == 'b' || *fmt == 'B')
		{
			base = 2;
			base_upper = *fmt == 'B';
			++fmt;
		}

		return fmt;
	}

protected:
	template <class T>
	void formatInteger(T v, FormatContext &ctx)
	{
		char buffer[65];
		char *p = buffer + sizeof(buffer) - 1;
		*p = '\0';

		bool negative = false;
		typename std::make_unsigned<T>::type n;

		if constexpr (std::is_signed<T>::value)
		{
			negative = v < 0;
			n = negative ? -static_cast<typename std::make_unsigned<T>::type>(v)
						 : static_cast<typename std::make_unsigned<T>::type>(v);
		} else
		{
			n = v;
		}

		const auto digits = !base_upper ? "0123456789abcdef" : "0123456789ABCDEF";

		do
		{
			*--p = digits[n % base];
			n /= base;
		} while (n > 0);

		if (alternate_form)
		{
			if (base == 16)
			{
				*--p = (base_upper) ? 'X' : 'x';
				*--p = '0';
			} else if (base == 8 && *p != '0')
			{
				*--p = '0';
			} else if (base == 2)
			{
				*--p = (base == 'B') ? 'B' : 'b';
				*--p = '0';
			}
		}

		if (negative)
		{
			*--p = '-';
		} else if (sign == '+' && base == 10)
		{
			*--p = '+';
		} else if (sign == ' ' && base == 10)
		{
			*--p = ' ';
		}

		size_t len = (buffer + sizeof(buffer) - 1) - p;
		formatString(p, len, ctx);
	}

protected:
	char sign = '-';
	bool alternate_form = false;
	int base = 10;
	bool base_upper = false;
};

class BaseFloatFormatter: public BaseWidthFormatter
{
public:
	// [[fill]align][width][+-(space) sign (+ always, - only -, space for +)][# alternate][. precision][type]
	constexpr auto parse(FormatParseContext &ctx)
	{
		auto fmt = BaseWidthFormatter::parse(ctx);

		if (*fmt == '+' || *fmt == '-' || *fmt == ' ')
		{
			sign = *fmt;
			++fmt;
		}

		if (*fmt == '#')
		{
			alternate_form = true;
			++fmt;
		}

		if (*fmt == '.')
		{
			++fmt;
			precision = 0;
			while (*fmt >= '0' && *fmt <= '9')
			{
				precision = precision * 10 + (*fmt - '0');
				++fmt;
			}
		}

		if (*fmt == 'f' || *fmt == 'F' ||
			*fmt == 'e' || *fmt == 'E' ||
			*fmt == 'g' || *fmt == 'G' ||
			*fmt == 'a' || *fmt == 'A')
		{
			format_type = *fmt;
			++fmt;
		}

		return fmt;
	}

protected:
	template <class T>
	void formatFloat(T v, FormatContext &ctx)
	{
		char format_str[32];
		build_format_string(format_str, sizeof(format_str));

		char buffer[64];
		int len = snprintf(buffer, sizeof(buffer), format_str, v);

		formatString(buffer, len, ctx);
	}

	void build_format_string(char *buf, size_t size)
	{
		char *p = buf;

		*p++ = '%';

		if (format_type == 'f' || format_type == 'F')
		{
			if (sign == '+')
				*p++ = '+';
			else if (sign == ' ')
				*p++ = ' ';
		}

		if (alternate_form)
			*p++ = '#';

		if (precision >= 0 && (format_type == 'f' || format_type == 'F' ||
								  format_type == 'e' || format_type == 'E' ||
								  format_type == 'g' || format_type == 'G'))
		{
			p += sprintf(p, ".%d", precision);
		}

		*p++ = format_type;
		*p = '\0';
	}

protected:
	int precision = 6;
	char format_type = 'f';
	char sign = '-';
	bool alternate_form = false;
};

template <>
class Formatter<const char *>: public BaseWidthFormatter
{
public:
	auto format(const char *v, FormatContext &ctx)
	{
		formatString(v, strlen(v), ctx);
	}
};

template <>
class Formatter<Unigine::String>: public BaseWidthFormatter
{
public:
	auto format(const Unigine::String &v, FormatContext &ctx)
	{
		formatString(v.get(), v.size(), ctx);
	}
};

template <>
class Formatter<char>
{
public:
	constexpr auto parse(FormatParseContext &ctx)
	{
		return ctx.begin();
	}

	auto format(char v, FormatContext &ctx)
	{
		ctx.write(v);
	}
};

#define DEF_INT_MATH_FORMATTER(type)               \
	template <>                                    \
	class Formatter<type>: public BaseIntFormatter \
	{                                              \
	public:                                        \
		auto format(type v, FormatContext &ctx)    \
		{                                          \
			formatInteger(v, ctx);                 \
		}                                          \
	}

#define DEF_INT_MATH_ARR_FORMATTER(type)              \
	template <size_t N>                               \
	class Formatter<type[N]>: public BaseIntFormatter \
	{                                                 \
	public:                                           \
		auto format(type v[N], FormatContext &ctx)    \
		{                                             \
			ctx.write('[');                           \
			formatInteger(v[0], ctx);                 \
			for (size_t i = 1; i < N; ++i)            \
			{                                         \
				ctx.write("; ");                      \
				formatInteget(v[i], ctx);             \
			}                                         \
			ctx.write(']');                           \
		}                                             \
	}

DEF_INT_MATH_FORMATTER(short);
DEF_INT_MATH_FORMATTER(unsigned short);
DEF_INT_MATH_FORMATTER(int);
DEF_INT_MATH_FORMATTER(unsigned int);
DEF_INT_MATH_FORMATTER(long int);
DEF_INT_MATH_FORMATTER(unsigned long int);
DEF_INT_MATH_FORMATTER(long long int);
DEF_INT_MATH_FORMATTER(unsigned long long int);
DEF_INT_MATH_ARR_FORMATTER(short);
DEF_INT_MATH_ARR_FORMATTER(unsigned short);
DEF_INT_MATH_ARR_FORMATTER(int);
DEF_INT_MATH_ARR_FORMATTER(unsigned int);
DEF_INT_MATH_ARR_FORMATTER(long int);
DEF_INT_MATH_ARR_FORMATTER(unsigned long int);
DEF_INT_MATH_ARR_FORMATTER(long long int);
DEF_INT_MATH_ARR_FORMATTER(unsigned long long int);

template <>
class Formatter<float>: public BaseFloatFormatter
{
public:
	auto format(float v, FormatContext &ctx)
	{
		formatFloat(v, ctx);
	}
};

template <>
class Formatter<double>: public BaseFloatFormatter
{
public:
	auto format(double v, FormatContext &ctx)
	{
		formatFloat(v, ctx);
	}
};
