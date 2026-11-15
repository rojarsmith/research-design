// pp.cpp: practice-loop CLI. It needs nothing beyond g++.
// Build: g++ -std=c++20 -O2 tools/pp.cpp -o build/pp.exe  (VS Code task "pp: 建置工具")
//
// Each command enforces one rule from the research report:
//   start  blank-file retrieval: archive the previous attempt, restore an empty stub
//   test   compile + run tests; reminds you of the 15-20 minute time box
//   hint   layered hints; using one turns this attempt's "pass" into "hint"
//   done   requires the review card ([觸發] and [關鍵洞見]) and schedules the next review
//   today  due reviews first, then one new problem; no new problem while the backlog is large
//   mix    interleaving: random solved problems with the pattern hidden
//   stats  weakest patterns first
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#else
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;
using std::string;
using std::vector;

// All strings inside pp are UTF-8. On Windows, argv arrives in the ANSI code page, and
// fs::path(std::string) also assumes ANSI, so convert explicitly at both boundaries.
static fs::path upath(const string& utf8) {
    return fs::path(std::u8string(utf8.begin(), utf8.end()));
}

static string ustr(const fs::path& p) {
    auto s = p.u8string();
    return string(s.begin(), s.end());
}

static vector<string> utf8_args(int argc, char** argv) {
#ifdef _WIN32
    (void)argc;
    (void)argv;
    int n = 0;
    LPWSTR* w = CommandLineToArgvW(GetCommandLineW(), &n);
    vector<string> out;
    for (int i = 0; i < n; ++i) {
        int len = WideCharToMultiByte(CP_UTF8, 0, w[i], -1, nullptr, 0, nullptr, nullptr);
        string s(static_cast<size_t>(len > 0 ? len - 1 : 0), '\0');
        if (len > 1) WideCharToMultiByte(CP_UTF8, 0, w[i], -1, s.data(), len, nullptr, nullptr);
        out.push_back(s);
    }
    LocalFree(w);
    return out;
#else
    return vector<string>(argv, argv + argc);
#endif
}

// ---------------------------------------------------------------- utilities

static string trim(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static vector<string> split(const string& s, char sep) {
    vector<string> out;
    string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

static string read_file(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void write_file(const fs::path& p, const string& s) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << s;
}

static string quote(const fs::path& p) { return "\"" + p.string() + "\""; }

// Runs a shell command and returns the program's exit code.
static int run(const string& cmd) {
#ifdef _WIN32
    // cmd.exe strips the outer quotes when the command starts with a quote, so wrap it once more.
    int rc = std::system(("\"" + cmd + "\"").c_str());
    return rc;
#else
    int rc = std::system(cmd.c_str());
    if (rc == -1) return -1;
    if (WIFEXITED(rc)) return WEXITSTATUS(rc);
    return 128 + (WIFSIGNALED(rc) ? WTERMSIG(rc) : 0);
#endif
}

#ifdef _WIN32
static const char* kNull = "nul";
#else
static const char* kNull = "/dev/null";
#endif

// ---------------------------------------------------------------- dates

using Days = std::chrono::sys_days;

static std::tm local_tm(std::time_t t) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

static Days today() {
    std::tm tm = local_tm(std::time(nullptr));
    return Days{std::chrono::year{tm.tm_year + 1900} / (tm.tm_mon + 1) / tm.tm_mday};
}

static string fmt_date(Days d) {
    std::chrono::year_month_day ymd{d};
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02u-%02u", static_cast<int>(ymd.year()),
                  static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()));
    return buf;
}

static Days parse_date(const string& s) {
    int y = 1970, m = 1, d = 1;
    std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
    return Days{std::chrono::year{y} / m / d};
}

static string stamp() {
    std::tm tm = local_tm(std::time(nullptr));
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", &tm);
    return buf;
}

// ---------------------------------------------------------------- catalog and paths

struct Problem {
    string id, kind, pattern, level, url, hint, lesson, cpp;
    int week = 0;
};

static fs::path g_root;
static vector<Problem> g_catalog;

static fs::path problem_dir(const Problem& p) { return g_root / "problems" / p.id; }
static bool is_io(const Problem& p) { return p.kind == "io"; }
static fs::path solution_file(const Problem& p) { return problem_dir(p) / (is_io(p) ? "solution.cpp" : "solution.hpp"); }
static fs::path stub_file(const Problem& p) { return problem_dir(p) / (is_io(p) ? "stub.cpp" : "stub.hpp"); }
static fs::path state_file(const Problem& p) { return g_root / "progress" / "state" / (p.id + ".txt"); }
static fs::path history_dir(const Problem& p) { return g_root / "progress" / "history" / p.id; }
static fs::path log_file() { return g_root / "progress" / "reviews.tsv"; }

