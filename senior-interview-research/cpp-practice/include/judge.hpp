// judge.hpp: a small test harness with no dependencies, for practice problems.
//
//   TEST("name") { EXPECT_EQ(actual, expected); }       ordinary test
//   STRESS("name", iterations) { ... CHECK_SAME(...); }  randomized check against an oracle
//                                                        (the body gets `rng` and `size`)
//   double ms = judge::time_ms([&] { ... });             timing
//   EXPECT_UNDER_MS(ms, limit);                          enforced only in optimized builds
//
// Tests whose name starts with "perf" are skipped in unoptimized (F5) builds unless the
// filter argument selects them, because their large inputs can run for minutes there.
//
// A failing stress check prints the smallest input it could find. Trace that
// input by hand before you change any code.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
#endif

namespace judge {

// ---------- printing values in failure messages ----------

namespace detail {
template <class T> struct is_pair : std::false_type {};
template <class A, class B> struct is_pair<std::pair<A, B>> : std::true_type {};
template <class T> struct is_tuple : std::false_type {};
template <class... A> struct is_tuple<std::tuple<A...>> : std::true_type {};
template <class T> struct is_optional : std::false_type {};
template <class T> struct is_optional<std::optional<T>> : std::true_type {};
template <class T> concept Range = requires(const T& v) { std::begin(v); std::end(v); };
template <class T> concept Streamable = requires(std::ostream& os, const T& v) { os << v; };
template <class T> concept HasJudgeShow = requires(const T& v) { judge_show(v); };
}  // namespace detail

template <class T>
std::string show(const T& v) {
    using U = std::remove_cvref_t<T>;
    std::ostringstream os;
    if constexpr (detail::HasJudgeShow<U>) {
        os << judge_show(v);
    } else if constexpr (std::is_same_v<U, bool>) {
        os << (v ? "true" : "false");
    } else if constexpr (std::is_same_v<U, char>) {
        os << '\'' << v << '\'';
    } else if constexpr (std::is_convertible_v<const U&, std::string_view>) {
        os << '"' << std::string_view(v) << '"';
    } else if constexpr (std::is_floating_point_v<U>) {
        os << std::setprecision(10) << v;
    } else if constexpr (detail::is_pair<U>::value) {
        os << '(' << show(v.first) << ", " << show(v.second) << ')';
    } else if constexpr (detail::is_tuple<U>::value) {
        os << '(';
        std::apply([&](const auto&... xs) {
            std::size_t i = 0;
            ((os << (i++ ? ", " : "") << show(xs)), ...);
        }, v);
        os << ')';
    } else if constexpr (detail::is_optional<U>::value) {
        os << (v ? show(*v) : std::string("nullopt"));
    } else if constexpr (std::is_pointer_v<U>) {
        if (v == nullptr) os << "nullptr"; else os << "ptr";
    } else if constexpr (detail::Range<U>) {
        os << '[';
        std::size_t i = 0;
        for (const auto& x : v) {
            if (i) os << ", ";
            if (++i > 40) { os << "..."; break; }
            os << show(x);
        }
        os << ']';
    } else if constexpr (detail::Streamable<U>) {
        os << v;
    } else {
        os << "<?>";
    }
    return os.str();
}

inline std::string show_all() { return ""; }
template <class T, class... R>
std::string show_all(const T& first, const R&... rest) {
    std::string s = show(first);
    ((s += ", " + show(rest)), ...);
    return s;
}

// ---------- test registry and failure state ----------

struct TestCase { std::string name; std::function<void()> fn; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
struct Registrar {
    Registrar(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

struct StopTest {};  // thrown after a counterexample so the stress test ends there

inline int& failures() { static int n = 0; return n; }
inline int& iteration() { static int i = -1; return i; }
inline std::uint64_t& seed() { static std::uint64_t s = 20261115; return s; }

inline std::string basename(const char* path) {
    std::string_view p(path);
    auto pos = p.find_last_of("/\\");
    return std::string(pos == std::string_view::npos ? p : p.substr(pos + 1));
}

inline void report(const char* file, int line, const std::string& what) {
    int n = ++failures();
    if (n <= 5) std::cout << "    " << basename(file) << ":" << line << ": " << what << "\n";
    else if (n == 6) std::cout << "    ... (further failures in this test suppressed)\n";
}

template <class A>
bool expect_eq(const char* expr, const char* file, int line, const A& actual,
               const std::type_identity_t<A>& expected) {
    if (actual == expected) return true;
    report(file, line, std::string("EXPECT_EQ(") + expr + ")\n      expected: " + show(expected) +
                           "\n      got     : " + show(actual));
    return false;
}

inline bool expect_near(const char* expr, const char* file, int line, double actual, double expected,
                        double eps) {
    if (std::fabs(actual - expected) <= eps) return true;
    report(file, line, std::string("EXPECT_NEAR(") + expr + ")\n      expected: " + show(expected) +
                           "\n      got     : " + show(actual));
    return false;
}

[[noreturn]] inline void counterexample(const char* file, int line, const char* names, const std::string& inputs,
                                        const std::string& expected, const std::string& got) {
    std::ostringstream os;
    if (iteration() >= 0)
        os << "counterexample at stress iteration " << iteration() << " (seed " << seed() << ")\n";
    else
        os << "check failed\n";
    os << "      input (" << names << "): " << inputs << "\n";
    os << "      expected: " << expected << "\n";
    os << "      got     : " << got << "\n";
    os << "      -> trace this input by hand and find where your invariant breaks";
    report(file, line, os.str());
    throw StopTest{};
}

// ---------- randomness for stress tests ----------

class Rng {
public:
    explicit Rng(std::uint64_t s) : g_(s) {}
    int range(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(g_); }
    long long range64(long long lo, long long hi) { return std::uniform_int_distribution<long long>(lo, hi)(g_); }
    bool coin(double p = 0.5) { return std::bernoulli_distribution(p)(g_); }
    std::vector<int> vec(int n, int lo, int hi) {
        std::vector<int> v(static_cast<std::size_t>(std::max(n, 0)));
        for (auto& x : v) x = range(lo, hi);
        return v;
    }
    std::vector<int> distinct(int n, int lo, int hi) {  // requires hi - lo + 1 >= n
        std::vector<int> all;
        for (int x = lo; x <= hi; ++x) all.push_back(x);
        shuffle(all);
        all.resize(static_cast<std::size_t>(n));
        return all;
    }
    std::string str(int n, std::string_view alphabet) {
        std::string s(static_cast<std::size_t>(std::max(n, 0)), ' ');
        for (auto& c : s) c = alphabet[static_cast<std::size_t>(range(0, static_cast<int>(alphabet.size()) - 1))];
        return s;
    }
    template <class T> void shuffle(std::vector<T>& v) { std::shuffle(v.begin(), v.end(), g_); }
    std::mt19937_64& engine() { return g_; }

private:
    std::mt19937_64 g_;
};

// size grows from 1 to 10 across the iterations, so the first counterexample found is small.
inline void run_stress(int iterations, void (*body)(Rng&, int)) {
    Rng rng(seed());
    for (int i = 0; i < iterations; ++i) {
        iteration() = i;
        int size = 1 + (iterations > 1 ? (9 * i) / (iterations - 1) : 0);
        body(rng, size);
    }
    iteration() = -1;
}

// ---------- timing ----------

template <class F>
double time_ms(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    std::forward<F>(f)();
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

inline void expect_under_ms(const char* file, int line, double ms, double limit) {
    std::cout << "    time " << std::fixed << std::setprecision(1) << ms << " ms (limit " << limit << " ms)\n";
    std::cout.unsetf(std::ios::fixed);
#ifdef __OPTIMIZE__
    if (ms > limit) {
        std::ostringstream os;
        os << "too slow: " << ms << " ms > " << limit
           << " ms. This usually means the complexity is wrong, not the constant factor";
        report(file, line, os.str());
    }
#else
    (void)file; (void)line;
    std::cout << "    (unoptimized build: time limit not enforced)\n";
#endif
}

// ---------- watchdog ----------
// An exponential or endless solution would otherwise hang forever. In optimized builds a
// background thread ends the program when one test runs longer than the limit (exit code 3).
// Debug builds have no watchdog, so pausing at a breakpoint is safe.

inline long long steady_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
inline std::atomic<long long>& test_started_ms() { static std::atomic<long long> v{0}; return v; }
inline std::atomic<const std::string*>& running_test() { static std::atomic<const std::string*> v{nullptr}; return v; }

inline void start_watchdog(double limit_s) {
#ifdef __OPTIMIZE__
    std::thread([limit_s] {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            long long started = test_started_ms().load();
            if (started == 0 || steady_ms() - started <= static_cast<long long>(limit_s * 1000)) continue;
            const std::string* name = running_test().load();
            std::fflush(stdout);
            std::printf("\n    TIMEOUT: \"%s\" still running after %.0f s. The algorithm is probably exponential "
                        "(for example recursion without memoization) or stuck in an endless loop.\n",
                        name ? name->c_str() : "?", limit_s);
            std::fflush(stdout);
            std::_Exit(3);
        }
    }).detach();
#else
    (void)limit_s;
#endif
}

// ---------- runner ----------

inline int run_all(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    std::string filter;
    double timeout_s = 10;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--seed=", 0) == 0) seed() = std::stoull(a.substr(7));
        else if (a.rfind("--timeout=", 0) == 0) timeout_s = std::stod(a.substr(10));
        else filter = a;
    }
    start_watchdog(timeout_s);
    int passed = 0, failed = 0, skipped = 0;
    for (auto& t : registry()) {
        if (!filter.empty() && t.name.find(filter) == std::string::npos) continue;
#ifndef __OPTIMIZE__
        // Unoptimized builds do not enforce time limits, and _GLIBCXX_DEBUG checks the whole heap
        // on every priority_queue push and pop, which turns a large perf input into minutes of
        // O(n^2) work. Skip perf tests here unless the filter asked for them by name.
        if (filter.empty() && t.name.rfind("perf", 0) == 0) {
            ++skipped;
            std::cout << "[ SKIP ] " << t.name << " (unoptimized build; pass \"perf\" to run it)\n";
            continue;
        }
#endif
        failures() = 0;
        iteration() = -1;
        std::cout << "[ RUN  ] " << t.name << std::endl;  // flushed, so a crash shows which test was running
        running_test() = &t.name;
        test_started_ms() = steady_ms();
        auto t0 = std::chrono::steady_clock::now();
        try {
            t.fn();
        } catch (const StopTest&) {
        } catch (const std::exception& e) {
            report("test", 0, std::string("uncaught exception: ") + e.what());
        } catch (...) {
            report("test", 0, "uncaught non-standard exception");
        }
        test_started_ms() = 0;
        auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        if (failures() == 0) {
            ++passed;
            std::cout << "[  OK  ] " << t.name << " (" << static_cast<long long>(ms) << " ms)\n";
        } else {
            ++failed;
            std::cout << "[ FAIL ] " << t.name << "\n";
        }
    }
    std::cout << "\n" << passed << "/" << (passed + failed) << " passed";
    if (skipped > 0) std::cout << ", " << skipped << " perf skipped (unoptimized build)";
    std::cout << "\n";
    return failed == 0 ? 0 : 1;
}

}  // namespace judge

#define JUDGE_CAT2(a, b) a##b
#define JUDGE_CAT(a, b) JUDGE_CAT2(a, b)

#define TEST(name)                                                                    \
    static void JUDGE_CAT(judge_test_, __LINE__)();                                   \
    static ::judge::Registrar JUDGE_CAT(judge_reg_, __LINE__)(name, JUDGE_CAT(judge_test_, __LINE__)); \
    static void JUDGE_CAT(judge_test_, __LINE__)()

#define STRESS(name, iterations)                                                      \
    static void JUDGE_CAT(judge_stress_, __LINE__)(::judge::Rng&, int);               \
    static ::judge::Registrar JUDGE_CAT(judge_reg_, __LINE__)(                         \
        name, [] { ::judge::run_stress((iterations), JUDGE_CAT(judge_stress_, __LINE__)); }); \
    static void JUDGE_CAT(judge_stress_, __LINE__)([[maybe_unused]] ::judge::Rng& rng, [[maybe_unused]] int size)

// EXPECT_EQ(actual, expected): expected may be a braced list such as {1, 2, 3}.
#define EXPECT_EQ(actual, ...) ::judge::expect_eq(#actual, __FILE__, __LINE__, (actual), __VA_ARGS__)
#define EXPECT_NEAR(actual, expected, eps) \
    ::judge::expect_near(#actual, __FILE__, __LINE__, (actual), (expected), (eps))
#define EXPECT_TRUE(cond)                                                         \
    do {                                                                          \
        if (!(cond)) ::judge::report(__FILE__, __LINE__, "EXPECT_TRUE(" #cond ")"); \
    } while (0)

// CHECK_SAME(actual, expected, inputs...): on mismatch, prints the inputs and stops this test.
// Pass copies of the inputs to the solution, because LeetCode signatures may modify their arguments.
#define CHECK_SAME(actual, expected, ...)                                                       \
    do {                                                                                        \
        auto&& judge_a_ = (actual);                                                             \
        auto&& judge_e_ = (expected);                                                           \
        if (!(judge_a_ == judge_e_))                                                            \
            ::judge::counterexample(__FILE__, __LINE__, #__VA_ARGS__, ::judge::show_all(__VA_ARGS__), \
                                    ::judge::show(judge_e_), ::judge::show(judge_a_));          \
    } while (0)

// CHECK_VALID(reason, inputs...): reason is a std::string that is empty when the answer is valid.
// Use it for problems where more than one answer is correct.
#define CHECK_VALID(reason, ...)                                                                 \
    do {                                                                                         \
        std::string judge_r_ = (reason);                                                         \
        if (!judge_r_.empty())                                                                   \
            ::judge::counterexample(__FILE__, __LINE__, #__VA_ARGS__, ::judge::show_all(__VA_ARGS__), \
                                    "a valid answer", judge_r_);                                 \
    } while (0)

#define EXPECT_UNDER_MS(ms, limit) ::judge::expect_under_ms(__FILE__, __LINE__, (ms), (limit))
