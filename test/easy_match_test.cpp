#include "easymatch/easymatch.hpp"

#include <any>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

#include <gtest/gtest.h>

using namespace easymatch;
using namespace std::string_literals;
using std::string;
using std::string_view;
using std::to_string;
using std::optional;
using std::make_optional;
using std::nullopt_t;

namespace {

constexpr int factorial(int n) {
    return match(n)(
        pattern | 0 = 1,
        pattern | _ = [](auto x) { return x * factorial(x - 1); }
    );
}

constexpr int factorial_2(int n) {
    return match(n)(
        pattern | 0 = []  { return 1; },
        pattern | _ = [n] { return n * factorial(n - 1); }
    );
}

TEST(EasyMatching, factorial) {
    static_assert(factorial(3) == 6);
    static_assert(factorial_2(3) == 6);
}

std::string check_value(int n) {
    return match(n)(
        pattern | (_ <   0) = [](int x) { return to_string(x) + " is negative.";         },
        pattern | (_ < 100) = [](int x) { return to_string(x) + " is lower than 100.";   },
        pattern | 100       = [](int x) { return to_string(x) + " is 100.";              },
        pattern | _         = [](int x) { return to_string(x) + " is greater than 100."; }
    );
}

TEST(EasyMatching, check_value) {
    EXPECT_EQ(check_value(-1),  "-1 is negative.");
    EXPECT_EQ(check_value(5),   "5 is lower than 100.");
    EXPECT_EQ(check_value(100), "100 is 100.");
    EXPECT_EQ(check_value(120), "120 is greater than 100.");
}

bool is_empty(const std::string& x) {
    return x.empty();
}

auto shorter_than(size_t n) {
    return [=](const std::string& x) -> bool {
        return x.size() < n;
    };
}

std::string string_length_check(const std::string& str) {
    return match(str)(
        pattern | &is_empty        = [](auto&& x) { return x + " is empty string.";                },
        pattern | shorter_than(5)  = [](auto&& x) { return x + " is shorter than 5.";              },
        pattern | shorter_than(10) = [](auto&& x) { return x + " is shorter than 10.";             },
        pattern | _                = [](auto&& x) { return x + " is equal to or longer than 10."; }
    );
}

TEST(EasyMatching, string_length_check) {
    EXPECT_EQ(string_length_check(""), " is empty string.");
    EXPECT_EQ(string_length_check("abc"), "abc is shorter than 5.");
    EXPECT_EQ(string_length_check("lorem"), "lorem is shorter than 10.");
    EXPECT_EQ(string_length_check("lorem ipsum"), "lorem ipsum is equal to or longer than 10.");
}

std::string check_variant(const std::variant<int, double, std::string>& var) {
    return match(var)(
        pattern | as<int> = [](auto&& x) {     // receives int
            return to_string(x) + " is int.";
        },
        pattern | as<string> = [](auto&& x) {  // receives string
            return x + " is string.";
        },
        pattern | as<double> = [](auto&& x) {  // receives double
            auto ss = std::stringstream();
            ss << std::fixed << std::setprecision(2) << x;
            return ss.str() + " is double.";
        }
    );
}

TEST(EasyMatching, check_variant) {
    EXPECT_EQ(check_variant(5),          "5 is int.");
    EXPECT_EQ(check_variant(3.14),       "3.14 is double.");
    EXPECT_EQ(check_variant("matching"), "matching is string.");
}

std::string_view check_any(const std::any& value) {
    return match(value)(
        pattern | as<int>    = std::string_view("holding int"),
        pattern | as<double> = std::string_view("holding double"),
        pattern | as<string> = std::string_view("holding string"),
        pattern | _          = [] { return std::string_view("holding unknown"); }
    );
}

TEST(EasyMatching, check_any) {
    EXPECT_EQ(check_any(5),                       "holding int");
    EXPECT_EQ(check_any(3.14),                    "holding double");
    EXPECT_EQ(check_any(std::string("matching")), "holding string");
    EXPECT_EQ(check_any(3.14f),                   "holding unknown");
}

std::string check_optional(const std::optional<int>& value) {
    return match(value)(
        pattern | some = [](int x)     { return "holds value: "s + to_string(x); },
        pattern | none = [](nullopt_t) { return "holds nullopt"s; }
    );
}

TEST(EasyMatching, check_optional) {
    EXPECT_EQ(check_optional(5),            "holds value: 5");
    EXPECT_EQ(check_optional(std::nullopt), "holds nullopt");
}

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

TEST(EasyMatching, check_numbers) {
    static_assert(check_numbers(1,  2,   3) == "1, 2, 3");
    static_assert(check_numbers(1,  2,  -1) == "1, 2, negative");
    static_assert(check_numbers(1,  2, 200) == "1, 2, large");
    static_assert(check_numbers(1,  2,  10) == "1, 2, _");
    static_assert(check_numbers(1,  9,   1) == "1, _, 1");
    static_assert(check_numbers(4,  8,   1) == "_, _, 1");
    static_assert(check_numbers(7,  7,   7) == "7, 7, 7");
    static_assert(check_numbers(9,  9,   9) == "otherwise");
}

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

TEST(EasyMatching, check_numbers_from_tuple) {
    static_assert(check_numbers_from_tuple(std::make_tuple(1,  2,   3)) == "1, 2, 3");
    static_assert(check_numbers_from_tuple(std::make_tuple(1,  2,  -1)) == "1, 2, negative");
    static_assert(check_numbers_from_tuple(std::make_tuple(1,  2, 200)) == "1, 2, large");
    static_assert(check_numbers_from_tuple(std::make_tuple(1,  2,  10)) == "1, 2, _");
    static_assert(check_numbers_from_tuple(std::make_tuple(1,  9,   1)) == "1, _, 1");
    static_assert(check_numbers_from_tuple(std::make_tuple(4,  8,   1)) == "_, _, 1");
    static_assert(check_numbers_from_tuple(std::make_tuple(7,  7,   7)) == "7, 7, 7");
    static_assert(check_numbers_from_tuple(std::make_tuple(9,  9,   9)) == "otherwise");
}

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

TEST(EasyMatching, check_large) {
    static_assert(check_large(200)                     == "large int");
    static_assert(check_large(30)                      == "small int");
    static_assert(check_large(150.0)                   == "large double");
    static_assert(check_large(3.14)                    == "small double");
    static_assert(check_large("lorem ipsum dolor sit") == "large string");
    static_assert(check_large("lorem")                 == "small string");
    static_assert(check_large(make_optional(100))      == "large optional<int>");
    static_assert(check_large(make_optional(10))       == "small optional<int>");
    static_assert(check_large(std::nullopt)            == "nullopt");
}

void tuple_handler(int a, int b, int c, std::stringstream& ss) {
    match(a, b, c)(
        // receive matched result as parameters.
        pattern | ds(0, 1, _ < 0) = [&](int x, int y, int z) {
            ss << x << " is zero\n";
            ss << y << " is one\n";
            ss << z << " is negative\n";
        },

        // receive matched result as tuple.
        pattern | ds(0, 1, _ < 10) = [&](const std::tuple<int, int, int>& z) {
            ss << std::get<0>(z) << " is zero\n";
            ss << std::get<1>(z) << " is one\n";
            ss << std::get<2>(z) << " is lower than 10\n";
        },

        // nullary lambda is also OK. {a, b, c} are captured to the lambda.
        pattern | ds(0, 1, _ < 20) = [&]() {
            ss << a << " is zero\n";
            ss << b << " is one\n";
            ss << c << " is lower than 20\n";
        },

        pattern | _ = [&] {
            ss << "not matched\n";
        }
    );
}

TEST(EasyMatching, tuple_handler) {
    std::stringstream ss;

    tuple_handler(0, 1, -1, ss);
    EXPECT_EQ(ss.str(), "0 is zero\n1 is one\n-1 is negative\n");
    ss.str("");

    tuple_handler(0, 1, 5, ss);
    EXPECT_EQ(ss.str(), "0 is zero\n1 is one\n5 is lower than 10\n");
    ss.str("");

    tuple_handler(0, 1, 15, ss);
    EXPECT_EQ(ss.str(), "0 is zero\n1 is one\n15 is lower than 20\n");
    ss.str("");

    tuple_handler(0, 1, 25, ss);
    EXPECT_EQ(ss.str(), "not matched\n");
    ss.str("");
}

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

TEST(EasyMatching, unwrap_tuple) {
    EXPECT_EQ(unwrap_tuple(1, 2), "a: 1 is value; b: 2 is int");
    EXPECT_EQ(unwrap_tuple(-1, "easy-matching"), "a: -1 is value and negative; b: easy-matching is string");
    EXPECT_EQ(unwrap_tuple(5, "easy-matching"), "a: 5 is value and non-negative; b: easy-matching is string");
    EXPECT_EQ(unwrap_tuple(std::nullopt, -4), "a is nullopt; b: -4 is negative int");
    EXPECT_EQ(unwrap_tuple(std::nullopt, 0), "a is nullopt; b: 0 is non-negative int");
    EXPECT_EQ(unwrap_tuple(std::nullopt, "easy-matching"), "a is nullopt; b: easy-matching is string");
}

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

TEST(EasyMatching, simplified_match) {
    EXPECT_EQ(simplified_match(-3), "negative");
    EXPECT_EQ(simplified_match(7),  "7");
    EXPECT_EQ(simplified_match(8),  "lower than 10");
    EXPECT_EQ(simplified_match(10), "10");
    EXPECT_EQ(simplified_match(99), "otherwise");
}

}  // namespace
