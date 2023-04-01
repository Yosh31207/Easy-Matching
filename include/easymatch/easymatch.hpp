/*
 *  Copyright (c) 2023 Yosh31207
 *  Distributed Under The Apache-2.0 License
 */

#ifndef EASY_MATCH_HPP_
#define EASY_MATCH_HPP_

#include <any>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <variant>

namespace easymatch {

namespace easymatch_impl {

/* utility */

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename T>
inline constexpr bool is_variant_v = false;

template<typename... Args>
inline constexpr bool is_variant_v<std::variant<Args...>> = true;

template<typename T>
inline constexpr bool is_tuple_v = false;

template<typename... Args>
inline constexpr bool is_tuple_v<std::tuple<Args...>> = true;

template<typename F, typename = void>
struct has_operator_call : std::false_type {};

template<typename F>
struct has_operator_call<F, std::void_t<decltype(&F::operator())>> : std::true_type {};

template <typename F>
inline constexpr auto has_operator_call_v = has_operator_call<F>::value;

template <typename T>
inline constexpr bool is_any_v = std::is_same_v<std::any, T>;

inline constexpr auto identity = [](auto&& x) {
    return x;
};

inline constexpr auto pass = [](auto&&) {
    return true;
};

/* types */

struct PatternStarter {};

template <typename MatchFn, typename UnwrapFn, typename HandlerFn>
struct PatternStatement {
    MatchFn condition;
    UnwrapFn unwrap;
    HandlerFn handler;
};

template <typename MatchFn, typename UnwrapFn>
struct Pattern {
    MatchFn condition;
    UnwrapFn unwrap;

