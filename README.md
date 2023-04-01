# Easy-Match: Header-only pattern matching library for C++17

## Features

* Single header library

* Macro-free APIs

* No heap memory allocation

* Composable patterns

## Goals & Non-Goals

Easy-Match aims to provide simple syntax for pattern matching.

It is not our goal to provide full-features of pattern matching. This library provides only some simple pattern matchings.

## Quick Start

### Basic Syntax

The basic syntax for pattern matching is as follows.

```C++
match(VALUE)(
    pattern | PATTERN1 = HANDLER1,
    pattern | PATTERN2 = HANDLER2,
    ...
)
```

The handler can be one of the followings.

* variable
* nullary function
* function that receives the matched value

Here is an example.

```C++
#include "easymatch/easymatch.hpp"

using namespace easymatch;

constexpr int factorial(int n) {
    return match(n)(
        pattern | 0 = 1,
        pattern | _ = [](auto x) { return x * factorial(x - 1); }
    );
}

```

You can also implement factorial function as follows.

```C++
// nullary function can also be handler.
constexpr int factorial(int n) {
    return match(n)(
        pattern | 0 = []  () { return 1; },
        pattern | _ = [n] () { return n * factorial(n - 1); }
    );
}
```

The result of `match(x)` ia a value returned by one of the handlers. The return type is the common for all handlers and will be void if if all handlers do not return value. Incompatible return types from multiple handlers is a compile error.

The wildcard `_` will match any values. It is recommended to to always use it as the last pattern to avoid case escaping.

### Conditional Matching

You can write conditional matching using wildcard `_`.

```C++
#include "easymatch/easymatch.hpp"

#include <string>

using namespace easymatch;
using std::to_string;

std::string check_value(int n) {
    return match(n)(
        pattern | (_ <    0) = [](int x) { return to_string(x) + " is negative.";         },
        pattern | (_ <  100) = [](int x) { return to_string(x) + " is lower than 100.";   },
        pattern | 100        = [](int x) { return to_string(x) + " is 100.";              },
        pattern | _          = [](int x) { return to_string(x) + " is greater than 100."; }
    );
}
```

The followings are conditional patterns that compares value with X.

* `_ == X`
* `_ != X`
* `_ < X`
* `_ <= X`
* `_ > X`
* `X == _`
* `X != _`
* `X < _`
* `X <= _`
* `X > _`

Function and lambda can also be conditional pattern.

```C++
#include "easymatch/easymatch.hpp"

#include <string>

using namespace easymatch;

bool is_empty(const std::string& x) {
    return x.empty();
}

// makes lambda
auto shorter_than(size_t n) {
    return [=](const std::string& x) -> bool {
        return x.size() < n;
    };
}

std::string string_length_check(const std::string& str) {
    return match(str)(
        pattern | &is_empty        = [](auto&& x) { return x + " is empty string.";               },
        pattern | shorter_than(5)  = [](auto&& x) { return x + " is shorter than 5.";             },
        pattern | shorter_than(20) = [](auto&& x) { return x + " is shorter than 20.";            },
        pattern | _                = [](auto&& x) { return x + " is equal to or longer than 20."; }
    );
}
```

### Type Matching

Type matcher `as<T>` is for `std::variant` and `std::any`. It checks the holding type of the value and unwrap it to type `T`.

```C++
#include "easymatch/easymatch.hpp"

#include <string>
#include <variant>

using namespace easymatch;
using std::string;
using std::to_string;

std::string check_variant(const std::variant<int, double, std::string>& value) {
    return match(value)(
        pattern | as<int>    = [](auto&& x) { return to_string(x) + " is int.";    }, // receives int
        pattern | as<double> = [](auto&& x) { return to_string(x) + " is double."; }, // receives double
        pattern | as<string> = [](auto&& x) { return x            + " is string."; }  // receives string
    );
}
```

In type matching, the handler can receive its holding type. Of course, variable and nullary function can also be handlers.

```C++
#include "easymatch/easymatch.hpp"

#include <any>
#include <string>
#include <string_view>

using namespace easymatch;
using std::any;
using std::string;

constexpr std::string_view check_any(const std::any& value) {
    return match(value)(
        pattern | as<int>    = std::string_view("holding int"),
        pattern | as<double> = std::string_view("holding double"),
        pattern | as<string> = std::string_view("holding string"),
        pattern | _          = [] { return std::string_view("holding unknown"); }
    );
}
```

### Optional Matching

