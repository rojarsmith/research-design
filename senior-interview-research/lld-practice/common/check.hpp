// check.hpp: the small test harness every LLD problem uses. It needs nothing beyond g++.
//
//   TEST("name", phase) { EXPECT_EQ(got, expected); }    phase 1 is the core, 2 and up are the variants
//   THREAD_TEST("name", phase) { ... }                   runs in its own process with a deadlock watchdog
//   return check::missing<T>("signature");               a variant test calls a facade method you have not added yet
//
// Every test has a watchdog. A test still running after 10 seconds (THREAD_TEST: 20 seconds) is
// stopped and reported, so an endless loop or a deadlock never hangs `ood test`.
#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
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
    bool isolated;
    std::function<void()> fn;
};

inline std::vector<Case>& cases() {
    static std::vector<Case> v;
    return v;
}

struct Reg {
    Reg(const char* name, int phase, bool isolated, std::function<void()> fn) {
        cases().push_back({name, phase, isolated, std::move(fn)});
    }
};

inline std::mutex& out_mutex() {
    static std::mutex m;
    return m;
}

inline int& failures() {
    static int n = 0;
    return n;
}

// Safe to call from worker threads.
inline void fail(const char* file, int line, const std::string& what) {
    std::lock_guard lk(out_mutex());
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

// A variant adds methods to the facade. Its tests reach them through small helpers that compile
// whether or not the method exists yet; when it does not, the test fails with this message.
template <class T>
T missing(const std::string& signature) {
    fail("facade", 0, "missing from your facade: " + signature + "  (add it as the variant describes)");
    return T{};
}
inline void missing_void(const std::string& signature) { missing<int>(signature); }

// Wraps a call made on a worker thread: an exception becomes a test failure instead of std::terminate.
template <class F>
void guarded(F&& f) {
    try {
        f();
    } catch (const std::exception& e) {
        fail("thread", 0, std::string("uncaught exception in a worker thread: ") + e.what());
    } catch (...) {
        fail("thread", 0, "uncaught non-standard exception in a worker thread");
    }
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
    long long range(long long lo, long long hi) { return lo + (long long)(next() % (std::uint64_t)(hi - lo + 1)); }
};

// ---------- runner ----------

inline std::string& current_test() {
    static std::string s;
    return s;
}

// Runs one test in this process with a watchdog. Returns true if it passed.
inline bool run_here(const Case& c) {
    current_test() = c.name;
    failures() = 0;
    double limit_s = c.isolated ? 20 : 10;
    std::mutex m;
    std::condition_variable cv;
    bool finished = false;
    std::thread dog([&] {
        std::unique_lock lk(m);
        if (!cv.wait_for(lk, std::chrono::duration<double>(limit_s), [&] { return finished; })) {
            std::cout << "  TIMEOUT " << c.name << ": still running after " << (int)limit_s << " s. Stopped.\n"
                      << (c.isolated ? "        With threads this usually means a deadlock: two locks taken in different orders,\n"
                                       "        or a wait that nobody wakes up. A reference design finishes each thread test in\n"
                                       "        about a second, so a correct but very slow design (scanning everything on every\n"
                                       "        call) can also end up here.\n"
                                     : "        Look for an endless loop.\n");
            std::cout.flush();
            std::_Exit(3);
        }
    });
    try {
        c.fn();
    } catch (const std::exception& e) {
        fail("exception", 0, std::string("uncaught exception: ") + e.what());
    } catch (...) {
        fail("exception", 0, "uncaught non-standard exception");
    }
    { std::lock_guard lk(m); finished = true; }
    cv.notify_all();
    dog.join();
    std::cout.flush();
    return failures() == 0;
}

// THREAD_TEST cases run in a child process each, so a deadlock or a crash in one does not hide the others.
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
    if (rc == -1073741571 || rc == (int)0xC00000FD) std::cout << " (stack overflow)";
    else if (rc == -1073741819 || rc == (int)0xC0000005) std::cout << " (access violation: often a data race on a container)";
    else if (rc == 134 || rc == 139 || rc == 3) std::cout << " (abort or segmentation fault)";
    std::cout << "\n";
    return false;
}

inline std::string phase_label(int p) {
    return p <= 1 ? "phase 1 (core)" : "phase " + std::to_string(p) + " (variant " + std::to_string(p - 1) + ")";
}

inline int run(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    // Static registration order across files is unspecified; run the core first.
    std::stable_sort(cases().begin(), cases().end(), [](const Case& a, const Case& b) { return a.phase < b.phase; });

    if (args.size() == 2 && args[0] == "--only") {  // child process for one isolated test
        for (auto& c : cases())
            if (c.name == args[1]) return run_here(c) ? 0 : 1;
        std::cout << "no test named " << args[1] << "\n";
        return 1;
    }

    const int kMax = 9;
    int passed[kMax + 1] = {}, total[kMax + 1] = {};
    for (auto& c : cases()) {
        bool want = args.empty();
        for (auto& f : args)
            if (c.name.find(f) != std::string::npos) want = true;
        if (!want) continue;
        int p = std::clamp(c.phase, 1, kMax);
        std::cout << "  ....  [" << (p == 1 ? std::string("core") : "variant " + std::to_string(p - 1)) << "] " << c.name << "\n";
        bool ok = c.isolated ? run_isolated(argv[0], c) : run_here(c);
        ++total[p];
        if (ok) ++passed[p];
        std::cout << (ok ? "  PASS  " : "  FAIL  ") << c.name << "\n";
    }
    std::cout << "\n";
    bool all = true;
    for (int p = 1; p <= kMax; ++p) {
        if (!total[p]) continue;
        std::cout << "  " << phase_label(p) << ": " << passed[p] << "/" << total[p] << " passed\n";
        if (passed[p] != total[p]) all = false;
    }
    return all ? 0 : 1;
}

}  // namespace check

#define CHECK_CAT2(a, b) a##b
#define CHECK_CAT(a, b) CHECK_CAT2(a, b)
#define CHECK_REGISTER(name, phase, isolated)                                                                      \
    static void CHECK_CAT(check_test_, __LINE__)();                                                               \
    static check::Reg CHECK_CAT(check_reg_, __LINE__)(name, phase, isolated, CHECK_CAT(check_test_, __LINE__)); \
    static void CHECK_CAT(check_test_, __LINE__)()
#define TEST(name, phase) CHECK_REGISTER(name, phase, false)
#define THREAD_TEST(name, phase) CHECK_REGISTER(name, phase, true)
#define EXPECT_EQ(got, expected) check::expect_eq((got), (expected), #got, __FILE__, __LINE__)
#define EXPECT_TRUE(cond) \
    do { if (!(cond)) check::fail(__FILE__, __LINE__, "EXPECT_TRUE(" #cond ")"); } while (0)