static string level_name(const string& l) {
    if (l == "E") return "Easy";
    if (l == "H") return "Hard";
    return "Medium";
}

static int target_minutes(const string& l) {
    if (l == "E") return 15;
    if (l == "H") return 40;
    return 25;
}

static bool find_root(const string& argv0) {
    vector<fs::path> starts{fs::current_path()};
    std::error_code ec;
    fs::path exe = fs::weakly_canonical(upath(argv0), ec);
    if (!ec) starts.push_back(exe.parent_path());
    for (fs::path p : starts) {
        for (;;) {
            if (fs::exists(p / "problems" / "catalog.tsv")) { g_root = p; return true; }
            if (!p.has_parent_path() || p.parent_path() == p) break;
            p = p.parent_path();
        }
    }
    return false;
}

static void load_catalog() {
    std::istringstream in(read_file(g_root / "problems" / "catalog.tsv"));
    string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (trim(line).empty() || line[0] == '#') continue;
        auto f = split(line, '\t');
        if (f.size() < 9) continue;
        Problem p;
        p.id = f[0]; p.kind = f[1]; p.pattern = f[2]; p.level = f[3];
        p.week = std::atoi(f[4].c_str());
        p.url = f[5]; p.hint = f[6]; p.lesson = f[7]; p.cpp = f[8];
        g_catalog.push_back(p);
    }
}

// Accepts an id, a LeetCode number (739), or any path inside problems/<id>/.
static const Problem* resolve(const string& arg) {
    string key = arg;
    std::error_code ec;
    if (arg.find('/') != string::npos || arg.find('\\') != string::npos) {
        fs::path p = fs::weakly_canonical(upath(arg), ec);
        key.clear();
        for (fs::path q = p; q.has_parent_path() && q != q.parent_path(); q = q.parent_path()) {
            if (q.parent_path().filename() == "problems") { key = q.filename().string(); break; }
        }
        if (key.empty()) return nullptr;
    }
    if (!key.empty() && std::all_of(key.begin(), key.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "lc%04d-", std::atoi(key.c_str()));
        key = buf;
    }
    const Problem* found = nullptr;
    for (auto& p : g_catalog) {
        if (p.id == key) return &p;
        if (p.id.rfind(key, 0) == 0) {
            if (found) return nullptr;  // ambiguous prefix
            found = &p;
        }
    }
    return found;
}

static const Problem* need_problem(const vector<string>& args, size_t i) {
    if (args.size() <= i) {
        std::cout << "請指定題目：題號（739）、id，或在 VS Code 先打開那一題的 solution 檔再執行工作。\n";
        return nullptr;
    }
    const Problem* p = resolve(args[i]);
    if (!p) std::cout << "找不到題目「" << args[i] << "」。先打開某一題的 solution 檔，或用 pp list 查 id。\n";
    return p;
}

// ---------------------------------------------------------------- per-attempt state

struct State {
    bool active = false;
    long long start = 0;
    int hints = 0;
    bool blind = false;
};

static State load_state(const Problem& p) {
    State s;
    if (!fs::exists(state_file(p))) return s;
    s.active = true;
    std::istringstream in(read_file(state_file(p)));
    string line;
    while (std::getline(in, line)) {
        auto kv = split(trim(line), '=');
        if (kv.size() != 2) continue;
        if (kv[0] == "start") s.start = std::atoll(kv[1].c_str());
        if (kv[0] == "hints") s.hints = std::atoi(kv[1].c_str());
        if (kv[0] == "blind") s.blind = kv[1] == "1";
    }
    return s;
}

static void save_state(const Problem& p, const State& s) {
    write_file(state_file(p), "start=" + std::to_string(s.start) + "\nhints=" + std::to_string(s.hints) +
                                  "\nblind=" + (s.blind ? "1" : "0") + "\n");
}

static int minutes_since(const State& s) {
    if (!s.active || s.start == 0) return 0;
    return static_cast<int>((static_cast<long long>(std::time(nullptr)) - s.start) / 60);
}

// ---------------------------------------------------------------- review log and scheduling

struct Row {
    string date, id, result;
    int interval = 0;
    string next;
    int minutes = 0;
};