    /* Pattern = Handler -> PatternStatement */
    template<typename Handler>
    constexpr auto operator=(const Handler& handler) const {
        auto handler_fn = [=](auto&& x) {
            if constexpr (std::is_invocable_v<Handler, decltype(x)>) {
                return handler(std::forward<decltype(x)>(x));
            } else if constexpr (std::is_invocable_v<Handler>) {
                return handler();
            } else if constexpr (!has_operator_call_v<Handler>) {
                return handler;
            } else if constexpr (is_tuple_v<remove_cvref_t<decltype(x)>>) {
                return std::apply(handler, std::forward<decltype(x)>(x));
            };
        };
        return PatternStatement<MatchFn, UnwrapFn, decltype(handler_fn)> {
            condition,
            unwrap,
            std::move(handler_fn)
        };
    }
};

struct Wildcard {
    /* Wildcard = Handler -> PatternStatement */
    template<typename Handler>
    constexpr auto operator=(const Handler& handler) const {
        auto handler_fn = [=](auto&& x) {
            if constexpr (std::is_invocable_v<Handler, decltype(x)>) {
                return handler(std::forward<decltype(x)>(x));
            } else if constexpr (std::is_invocable_v<Handler>) {
                return handler();
            } else if constexpr (!has_operator_call_v<Handler>) {
                return handler;
            } else if constexpr (is_tuple_v<remove_cvref_t<decltype(x)>>) {
                return std::apply(handler, std::forward<decltype(x)>(x));
            };
        };
        return PatternStatement<decltype(pass), decltype(identity), decltype(handler_fn)> {
            pass,
            identity,
            std::move(handler_fn)
        };
    }
};

template<typename T>
inline constexpr bool is_pattern_v = false;

template<typename MatchFn, typename UnwrapFn>
inline constexpr bool is_pattern_v<Pattern<MatchFn, UnwrapFn>> = true;

template<typename T>
inline constexpr bool is_wildcard_v = std::is_same_v<T, Wildcard>;

/* constants */

inline constexpr auto _ = Wildcard{};

inline constexpr auto pattern = PatternStarter{};

/* patterns */

template <typename T>
inline constexpr auto as_match_fn = [](auto&& x) {
    if constexpr (is_variant_v<remove_cvref_t<decltype(x)>>) {
        return std::holds_alternative<T>(x);
    }
    if constexpr (is_any_v<remove_cvref_t<decltype(x)>>) {
        return x.type() == typeid(T);
    }
};

template <typename T>
inline constexpr auto as_unwrap_fn = [](auto&& x) {
    if constexpr (is_variant_v<remove_cvref_t<decltype(x)>>) {
        return std::get<T>(std::forward<decltype(x)>(x));
    }
    if constexpr (is_any_v<remove_cvref_t<decltype(x)>>) {
        return std::any_cast<T>(std::forward<decltype(x)>(x));
    }
};

template <typename T>
inline constexpr auto as = Pattern<decltype(as_match_fn<T>), decltype(as_unwrap_fn<T>)> {
    as_match_fn<T>,
    as_unwrap_fn<T>
};

inline constexpr auto some_match_fn = [](auto&& x) {
    return x.has_value();
};

inline constexpr auto some_unwrap_fn = [](auto&& x) {
    return *std::forward<decltype(x)>(x);
};

inline constexpr auto some = Pattern<decltype(some_match_fn), decltype(some_unwrap_fn)> {
    some_match_fn,
    some_unwrap_fn
};

inline constexpr auto none_match_fn = [](auto&& x) {
    return !x.has_value();
};

inline constexpr auto none_unwrap_fn = [](auto&&) {
    return std::nullopt;
};

inline constexpr auto none = Pattern<decltype(none_match_fn), decltype(none_unwrap_fn)> {
    none_match_fn,
    none_unwrap_fn
};

template <typename Condition>
constexpr auto when(const Condition& cond) {
    if constexpr (is_pattern_v<Condition> || is_wildcard_v<Condition>) {
        return cond;
    } else {
        auto match_fn = [=](auto&& x) {
            if constexpr (std::is_invocable_v<Condition, decltype(x)>) {
                return cond(x);
            } else {
                return cond == x;
            }
        };
        return Pattern<decltype(match_fn), decltype(identity)> {
            std::move(match_fn),
            identity
        };
    }
}

/* Pattern | Pattern -> Pattern */

template<typename PatternT>
constexpr auto operator | (const PatternStarter&, const PatternT& pattern) {
    if constexpr (is_pattern_v<PatternT>) {
        return pattern;
    } else {
        return when(pattern);
    }
}

constexpr auto operator | (const PatternStarter&, const Wildcard& wildcard) {
    return wildcard;
}

template<typename PatternLhs, typename PatternRhs, std::enable_if_t<is_pattern_v<PatternLhs>, nullptr_t> = nullptr>
constexpr auto operator | (const PatternLhs& lhs, const PatternRhs& rhs) {
    if constexpr (is_pattern_v<PatternRhs>) {
        auto match_fn = [=](auto&& x) {
            return lhs.condition(x) && rhs.condition(lhs.unwrap(x));
        };
        auto unwrap_fn = [=](auto&& x) {
            return rhs.unwrap(lhs.unwrap(std::forward<decltype(x)>(x)));
        };
        return Pattern<decltype(match_fn), decltype(unwrap_fn)> {std::move(match_fn), std::move(unwrap_fn)};
    } else if constexpr (is_wildcard_v<PatternRhs>) {
        return lhs;
    } else {
        return lhs | when(rhs);
    }
}

/* Wildcard <op> x -> Pattern */

#define MAKE_PATTERN_WITH_WILDCARD(op)                       \
template<typename T>                                         \
constexpr auto operator op (const Wildcard&, const T& t) {   \
    auto comp = [=](auto&& x) { return x op t; };            \
    return Pattern<decltype(comp), decltype(identity)> {     \
        std::move(comp),                                     \
        identity                                             \
    };                                                       \
}                                                            \
template<typename T>                                         \
constexpr auto operator op (const T& t, const Wildcard&) {   \
    auto comp = [=](auto&& x) { return t op x; };            \
    return Pattern<decltype(comp), decltype(identity)> {     \
        std::move(comp),                                     \
        identity                                             \
    };                                                       \
}

MAKE_PATTERN_WITH_WILDCARD(==)
MAKE_PATTERN_WITH_WILDCARD(!=)
MAKE_PATTERN_WITH_WILDCARD(<)
MAKE_PATTERN_WITH_WILDCARD(>)
MAKE_PATTERN_WITH_WILDCARD(>=)
MAKE_PATTERN_WITH_WILDCARD(<=)

#undef MAKE_PATTERN_WITH_WILDCARD

/* ds(Patterns...) -> Pattern */

template<typename Value, typename PatternT>
constexpr bool ds_match(Value&& x, const PatternT& pattern) {
    if constexpr (is_pattern_v<PatternT>) {
        return pattern.condition(x);
    } else if constexpr (is_wildcard_v<PatternT>) {
        return true;
    } else if constexpr (std::is_invocable_v<PatternT, Value>) {
        return pattern(x);
    } else {
        return x == pattern;
    }
}

template<typename Value, typename PatternT>
constexpr auto ds_unwrap(Value&& x, const PatternT& pattern) {
    if constexpr (is_pattern_v<PatternT>) {
        return pattern.unwrap(x);
    } else {
        return x;
    }
}

template<typename Value, typename... Patterns, size_t... Is>
constexpr auto ds_match_fn(Value&& x, std::index_sequence<Is...>, const Patterns&... patterns) {
    return (ds_match(std::get<Is>(x), patterns) && ...);
}

template<typename Value, typename... Patterns, size_t... Is>
constexpr auto ds_unwrap_fn(Value&& x, std::index_sequence<Is...>, const Patterns&... patterns) {
    return std::make_tuple(ds_unwrap(std::get<Is>(x), patterns)...);
}

template<typename... Patterns>
constexpr auto ds(const Patterns&... patterns) {
    auto match_fn = [=](auto&& packed_x) {
        return ds_match_fn(packed_x, std::index_sequence_for<Patterns...>{}, patterns...);
    };
    auto unwrap_fn = [=](auto&& packed_x) {
        return ds_unwrap_fn(packed_x, std::index_sequence_for<Patterns...>{}, patterns...);
    };
    return Pattern<decltype(match_fn), decltype(unwrap_fn)> {
        std::move(match_fn),
        std::move(unwrap_fn)
    };
}

/* match */

template<typename Value, typename PatternStatementT>
constexpr auto match_impl(Value&& x, const PatternStatementT& ps) {
    if (!ps.condition(x)) {
        throw std::runtime_error("unmatched to all cases");
    }
    return ps.handler(ps.unwrap(std::forward<Value>(x)));
}

template<typename Value, typename PatternStatementT, typename... RestPatternStatements>
constexpr auto match_impl(Value&& x, const PatternStatementT& ps, const RestPatternStatements&... rests) {
    if (ps.condition(x)) {
        return ps.handler(ps.unwrap(std::forward<Value>(x)));
    }
    return match_impl(std::forward<Value>(x), rests...);
}

}  // namespace easymatch_impl

using easymatch_impl::as;
using easymatch_impl::some;
using easymatch_impl::none;
using easymatch_impl::when;
using easymatch_impl::_;
using easymatch_impl::pattern;
using easymatch_impl::ds;

template<typename T>
constexpr auto match(T&& x) {
    return [&](auto&&... args) {
        return easymatch_impl::match_impl(std::forward<decltype(x)>(x), std::forward<decltype(args)>(args)...);
    };
}

template<typename... Args>
constexpr auto match(Args&&... x) {
    return [&](auto&&... args) {
        return easymatch_impl::match_impl(std::forward_as_tuple(x...), std::forward<decltype(args)>(args)...);
    };
}

}  // namespace easymatch

#endif  // EASY_MATCH_HPP_