Optional matcher `some` and `none` are for `std::optional<T>`. They check if an optional has value or nullopt and unwrap it to type `T` or `std::nullopt`.

```C++
#include "easymatch/easymatch.hpp"

#include <optional>
#include <string>

using namespace easymatch;
using namespace std::string_literals;
using std::string;
using std::to_string;
using std::optional;
using std::nullopt_t;

std::string check_optional(const std::optional<int>& value) {
    return match(value)(
        pattern | some = [](int x)     { return "holds value: "s + to_string(x); },
        pattern | none = [](nullopt_t) { return "holds nullopt"s; }
    );
}
```

### Matching Multiple Values

Using `ds`, you can check multiple values.

```C++
#include "easymatch/easymatch.hpp"

#include <string_view>

using namespace easymatch;
using std::string_view;

constexpr std::string_view check_numbers(int a, int b, int c) {
    constexpr auto is_seven = [](auto x) { return x == 7; };
    return match(a, b, c)(
        pattern | ds(1, 2, 3       ) = string_view("1, 2, 3"),
        pattern | ds(1, 2, _ <   0 ) = string_view("1, 2, negative"),
        pattern | ds(1, 2, _ > 100 ) = string_view("1, 2, large"),
        pattern | ds(1, 2, _       ) = string_view("1, 2, _"),
        pattern | ds(1, _, 1       ) = string_view("1, _, 1"),
        pattern | ds(_, _, 1       ) = string_view("_, _, 1"),
        pattern | ds(7, is_seven, 7) = string_view("7, 7, 7"),
        pattern | _                  = string_view("otherwise")
    );
}
```

You can pass `std::tuple` to `match` in `ds` pattern. The following example works as same as the example above.

```C++
#include "easymatch/easymatch.hpp"

#include <string_view>
#include <tuple>

using namespace easymatch;
using std::string_view;

constexpr std::string_view check_numbers_from_tuple(const std::tuple<int, int, int>& value) {
    constexpr auto is_seven = [](auto x) { return x == 7; };
    return match(value)(
        pattern | ds(1, 2, 3       ) = string_view("1, 2, 3"),
        pattern | ds(1, 2, _ <   0 ) = string_view("1, 2, negative"),
        pattern | ds(1, 2, _ > 100 ) = string_view("1, 2, large"),
        pattern | ds(1, 2, _       ) = string_view("1, 2, _"),
        pattern | ds(1, _, 1       ) = string_view("1, _, 1"),
        pattern | ds(_, _, 1       ) = string_view("_, _, 1"),
        pattern | ds(7, is_seven, 7) = string_view("7, 7, 7"),
        pattern | _                  = string_view("otherwise")
    );
}
```

You can receive the matched results in handler as parameters or packed tuple. See the example below.

```C++
#include "easymatch/easymatch.hpp"

#include <iostream>
#include <tuple>

using namespace easymatch;

void tuple_handler(int a, int b, int c) {
    match(a, b, c)(
        // receive matched result as parameters.
        pattern | ds(0, 1, _ < 0) = [](int x, int y, int z) {
            std::cout << x << " is zero\n";
            std::cout << y << " is one\n";
            std::cout << z << " is negative\n";
        },

        // receive matched result as tuple.
        pattern | ds(0, 1, _ < 10) = [](const std::tuple<int, int, int>& z) {
            std::cout << std::get<0>(z) << " is zero\n";
            std::cout << std::get<1>(z) << " is one\n";
            std::cout << std::get<2>(z) << " is lower than 10\n";
        },

        // nullary lambda is also OK.
        // input value {a, b, c} are passed by capturing them to the lambda.
        pattern | ds(0, 0, _ < 20) = [&]() {
            std::cout << std::get<0>(a) << " is zero\n";
            std::cout << std::get<1>(b) << " is zero\n";
            std::cout << std::get<2>(c) << " is lower than 20\n";
        },

        pattern | _ = [] {
            std::cout << "not matched";
        }
    );
}
```

### Compose Patterns

You can pipe patterns with `|`.