static vector<Row> load_log() {
    vector<Row> rows;
    if (!fs::exists(log_file())) return rows;
    std::istringstream in(read_file(log_file()));
    string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        auto f = split(line, '\t');
        if (f.size() < 6) continue;
        rows.push_back({f[0], f[1], f[2], std::atoi(f[3].c_str()), f[4], std::atoi(f[5].c_str())});
    }
    return rows;
}

static std::map<string, Row> latest_by_id(const vector<Row>& rows) {
    std::map<string, Row> m;
    for (auto& r : rows) m[r.id] = r;
    return m;
}

// Interval rules from section 4.6 of the report: pass moves up 1 -> 3 -> 7 -> 14 -> 30 -> 60;
// a hint halves the interval; reading the solution means redo tomorrow.
static int next_interval(const vector<Row>& prev, const string& result) {
    static const int ladder[] = {1, 3, 7, 14, 30, 60, 120};
    if (prev.empty()) return result == "pass" ? 3 : 1;
    int last = prev.back().interval;
    if (result == "pass") {
        for (int x : ladder) if (x > last) return x;
        return last * 2;
    }
    if (result == "hint") return std::max(1, last / 2);
    return 1;
}

static bool retired(const Row& r) { return r.result == "pass" && r.interval >= 60; }

// ---------------------------------------------------------------- review card

static const char* kCardFields[] = {"[複述]", "[限制→複雜度]", "[暴力解]", "[瓶頸]", "[觸發]",
                                    "[關鍵洞見]", "[不變量]", "[複雜度]", "[卡在哪]"};

static std::map<string, string> read_card(const fs::path& file) {
    std::map<string, string> card;
    std::istringstream in(read_file(file));
    string line;
    while (std::getline(in, line)) {
        string t = trim(line);
        for (const char* f : kCardFields) {
            string tag = f;
            if (t.rfind(tag, 0) == 0) card[tag] = trim(t.substr(tag.size()));
        }
    }
    return card;
}

static std::optional<fs::path> latest_attempt(const Problem& p) {
    if (!fs::exists(history_dir(p))) return std::nullopt;
    std::optional<fs::path> best;
    for (auto& e : fs::directory_iterator(history_dir(p)))
        if (!best || e.path().filename() > best->filename()) best = e.path();
    return best;
}

// ---------------------------------------------------------------- compiling

static string cxx() {
    const char* e = std::getenv("PP_CXX");
    return e && *e ? e : "g++";
}

static string compile_flags(bool debug) {
    // -Wno-unused-parameter: the stubs leave parameters unused until you write the solution.
    string f = " -std=c++20 -Wall -Wextra -Wshadow -Wno-unused-parameter";
    f += debug ? " -O0 -g -D_GLIBCXX_DEBUG" : " -O2 -g -D_GLIBCXX_ASSERTIONS";
#ifndef _WIN32
    if (!debug) f += " -fsanitize=address,undefined";
#endif
    return f;
}

static string link_flags() {
#ifdef _WIN32
    // Windows defaults to a 1 MB stack; judges give far more. -static avoids picking up another
    // toolchain's libstdc++ DLL from PATH (Git for Windows ships its own).
    return " -static -Wl,--stack,268435456";
#else
    return "";
#endif
}

// Precompiles leetcode.hpp, so each test build takes about a second instead of about five.
static fs::path ensure_pch(bool debug) {
    fs::path dir = g_root / "build" / (debug ? "pch-debug" : "pch-test");
    fs::path gch = dir / "leetcode.hpp.gch";
    fs::path hdr = g_root / "include" / "leetcode.hpp";
    fs::path judge = g_root / "include" / "judge.hpp";
    std::error_code ec;
    bool stale = !fs::exists(gch) || fs::last_write_time(gch, ec) < fs::last_write_time(hdr, ec) ||
                 fs::last_write_time(gch, ec) < fs::last_write_time(judge, ec);
    if (stale) {
        fs::create_directories(dir);
        std::cout << "（第一次使用：預先編譯 leetcode.hpp，約 10 秒）\n" << std::flush;
        run(cxx() + compile_flags(debug) + " -x c++-header " + quote(hdr) + " -o " + quote(gch));
    }
    return dir;
}

