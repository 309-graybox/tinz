#pragma once
#include <type_traits>

#include <UnigineHash.h>

#define BIT(X) (1 << (X))

#define UND_T(X) static_cast<std::underlying_type<decltype(X)>::type>(X)

#define DEFINE_ENUM(Enum) \
    UNIGINE_INLINE Enum operator|(Enum a, Enum b) \
    { \
        return static_cast<Enum>(UND_T(a) | UND_T(b)); \
    } \
    UNIGINE_INLINE Enum &operator|=(Enum &a, Enum b) \
    { \
        return a = a | b; \
    } \
    UNIGINE_INLINE Enum operator&(Enum a, Enum b) \
    { \
        return static_cast<Enum>(UND_T(a) & UND_T(b)); \
    } \
    UNIGINE_INLINE Enum operator&=(Enum a, Enum b) \
    { \
        return a = a & b; \
    } \
    template<> \
    struct Unigine::Hasher<Enum> \
    { \
        using und_t = std::underlying_type<Enum>::type; \
        using HashType = Hasher<und_t>::HashType; \
        UNIGINE_INLINE static HashType create(Enum v) \
        { \
            return Hasher<und_t>::create(static_cast<und_t>(v)); \
        } \
    }
