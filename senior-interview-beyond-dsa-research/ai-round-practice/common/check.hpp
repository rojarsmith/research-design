// check.hpp: the small test harness every scenario uses. It needs nothing beyond g++.
//
//   TEST("name", phase) { EXPECT_EQ(got, expected); }
//   double ms = check::measure(limit_ms, [&] { ... });   // fails the test if slower than limit_ms
//
// measure() also starts a watchdog. If the measured code runs past the hard stop
// (3x the limit, at least 3 seconds), the test binary prints TLE and exits, so a
// quadratic solution never hangs the drill.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace check {

// ---------- printing values ----------

template <class T>
concept Streamable = requires(std::ostream& os, const T& v) { os << v; };
template <class T>
concept Range = requires(const T& v) { std::begin(v); std::end(v); };

template <class T>
std::string show(const T& v) {
    using U = std::remove_cvref_t<T>;
    std::ostringstream os;
    if constexpr (std::is_same_v<U, bool>) {
        os << (v ? "true" : "false");
    } else if constexpr (std::is_convertible_v<const U&, std::string_view>) {
        os << '"' << std::string_view(v) << '"';
    } else if constexpr (Streamable<U>) {
        os << v;
    } else if constexpr (Range<U>) {
        os << '[';
        int i = 0;
        for (const auto& x : v) {
            if (i) os << ", ";
            if (++i > 20) { os << "..."; break; }
            os << show(x);
        }
        os << ']';
    } else {
        os << "<?>";
    }
    return os.str();
}

// ---------- registry ----------

struct Case {
    std::string name;
    int phase;
    std::function<void()> fn;
};

inline std::vector<Case>& cases() {
    static std::vector<Case> v;
    return v;
}

struct Reg {
    Reg(const char* name, int phase, std::function<void()> fn) { cases().push_back({name, phase, std::move(fn)}); }
};

inline int& failures() {
    static int n = 0;
    return n;
}

inline void fail(const char* file, int line, const std::string& what) {
    int n = ++failures();
    std::string f = file;
    auto pos = f.find_last_of("/\\");
    if (pos != std::string::npos) f = f.substr(pos + 1);
    if (n <= 4) std::cout << "      " << (line > 0 ? f + ":" + std::to_string(line) + ": " : std::string()) << what << "\n";
    else if (n == 5) std::cout << "      ... (more failures in this test hidden)\n";
}

template <class A, class B>
void expect_eq(const A& got, const B& expected, const char* expr, const char* file, int line) {
    if (got == expected) return;
    fail(file, line, std::string("EXPECT_EQ(") + expr + ")\n        expected: " + show(expected) +
                         "\n        got     : " + show(got));
}

// ---------- timing with a watchdog ----------

inline std::string& current_test() {
    static std::string s;
    return s;
}