```C++
#include "easymatch/easymatch.hpp"

#include <optional>
#include <string_view>

using namespace easymatch;
using std::optional;
using std::string_view;

constexpr std::string_view check_large(const std::variant<int, double, std::string_view, std::optional<int>>& x) {
    constexpr auto is_large_str = [](std::string_view x) {
        return x.size() > 20;
    };

    return match(x)(
        pattern | as<int>           | when(_ >= 100)        = string_view("large int"),
        pattern | as<int>           | _                     = string_view("small int"),
        pattern | as<double>        | when(_ >= 100)        = string_view("large double"),
        pattern | as<double>        | _                     = string_view("small double"),
        pattern | as<string_view>   | when(is_large_str)    = string_view("large string"),
        pattern | as<string_view>   | _                     = string_view("small string"),
        pattern | as<optional<int>> | some | when(_ >= 100) = string_view("large optional<int>"),
        pattern | as<optional<int>> | some | _              = string_view("small optional<int>"),
        pattern | as<optional<int>> | none                  = string_view("nullopt"),
        pattern | _                                         = string_view("otherwise")
    );
}
```

Actuary, `when` in the example above are not required. It is only a wrapper of patterns to make it clear that `when(...)` works as a filter.

### Other Examples

The following example shows how it works when composing `ds` and other patterns.

```C++
std::string unwrap_tuple(std::optional<int> a, const std::variant<int, std::string>& b) {
    return match(a, b)(
        pattern | ds(some, as<int>) = [](int x, int y) {
            std::stringstream ss;
            ss << "a: " << x << " is value; b: " << y << " is int";
            return ss.str();
        },

        pattern | ds(some | when(_ < 0), as<std::string>) = [](int x, const std::string& y) {
            std::stringstream ss;
            ss << "a: " << x << " is value and negative; b: " << y << " is string";
            return ss.str();
        },

        pattern | ds(some | _, as<std::string>) = [](int x, const std::string& y) {
            std::stringstream ss;
            ss << "a: " << x << " is value and non-negative; b: " << y << " is string";
            return ss.str();
        },

        pattern | ds(none, as<int> | when(_ < 0)) = [](nullopt_t, int y) {
            std::stringstream ss;
            ss << "a is nullopt; b: " << y << " is negative int";
            return ss.str();
        },

        pattern | ds(none, as<int> | _) = [](nullopt_t, int y) {
            std::stringstream ss;
            ss << "a is nullopt; b: " << y << " is non-negative int";
            return ss.str();
        },

        pattern | ds(none, as<std::string>) = [](nullopt_t, const std::string& y) {
            std::stringstream ss;
            ss << "a is nullopt; b: " << y << " is string";
            return ss.str();
        },

        pattern | _ = [] { return std::string("not matched"); }
    );
}
```

## Tips

In actual, `pattern` in the syntax is not always required. See the code below.

```C++
#include "easymatch/easymatch.hpp"

#include <string>

using namespace easymatch;
using namespace std::string_literals;

std::string simplified_match(int value) {
    auto is_seven = [](int x) { return x == 7; };

    return match(value)(
        _ < 0          = "negative"s,
        when(is_seven) = "7"s,
        _ < 10         = "lower than 10"s,
        when(10)       = "10"s,
        _              = "otherwise"s
    );
}
```

Patterns provided in this library such as `when`, `_`, `ds`, `some`, `as` are recognized as pattern, so you can write like `_ = handler`. However, types such as literal or lambda are not recognized as pattern, so you should wrap them with `when` or pipe from `pattern`.

```C++
match(x) (
    pattern | 1        = handler,  // ok
    pattern | lambda   = handler,  // ok
    when(2)            = handler,  // ok
    when(lambda)       = handler,  // ok
    _ < 3              = handler,  // ok
    pattern | (_ < 5)  = handler,  // ok

    5                  = handler,  // compile-error
    lambda             = handler,  // compile-error
)
```

In examples in this document, patterns are started with `pattern` to make it more consistent.

```C++
// This code may confuse readers why there is a compilation error.
match(x) (
    (_ <  0)  = "negative"s,
    0         = "zero"s,
    _         = "positive"s
);

// These syntax are also OK.
match(x) (
    (_ <  0)  = "negative"s,
    when(0)   = "zero"s,
    _         = "positive"s
);

// This syntax is more consistent.
match(x) (
    pattern | (_ <  0)  = "negative"s,
    pattern | 0         = "zero"s,
    pattern | _         = "positive"s
);
```

## Influenced Works

This library have been influenced by these great works.

* [mpark/patterns](https://github.com/mpark/patterns)
* [match(it)](https://github.com/BowenFu/matchit.cpp)

I personally recommend you to check the these libraries if you are interested in pattern-matching. They provides more functionalities and are well-tested.

The main difference is, Easy-Match's handler can receive the matched value as lambda parameter instead of their Identifier Pattern.

## Tested Compiler

* GCC 11.3.0