static bool compile_problem(const Problem& p, bool debug, fs::path& exe) {
    fs::create_directories(g_root / "build");
    exe = g_root / "build" / (p.id + (debug ? "-debug.exe" : ".exe"));
    string cmd = cxx() + compile_flags(debug);
    if (is_io(p)) {
        cmd += " " + quote(solution_file(p));
    } else {
        fs::path pch = ensure_pch(debug);
        cmd += " -I" + quote(pch) + " -I" + quote(g_root / "include") + " " + quote(problem_dir(p) / "test.cpp");
    }
    cmd += " -o " + quote(exe) + link_flags();
    if (run(cmd) != 0) {
        std::cout << "\n編譯失敗：先看第一個 error，後面的常是連鎖反應。\n";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- shared messages

static void after_run(const Problem& p, bool ok, int rc) {
    State s = load_state(p);
    int mins = minutes_since(s);
    std::cout << "\n";
    if (ok) {
        std::cout << "✓ 全部通過";
        if (s.active) std::cout << "（用時 " << mins << " 分鐘，目標 " << target_minutes(p.level) << " 分鐘）";
        std::cout << "\n下一步：\n"
                     "  1. 在 test.cpp 補一個你自己想到、測資還沒涵蓋的邊界情況\n"
                     "  2. 填好 [觸發] 與 [關鍵洞見]：記洞見，不記程式碼\n"
                     "  3. 記錄結果：pp done "
                  << p.id << " pass｜hint｜solution\n";
        return;
    }
    if (rc == 3)
        std::cout << "超過時間上限還沒結束：多半是指數時間（例如沒有記憶化的遞迴）或無窮迴圈。"
                     "先想複雜度，或用小輸入手算一次；要追蹤就按 F5，在迴圈裡設中斷點。\n";
    else if (rc != 1)
        std::cout << "程式崩潰（越界存取、空指標或遞迴太深）。按 F5 用除錯版重跑，_GLIBCXX_DEBUG 會停在越界的那一行。\n";
    else
        std::cout << "未通過：先看最小反例並手算，找出不變量在哪一步被破壞，再改程式。不要靠猜改到通過。\n";
    if (s.active && mins >= 20 && s.hints == 0)
        std::cout << "已經 " << mins << " 分鐘了。依時間盒規則：pp hint 看第一層提示（不是直接看解答）。\n";
}

// ---------------------------------------------------------------- commands

static int cmd_start(const Problem& p, bool blind) {
    fs::path sol = solution_file(p), stub = stub_file(p);
    if (!fs::exists(stub)) { std::cout << "缺少 " << ustr(stub) << "\n"; return 1; }
    if (fs::exists(sol) && read_file(sol) != read_file(stub)) {
        fs::path dst = history_dir(p) / (stamp() + sol.extension().string());
        fs::create_directories(dst.parent_path());
        fs::copy_file(sol, dst, fs::copy_options::overwrite_existing);
        std::cout << "上一版已封存到 progress/history/" << p.id << "/（VS Code 已隱藏這個資料夾，重做時不要去翻）\n";
    }
    fs::copy_file(stub, sol, fs::copy_options::overwrite_existing);
    State s;
    s.active = true;
    s.start = static_cast<long long>(std::time(nullptr));
    s.blind = blind;
    save_state(p, s);

    std::cout << "══ 開始 " << p.id << "（" << level_name(p.level) << "，目標 " << target_minutes(p.level)
              << " 分鐘）\n題目：" << p.url << "\n";
    if (blind) std::cout << "模式：已隱藏，先自己判斷，再動手寫\n";
    else std::cout << "模式：" << p.pattern << "（第 " << p.week << " 週）\n";
    std::cout << "流程：\n"
                 "  1. 填 [複述] 和 [限制→複雜度]：由 n 的上限推出目標複雜度\n"
                 "  2. 講出暴力解，找出瓶頸，再用觸發條件選模式\n"
                 "  3. 先在 [不變量] 寫一句話再寫程式，並在 test.cpp 加兩個自己的邊界測資\n"
                 "  4. 15–20 分鐘沒進展就 pp hint，不要直接看解答\n"
                 "打開："
              << ustr(fs::relative(sol, g_root)) << "\n";
    const char* term = std::getenv("TERM_PROGRAM");
    if (term && string(term) == "vscode") run("code -r " + quote(sol) + " >" + kNull + " 2>&1");
    return 0;
}

static int io_test(const Problem& p, fs::path exe) {
    fs::path dir = problem_dir(p), tests = dir / "tests";
    if (fs::exists(dir / "gen.cpp") && !fs::exists(tests / "big.in")) {
        fs::path gen = g_root / "build" / (p.id + "-gen.exe");
        std::cout << "產生大型測資 tests/big.in…\n" << std::flush;
        if (run(cxx() + " -std=c++20 -O2 " + quote(dir / "gen.cpp") + " -o " + quote(gen)) != 0 ||
            run(quote(gen) + " > " + quote(tests / "big.in")) != 0) {
            std::cout << "產生大型測資失敗\n";
            return 1;
        }
    }
    vector<fs::path> inputs;
    for (auto& e : fs::directory_iterator(tests))
        if (e.path().extension() == ".in") inputs.push_back(e.path());
    std::sort(inputs.begin(), inputs.end(), [](const fs::path& a, const fs::path& b) {
        auto ka = a.stem().string(), kb = b.stem().string();
        return ka.size() != kb.size() ? ka.size() < kb.size() : ka < kb;
    });
    const double limit_ms = 2000;
    int failed = 0;
    fs::path out = g_root / "build" / (p.id + ".out");
    for (auto& in : inputs) {
        auto t0 = std::chrono::steady_clock::now();
        int rc = run(quote(exe) + " < " + quote(in) + " > " + quote(out));
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        fs::path expected = in;
        expected.replace_extension(".out");
        string verdict, detail;
        if (rc != 0) {
            verdict = "RE";
            detail = "exit code " + std::to_string(rc);
        } else if (ms > limit_ms) {
            verdict = "TLE";
        } else if (!fs::exists(expected)) {
            verdict = "TIME";
            detail = "只檢查時間，沒有標準答案";
        } else {
            std::istringstream a(read_file(out)), b(read_file(expected));
            string x, y;
            size_t idx = 0;
            verdict = "AC";
            for (;;) {
                bool ha = static_cast<bool>(a >> x), hb = static_cast<bool>(b >> y);
                if (!ha && !hb) break;
                if (ha != hb || x != y) {
                    verdict = "WA";
                    detail = "第 " + std::to_string(idx + 1) + " 個輸出值：期望 " + (hb ? y : "(沒有)") + "，得到 " +
                             (ha ? x : "(沒有)");
                    break;
                }
                ++idx;
            }
        }
        if (verdict != "AC" && verdict != "TIME") ++failed;
        char line[256];
        std::snprintf(line, sizeof line, "  %-10s %-4s %7.0f ms  ", in.filename().string().c_str(), verdict.c_str(), ms);
        std::cout << line << detail << "\n";
    }
    std::cout << "\n" << (inputs.size() - static_cast<size_t>(failed)) << "/" << inputs.size() << " 通過（每個測資時限 "
              << limit_ms << " ms，含啟動時間）\n";
    if (failed)
        std::cout << "HackerRank 隱藏測資檢查清單：n = 1？最大的 n 會不會逾時？總和是否超過 int（改用 long long）？"
                     "全部相同或已排序？輸入格式是否完全照題目讀？\n";
    after_run(p, failed == 0, failed == 0 ? 0 : 1);
    return failed == 0 ? 0 : 1;
}

static int cmd_test(const Problem& p, const vector<string>& extra) {
    if (!fs::exists(solution_file(p))) {
        std::cout << "還沒開始這題：先執行 pp start " << p.id << "（VS Code 工作：pp: 開始/重做這題）\n";
        return 1;
    }
    fs::path exe;
    if (!compile_problem(p, false, exe)) return 1;
    if (is_io(p)) return io_test(p, exe);
    string cmd = quote(exe);
    for (auto& a : extra) cmd += " \"" + a + "\"";
    int rc = run(cmd);
    after_run(p, rc == 0, rc);
    return rc == 0 ? 0 : 1;
}

static int cmd_debug(const Problem& p) {
    if (!fs::exists(solution_file(p))) {
        std::cout << "還沒開始這題：先執行 pp start " << p.id << "\n";
        return 1;
    }
    fs::path exe;
    if (!compile_problem(p, true, exe)) return 1;
    std::cout << "除錯版：" << ustr(fs::relative(exe, g_root)) << "\n";
    return 0;
}

static int cmd_hint(const Problem& p) {
    State s = load_state(p);
    if (!s.active) {
        s.active = true;
        s.start = static_cast<long long>(std::time(nullptr));
    }
    s.hints += 1;
    save_state(p, s);

    vector<string> layers;
    if (s.blind) layers.push_back("模式：" + p.pattern);
    layers.push_back("提示問題：" + p.hint);
    if (auto prev = latest_attempt(p)) {
        auto card = read_card(*prev);
        string t = card["[觸發]"], k = card["[關鍵洞見]"];
        if (!t.empty() || !k.empty()) layers.push_back("你上次的卡片：觸發「" + t + "」；關鍵洞見「" + k + "」");
    }
    layers.push_back("關鍵洞見：" + p.lesson +
                     "\n（這是最後一層。仍然卡住：讀 editorial，關掉之後從零重寫，結果記為 solution）");
    size_t k = std::min(static_cast<size_t>(s.hints), layers.size());
    std::cout << "提示 " << k << "/" << layers.size() << "\n" << layers[k - 1] << "\n";
    std::cout << "（用過提示，這次只能記 hint 或 solution；想好之後再試 10 分鐘）\n";
    return 0;
}

static int cmd_done(const Problem& p, string result, bool force) {
    if (result != "pass" && result != "hint" && result != "solution") {
        std::cout << "結果只能是 pass、hint 或 solution\n";
        return 1;
    }
    State s = load_state(p);
    if (!force && fs::exists(solution_file(p))) {
        auto card = read_card(solution_file(p));
        vector<string> missing;
        for (const char* f : {"[觸發]", "[關鍵洞見]"})
            if (card[f].empty()) missing.push_back(f);
        if (!missing.empty()) {
            std::cout << "卡片還沒填：";
            for (auto& m : missing) std::cout << m << " ";
            std::cout << "\n用一句話寫下次看到什麼就會想到這招，這是複習時唯一要回想的內容。"
                         "填完再執行一次（真的要跳過，就在終端機加 --force）。\n";
            return 1;
        }
    }
    if (s.hints > 0 && result == "pass") {
        result = "hint";
        std::cout << "這次用過 " << s.hints << " 次提示，改記為 hint（誠實記錄，排程才準）\n";
    }
    vector<Row> rows = load_log(), prev;
    for (auto& r : rows) if (r.id == p.id) prev.push_back(r);
    int interval = next_interval(prev, result);
    Days next = today() + std::chrono::days{interval};
    int mins = minutes_since(s);
    if (!fs::exists(log_file())) write_file(log_file(), "# date\tid\tresult\tinterval\tnext\tminutes\n");
    std::ofstream(log_file(), std::ios::app) << fmt_date(today()) << '\t' << p.id << '\t' << result << '\t' << interval
                                             << '\t' << fmt_date(next) << '\t' << mins << '\n';
    std::error_code ec;
    fs::remove(state_file(p), ec);

    Row last{fmt_date(today()), p.id, result, interval, fmt_date(next), mins};
    std::cout << "記錄：" << p.id << " " << result;
    if (s.active) std::cout << "，用時 " << mins << " 分鐘（目標 " << target_minutes(p.level) << "）";
    std::cout << "\n";
    if (retired(last)) std::cout << "已穩定：通過了 30 天以上的複習，之後不會出現在到期清單，但 mix 仍可能抽到。\n";
    else std::cout << "下次複習：" << fmt_date(next) << "（" << interval << " 天後）\n";
    std::cout << "── 本題學習要點 ──\n關鍵洞見：" << p.lesson << "\nC++ 要點：" << p.cpp << "\n";
    if (result != "pass")
        std::cout << "下次用 pp start " << p.id << " 從空白檔重寫，不要看今天的程式碼。重寫得出來才算學會。\n";
    return 0;
}

static vector<Row> due_rows() {
    vector<Row> due;
    Days t = today();
    for (auto& [id, r] : latest_by_id(load_log()))
        if (!retired(r) && parse_date(r.next) <= t) due.push_back(r);
    std::sort(due.begin(), due.end(), [](const Row& a, const Row& b) { return a.next < b.next; });
    return due;
}

static int cmd_due() {
    auto due = due_rows();
    if (due.empty()) { std::cout << "今天沒有到期的複習。\n"; return 0; }
    Days t = today();
    std::cout << "到期複習（從空白檔重寫，不看舊程式）：\n";
    for (auto& r : due) {
        long long late = (t - parse_date(r.next)).count();
        std::cout << "  pp start " << r.id << "    上次 " << r.result << "，" << (late > 0 ? "逾期 " + std::to_string(late) + " 天" : "今天到期") << "\n";
    }
    return 0;
}

static int cmd_today() {
    for (auto& p : g_catalog) {
        State s = load_state(p);
        if (s.active) std::cout << "進行中：" << p.id << "（已 " << minutes_since(s) << " 分鐘）→ 先完成它，再 pp done\n";
    }
    auto due = due_rows();
    auto latest = latest_by_id(load_log());
    std::cout << "今日練習（先複習，再做新題）：\n";
    size_t shown = 0;
    for (auto& r : due) {
        if (shown == 2) break;
        std::cout << "  複習  pp start " << r.id << "\n";
        ++shown;
    }
    if (due.size() > 3) {
        std::cout << "  積壓 " << due.size() << " 題到期：今天只做複習，不開新題（新題只會讓積壓更多）。\n";
        return 0;
    }
    for (auto& p : g_catalog) {
        if (latest.count(p.id) || load_state(p).active) continue;
        std::cout << "  新題  pp start " << p.id << "    第 " << p.week << " 週，" << p.pattern << "，"
                  << level_name(p.level) << "\n";
        return 0;
    }
    std::cout << "  題單裡的新題都做過了：用 pp mix 做交錯練習，或用 pp new 加題（例如 Grind 169 的下一題）。\n";
    return 0;
}

static int cmd_mix(int n) {
    vector<string> learned;
    for (auto& [id, r] : latest_by_id(load_log())) learned.push_back(id);
    if (learned.empty()) { std::cout << "還沒有做過的題目。先照 pp today 做新題。\n"; return 0; }
    std::mt19937 g(std::random_device{}());
    std::shuffle(learned.begin(), learned.end(), g);
    learned.resize(std::min(learned.size(), static_cast<size_t>(n)));
    std::cout << "交錯練習：模式已隱藏。先判斷要用哪一招，再動手。\n";
    for (auto& id : learned) std::cout << "  pp start " << id << " --blind\n";
    return 0;
}

static int cmd_stats() {
    struct Agg { int n = 0, pass = 0, hint = 0, sol = 0, minutes = 0; };
    std::map<string, Agg> by;
    std::map<string, string> pattern_of;
    for (auto& p : g_catalog) pattern_of[p.id] = p.pattern;
    auto rows = load_log();
    if (rows.empty()) { std::cout << "還沒有紀錄。\n"; return 0; }
    for (auto& r : rows) {
        Agg& a = by[pattern_of.count(r.id) ? pattern_of[r.id] : "(未登錄)"];
        a.n++;
        a.minutes += r.minutes;
        if (r.result == "pass") a.pass++;
        else if (r.result == "hint") a.hint++;
        else a.sol++;
    }
    vector<std::pair<string, Agg>> v(by.begin(), by.end());
    auto weakness = [](const Agg& a) { return (a.hint + 2.0 * a.sol) / a.n; };
    std::sort(v.begin(), v.end(), [&](auto& x, auto& y) { return weakness(x.second) > weakness(y.second); });
    std::cout << "依弱項排序（hint 算 1、看解答算 2，取平均）：\n";
    for (auto& [pat, a] : v) {
        char line[256];
        std::snprintf(line, sizeof line, "  %5.2f  次數 %-3d pass %-3d hint %-3d solution %-3d 平均 %d 分  ", weakness(a), a.n,
                      a.pass, a.hint, a.sol, a.minutes / a.n);
        std::cout << line << pat << "\n";
    }
    auto latest = latest_by_id(rows);
    int stable = 0;
    for (auto& [id, r] : latest) stable += retired(r);
    std::cout << "做過 " << latest.size() << "/" << g_catalog.size() << " 題，已穩定 " << stable
              << " 題。弱項前兩名：用 pp new 多加幾題同模式的題目。\n";
    return 0;
}

static int cmd_list() {
    auto latest = latest_by_id(load_log());
    Days t = today();
    for (auto& p : g_catalog) {
        string status = "新題";
        if (load_state(p).active) status = "進行中";
        else if (latest.count(p.id)) {
            const Row& r = latest[p.id];
            if (retired(r)) status = "已穩定";
            else if (parse_date(r.next) <= t) status = "到期（上次 " + r.result + "）";
            else status = "下次 " + r.next + "（上次 " + r.result + "）";
        }
        char line[256];
        std::snprintf(line, sizeof line, "  W%-2d %-6s %-48s ", p.week, level_name(p.level).c_str(), p.id.c_str());
        std::cout << line << status << "\n";
    }
    return 0;
}

static string replace_all(string s, const string& from, const string& to) {
    for (size_t pos = 0; (pos = s.find(from, pos)) != string::npos; pos += to.size()) s.replace(pos, from.size(), to);
    return s;
}

static int cmd_new(const vector<string>& a) {
    if (a.size() < 6) {
        std::cout << "用法：pp new <id> <fn|io> <模式> <網址> [E|M|H] [週]\n"
                     "例：pp new lc0076-minimum-window-substring fn 滑動視窗 https://leetcode.com/problems/minimum-window-substring/ H 2\n";
        return 1;
    }
    string id = a[2], kind = a[3], pattern = a[4], url = a[5];
    string level = a.size() > 6 ? a[6] : "M", week = a.size() > 7 ? a[7] : "9";
    if (kind != "fn" && kind != "io") { std::cout << "種類只能是 fn（LeetCode 函式題）或 io（HackerRank 讀 STDIN）\n"; return 1; }
    for (auto& p : g_catalog) if (p.id == id) { std::cout << "已經有 " << id << "\n"; return 1; }
    fs::path from = g_root / "templates" / kind, to = g_root / "problems" / id;
    for (auto& e : fs::recursive_directory_iterator(from)) {
        if (!e.is_regular_file()) continue;
        fs::path dst = to / fs::relative(e.path(), from);
        write_file(dst, replace_all(replace_all(read_file(e.path()), "{{ID}}", id), "{{URL}}", url));
    }
    std::ofstream(g_root / "problems" / "catalog.tsv", std::ios::app)
        << id << '\t' << kind << '\t' << pattern << '\t' << level << '\t' << week << '\t' << url
        << "\t（自己寫一個引導問題）\t（做完後寫下關鍵洞見）\t（做完後寫下 C++ 要點）\n";
    std::cout << "已建立 problems/" << id << "/\n";
    if (kind == "fn")
        std::cout << "接著：在 stub.hpp 貼上 LeetCode 的函式簽章，在 test.cpp 寫自己的測資和暴力 oracle，再 pp start " << id << "\n";
    else
        std::cout << "接著：在 tests/ 放 1.in、1.out（自己出的測資），再 pp start " << id << "\n";
    std::cout << "記得事後回 problems/catalog.tsv 補上提示問題、關鍵洞見和 C++ 要點。\n";
    return 0;
}

static void usage() {
    std::cout << "pp：刷題練習迴圈\n"
                 "  pp today                    今天做什麼：先到期複習，再一題新題\n"
                 "  pp start <題> [--blind]     開始或重做：封存舊版，還原空白檔（--blind 隱藏模式）\n"
                 "  pp test <題> [篩選] [--seed=N] [--timeout=秒]  編譯並跑測試（HackerRank 題跑 tests/*.in）\n"
                 "  pp hint <題>                分層提示，每次多一層\n"
                 "  pp done <題> pass|hint|solution [--force]  記錄結果並排下次複習\n"
                 "  pp due                      列出到期複習\n"
                 "  pp mix [n]                  交錯練習：隨機抽 n 題做過的題（預設 3）\n"
                 "  pp stats                    依模式統計弱項\n"
                 "  pp list                     全部題目與狀態\n"
                 "  pp debug <題>               建置除錯版（給 F5 用）\n"
                 "  pp new <id> <fn|io> <模式> <網址> [E|M|H] [週]  新增題目\n"
                 "<題> 可以是題號（739）、id，或題目資料夾裡任一檔案的路徑。\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    vector<string> args = utf8_args(argc, argv);
    if (!find_root(args[0])) {
        std::cout << "找不到 problems/catalog.tsv。請在 VS Code 用「開啟資料夾」打開 cpp-practice。\n";
        return 1;
    }
    {
        std::error_code ec;
        fs::path self = fs::weakly_canonical(upath(args[0]), ec);
        fs::path src = g_root / "tools" / "pp.cpp";
        if (!ec && fs::exists(self) && fs::exists(src) && fs::last_write_time(src, ec) > fs::last_write_time(self, ec))
            std::cout << "（tools/pp.cpp 比 pp.exe 新：執行工作「pp: 建置工具」重新建置）\n";
    }
    load_catalog();
    if (args.size() < 2 || args[1] == "help" || args[1] == "--help") { usage(); return 0; }
    const string& cmd = args[1];
    if (cmd == "today") return cmd_today();
    if (cmd == "due") return cmd_due();
    if (cmd == "stats") return cmd_stats();
    if (cmd == "list") return cmd_list();
    if (cmd == "new") return cmd_new(args);
    if (cmd == "mix") return cmd_mix(args.size() > 2 ? std::max(1, std::atoi(args[2].c_str())) : 3);
    const Problem* p = need_problem(args, 2);
    if (!p) return 1;
    if (cmd == "start") return cmd_start(*p, std::find(args.begin(), args.end(), "--blind") != args.end());
    if (cmd == "test") return cmd_test(*p, vector<string>(args.begin() + 3, args.end()));
    if (cmd == "debug") return cmd_debug(*p);
    if (cmd == "hint") return cmd_hint(*p);
    if (cmd == "done") {
        if (args.size() < 4) { std::cout << "用法：pp done <題> pass|hint|solution\n"; return 1; }
        return cmd_done(*p, args[3], std::find(args.begin(), args.end(), "--force") != args.end());
    }
    usage();
    return 1;
}