template <class F>
double measure(double limit_ms, F&& fn) {
    double hard_ms = std::max(3 * limit_ms, 3000.0);
    std::mutex m;
    std::condition_variable cv;
    bool finished = false;
    std::thread dog([&] {
        std::unique_lock lk(m);
        if (!cv.wait_for(lk, std::chrono::duration<double, std::milli>(hard_ms), [&] { return finished; })) {
            std::cout << "  TLE   " << current_test() << ": still running after " << (long long)hard_ms
                      << " ms (limit " << (long long)limit_ms << " ms). Stopped.\n"
                      << "        Too slow by a wide margin: check the complexity before tuning constants.\n";
            std::cout.flush();
            std::_Exit(3);
        }
    });
    auto t0 = std::chrono::steady_clock::now();
    try {
        fn();
    } catch (...) {
        { std::lock_guard lk(m); finished = true; }
        cv.notify_all();
        dog.join();
        throw;
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    { std::lock_guard lk(m); finished = true; }
    cv.notify_all();
    dog.join();
    std::cout << "        time " << (long long)ms << " ms (limit " << (long long)limit_ms << " ms)\n";
    if (ms > limit_ms) {
        ++failures();
        std::cout << "      too slow: " << (long long)ms << " ms > " << (long long)limit_ms << " ms\n";
    }
    return ms;
}

// ---------- deterministic data ----------

// splitmix64: the same numbers on every compiler and platform (std:: distributions are not).
struct Rng {
    std::uint64_t s;
    explicit Rng(std::uint64_t seed) : s(seed) {}
    std::uint64_t next() {
        std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    // uniform-ish integer in [lo, hi]
    long long range(long long lo, long long hi) { return lo + (long long)(next() % (std::uint64_t)(hi - lo + 1)); }
};

// FNV-1a, used to compare large outputs without shipping them.
inline std::uint64_t fnv1a(std::string_view s) {
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

// Writes generated data under data/generated/ so you can look at its shape.
inline std::string write_generated(const std::string& name, const std::string& content) {
    namespace fs = std::filesystem;
    fs::create_directories("data/generated");
    std::string path = "data/generated/" + name;
    std::error_code ec;
    if (!fs::exists(path, ec) || fs::file_size(path, ec) != content.size()) {
        std::ofstream(path, std::ios::binary) << content;
    }
    return path;
}

inline std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// ---------- runner ----------

// Runs one test in this process. Returns true if it passed.
inline bool run_here(const Case& c) {
    current_test() = c.name;
    failures() = 0;
    try {
        c.fn();
    } catch (const std::exception& e) {
        fail("exception", 0, std::string("uncaught exception: ") + e.what());
    } catch (...) {
        fail("exception", 0, "uncaught non-standard exception");
    }
    std::cout.flush();
    return failures() == 0;
}

// Phase 3 tests run in a child process each, so a timeout or a crash (for example a
// stack overflow from deep recursion) in one large test does not hide the others.
inline bool run_isolated(const char* self, const Case& c) {
    std::cout.flush();
#ifdef _WIN32
    std::string cmd = "\"\"" + std::string(self) + "\" --only \"" + c.name + "\"\"";
    int rc = std::system(cmd.c_str());
#else
    std::string cmd = "'" + std::string(self) + "' --only '" + c.name + "'";
    int status = std::system(cmd.c_str());
    int rc = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
#endif
    if (rc == 0 || rc == 1 || rc == 3) return rc == 0;
    std::cout << "      crashed with exit code " << rc;
    if (rc == -1073741571 || rc == (int)0xC00000FD) std::cout << " (stack overflow: is there deep recursion?)";
    else if (rc == -1073741819 || rc == (int)0xC0000005) std::cout << " (access violation)";
    else if (rc == 134 || rc == 139) std::cout << " (abort or segmentation fault)";
    std::cout << "\n";
    return false;
}

inline int run(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    // Static registration order across files is unspecified; run phase 1 first.
    std::stable_sort(cases().begin(), cases().end(), [](const Case& a, const Case& b) { return a.phase < b.phase; });

    if (args.size() == 2 && args[0] == "--only") {  // child process for one isolated test
        for (auto& c : cases())
            if (c.name == args[1]) return run_here(c) ? 0 : 1;
        std::cout << "no test named " << args[1] << "\n";
        return 1;
    }

    int passed[4] = {0, 0, 0, 0}, total[4] = {0, 0, 0, 0};
    for (auto& c : cases()) {
        bool want = args.empty();
        for (auto& f : args)
            if (c.name.find(f) != std::string::npos) want = true;
        if (!want) continue;
        std::cout << "  ....  [phase " << c.phase << "] " << c.name << "\n";
        bool ok = c.phase >= 3 ? run_isolated(argv[0], c) : run_here(c);
        int p = std::clamp(c.phase, 1, 3);
        ++total[p];
        if (ok) ++passed[p];
        std::cout << (ok ? "  PASS  " : "  FAIL  ") << c.name << "\n";
    }
    std::cout << "\n";
    bool all = true;
    for (int p = 1; p <= 3; ++p) {
        if (!total[p]) continue;
        std::cout << "  phase " << p << ": " << passed[p] << "/" << total[p] << " passed\n";
        if (passed[p] != total[p]) all = false;
    }
    return all ? 0 : 1;
}

}  // namespace check

#define CHECK_CAT2(a, b) a##b
#define CHECK_CAT(a, b) CHECK_CAT2(a, b)
#define TEST(name, phase)                                                                 \
    static void CHECK_CAT(check_test_, __LINE__)();                                       \
    static check::Reg CHECK_CAT(check_reg_, __LINE__)(name, phase, CHECK_CAT(check_test_, __LINE__)); \
    static void CHECK_CAT(check_test_, __LINE__)()
#define EXPECT_EQ(got, expected) check::expect_eq((got), (expected), #got, __FILE__, __LINE__)
#define EXPECT_TRUE(cond) \
    do { if (!(cond)) check::fail(__FILE__, __LINE__, "EXPECT_TRUE(" #cond ")"); } while (0)
