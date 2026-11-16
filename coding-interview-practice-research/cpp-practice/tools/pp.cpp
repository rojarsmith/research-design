// pp.cpp: practice-loop CLI. It needs nothing beyond g++.
// Build: g++ -std=c++20 -O2 tools/pp.cpp -o build/pp.exe  (VS Code task "pp: 重新建置工具")
//
// Each command enforces one rule from the research report:
//   start  blank-file retrieval: archive the previous attempt, restore an empty stub
//   test   compile + run tests; reminds you of the 15-20 minute time box
//   hint   layered hints; using one turns this attempt's "pass" into "hint"
//   done   requires the review card ([觸發] and [關鍵洞見]) and schedules the next review
//   today  due reviews first, then one new problem; no new problem while the backlog is large
//   mix    interleaving: random solved problems with the pattern hidden
//   stats  weakest patterns first
//   mock   timed mock interview: one clock, problems revealed one at a time with pattern and
//          level hidden, no hints, and a scored self-review before anything is recorded
//   ready  readiness against the report's standard (sections 4.2 and 9)
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
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

// One line of free text for a TSV cell.
static string one_line(string s) {
    for (char& c : s)
        if (c == '\t' || c == '\r' || c == '\n') c = ' ';
    return trim(s);
}

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

// Opens a file in the editor when pp runs inside a VS Code terminal or task.
static void open_in_editor(const fs::path& f) {
    const char* term = std::getenv("TERM_PROGRAM");
    if (term && string(term) == "vscode") run("code -r " + quote(f) + " >" + kNull + " 2>&1");
}

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

static long long now_s() { return static_cast<long long>(std::time(nullptr)); }

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

static const Problem* by_id(const string& id) {
    for (auto& p : g_catalog)
        if (p.id == id) return &p;
    return nullptr;
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
    string mock;  // id of the mock session this attempt belongs to; empty for normal practice
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
        if (kv[0] == "mock") s.mock = kv[1];
    }
    return s;
}

static void save_state(const Problem& p, const State& s) {
    write_file(state_file(p), "start=" + std::to_string(s.start) + "\nhints=" + std::to_string(s.hints) +
                                  "\nblind=" + (s.blind ? "1" : "0") + "\n" +
                                  (s.mock.empty() ? "" : "mock=" + s.mock + "\n"));
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

// Rows from pp done have six columns; rows from pp mock end add a seventh, "mock".
static void append_review(const Row& r, const string& source) {
    if (!fs::exists(log_file())) write_file(log_file(), "# date\tid\tresult\tinterval\tnext\tminutes\tsource\n");
    std::ofstream out(log_file(), std::ios::app);
    out << r.date << '\t' << r.id << '\t' << r.result << '\t' << r.interval << '\t' << r.next << '\t' << r.minutes;
    if (!source.empty()) out << '\t' << source;
    out << '\n';
}

static std::map<string, Row> latest_by_id(const vector<Row>& rows) {
    std::map<string, Row> m;
    for (auto& r : rows) m[r.id] = r;
    return m;
}

// Interval rules from section 4.6 of the report: pass moves up 1 -> 3 -> 7 -> 14 -> 30 -> 60;
// a hint halves the interval; reading the solution means redo tomorrow. A problem failed in a
// mock interview ("fail") is due the same day, with interval 0 (see record_mock).
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

// Reads "[tag] text" lines; the text must be on the same line as its tag.
static std::map<string, string> read_fields(const fs::path& file, const vector<string>& tags) {
    std::map<string, string> out;
    std::istringstream in(read_file(file));
    string line;
    while (std::getline(in, line)) {
        string t = trim(line);
        for (auto& tag : tags)
            if (t.rfind(tag, 0) == 0) out[tag] = trim(t.substr(tag.size()));
    }
    return out;
}

static std::map<string, string> read_card(const fs::path& file) {
    return read_fields(file, vector<string>(std::begin(kCardFields), std::end(kCardFields)));
}

static std::optional<fs::path> latest_attempt(const Problem& p) {
    if (!fs::exists(history_dir(p))) return std::nullopt;
    std::optional<fs::path> best;
    for (auto& e : fs::directory_iterator(history_dir(p)))
        if (!best || e.path().filename() > best->filename()) best = e.path();
    return best;
}

// ---------------------------------------------------------------- mock interviews: session state
//
// A mock is one clock for a few problems. They are revealed one at a time, the way an interviewer
// gives the next question, so each problem's time runs from its reveal. A problem's result is its
// last test run before time ran out (or before pp mock next moved on). Files:
//   progress/mocks/active.txt   the session in progress
//   progress/mocks/<id>.txt     its self-review, which pp mock end checks before recording
//   progress/mocks.tsv          one row per problem of every recorded session (pp ready reads it)

struct MockItem {
    string id;
    bool fresh = false;     // never attempted before this session
    long long shown = 0;    // when it was revealed; 0 = not reached
    long long left = 0;     // when pp mock next moved on from it
    string status;          // "", "pass" or "fail": the last counted test run
    long long pass_at = 0;  // when status last turned to pass
    int runs = 0;
    bool late = false;      // passed only after time ran out
};

struct MockSession {
    string id;
    long long start = 0, ended = 0;
    int limit = 45;   // minutes
    int minute = 60;  // seconds per minute; PP_MOCK_MINUTE shortens it to test pp itself
    vector<MockItem> items;
};

struct RubricItem { const char* tag; const char* question; };

// What interviewers score. The tag, without brackets, is the short name shown in reports.
static const RubricItem kRubric[] = {
    {"[澄清]", "動手前有沒有用自己的話複述題目，問輸入範圍和邊界（空輸入、重複值、極值）？"},
    {"[思路]", "寫程式前有沒有先講暴力解和它的複雜度，再講怎麼優化、為什麼選這個模式？"},
    {"[正確]", "程式是不是一次就寫得差不多對、結構清楚，而不是靠反覆跑測試改到通過？"},
    {"[測試]", "有沒有自己出邊界測資、手動走一遍程式，在跑測試之前就找到 bug？"},
    {"[溝通]", "有沒有邊寫邊講？卡住時有沒有說出卡在哪、正在考慮什麼？"},
};
static const char* kImprove = "[下次改進]";
static const char* kPartner = "[夥伴或錄影]";

// A mock passes when every problem passed its tests in time and no rubric item scored 1.
static const int kPassMinScore = 2;

// Readiness thresholds for pp ready, from the research report:
//   kReadyMocks    sections 1, 6.2 and 9: at least 5 mock interviews
//   kReadyStreak   sections 4.2 and 9: several consecutive mocks passed, not just one (the report
//                  gives no number; 3 is this project's reading)
//   kReadyMedium*  section 4.2: most unseen Mediums solved in 20-25 minutes (NeetCode's benchmark);
//                  "most" is read as 70%, against the upper end of 25 minutes
static const int kReadyMocks = 5, kReadyStreak = 3, kReadyWindow = 5;
static const int kReadyMediumMinutes = 25, kReadyMediumPercent = 70;

static fs::path mock_dir() { return g_root / "progress" / "mocks"; }
static fs::path mock_file() { return mock_dir() / "active.txt"; }
static fs::path review_file(const string& id) { return mock_dir() / (id + ".txt"); }
static fs::path mock_log_file() { return g_root / "progress" / "mocks.tsv"; }

static string rubric_name(const RubricItem& r) {
    string t = r.tag;
    return t.substr(1, t.size() - 2);
}

static std::optional<MockSession> load_mock() {
    if (!fs::exists(mock_file())) return std::nullopt;
    MockSession m;
    std::istringstream in(read_file(mock_file()));
    string line;
    while (std::getline(in, line)) {
        line = trim(line);
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (k == "id") m.id = v;
        else if (k == "start") m.start = std::atoll(v.c_str());
        else if (k == "ended") m.ended = std::atoll(v.c_str());
        else if (k == "limit") m.limit = std::max(1, std::atoi(v.c_str()));
        else if (k == "minute") m.minute = std::max(1, std::atoi(v.c_str()));
        else if (k == "item") {
            auto f = split(v, ',');
            if (f.size() < 8) continue;
            MockItem it;
            it.id = f[0];
            it.fresh = f[1] == "new";
            it.shown = std::atoll(f[2].c_str());
            it.left = std::atoll(f[3].c_str());
            it.status = f[4] == "-" ? "" : f[4];
            it.pass_at = std::atoll(f[5].c_str());
            it.runs = std::atoi(f[6].c_str());
            it.late = f[7] == "1";
            m.items.push_back(it);
        }
    }
    if (m.id.empty() || m.start == 0 || m.items.empty()) return std::nullopt;
    return m;
}

static void save_mock(const MockSession& m) {
    std::ostringstream o;
    o << "id=" << m.id << "\nstart=" << m.start << "\nended=" << m.ended << "\nlimit=" << m.limit
      << "\nminute=" << m.minute << "\n";
    for (auto& it : m.items)
        o << "item=" << it.id << ',' << (it.fresh ? "new" : "seen") << ',' << it.shown << ',' << it.left << ','
          << (it.status.empty() ? "-" : it.status) << ',' << it.pass_at << ',' << it.runs << ',' << (it.late ? 1 : 0)
          << "\n";
    write_file(mock_file(), o.str());
}

static long long mock_deadline(const MockSession& m) { return m.start + static_cast<long long>(m.limit) * m.minute; }
static bool mock_time_up(const MockSession& m) { return now_s() >= mock_deadline(m); }
// The clock stops at pp mock end or when time runs out, whichever comes first.
static long long mock_stop(const MockSession& m) { return std::min(m.ended ? m.ended : now_s(), mock_deadline(m)); }
static int mock_min(const MockSession& m, long long from, long long to) {
    return to > from ? static_cast<int>((to - from) / m.minute) : 0;
}
// Rounded up, so "0 minutes left" only appears once time is up.
static int mock_left_min(const MockSession& m) {
    long long s = mock_deadline(m) - now_s();
    return s > 0 ? static_cast<int>((s + m.minute - 1) / m.minute) : 0;
}
static int mock_budget(const MockSession& m) { return m.limit / static_cast<int>(m.items.size()); }
static int mock_current(const MockSession& m) {
    int k = 0;
    for (size_t i = 0; i < m.items.size(); ++i)
        if (m.items[i].shown) k = static_cast<int>(i);
    return k;
}
static int mock_index(const MockSession& m, const string& id) {
    for (size_t i = 0; i < m.items.size(); ++i)
        if (m.items[i].id == id) return static_cast<int>(i);
    return -1;
}
static bool in_mock(const MockSession& m, const string& id) {
    int k = mock_index(m, id);
    return k >= 0 && m.items[static_cast<size_t>(k)].shown != 0;
}

// Minutes a problem took: until it passed, or until pp mock next, the end, or the time limit.
static int item_minutes(const MockSession& m, const MockItem& it) {
    if (!it.shown) return 0;
    if (it.status == "pass") return mock_min(m, it.shown, it.pass_at);
    return mock_min(m, it.shown, it.left ? std::min(it.left, mock_deadline(m)) : mock_stop(m));
}

static string item_result(const MockItem& it) {
    if (!it.shown) return "none";
    if (it.status == "pass") return "pass";
    return it.late ? "late" : "fail";
}

static string result_text(const string& r) {
    if (r == "pass") return "通過";
    if (r == "late") return "時間到之後才通過（不算）";
    if (r == "none") return "沒輪到";
    return "未通過";
}

// Practice mode says "20 minutes without progress: take a hint". An interview has no hints, so a
// mock replaces that nudge with checkpoints scaled to this session's clock.
static string mock_advice(const MockSession& m) {
    if (m.ended) return "計時已經停止：填好檢討表，再執行一次 pp mock end。";
    if (mock_time_up(m))
        return "時間到：停止寫程式，執行 pp mock end（VS Code 工作：pp: 結束模擬面試）寫檢討。之後的測試結果不算進這場。";
    size_t k = static_cast<size_t>(mock_current(m));
    const MockItem& it = m.items[k];
    int left = mock_left_min(m), used = mock_min(m, it.shown, now_s()), budget = mock_budget(m);
    bool last = k + 1 == m.items.size();
    if (left <= 5 && m.limit > 5)
        return "只剩 " + std::to_string(left) +
               " 分鐘：不要再大改。確定現在的版本能編譯、能跑，跑一次極端測資，口頭講完還差什麼（研究報告第 8 節）。";
    if (it.status != "pass" && used >= budget && !last)
        return "這題已經 " + std::to_string(used) + " 分鐘，用完了平均每題的 " + std::to_string(budget) +
               " 分鐘。面試裡沒有提示可看：先寫出一個正確的暴力解拿部分分數（第 8 節），講清楚卡在哪、打算怎麼優化；"
               "再撐下去會壓縮下一題，可以用 pp mock next 換題。";
    if (it.status != "pass" && used >= budget)
        return "這題已經 " + std::to_string(used) + " 分鐘：先寫出一個正確的暴力解，再講怎麼優化。";
    return "";
}

// ---------------------------------------------------------------- mock interviews: records

struct MockRow {
    string id, level, fresh, result;
    int minutes = 0;
};

struct MockRecord {
    string date, session, improve, note;
    int limit = 0, used = 0;
    vector<int> rubric;
    vector<MockRow> rows;
    bool passed = false;
};

static vector<MockRecord> load_mock_records() {
    vector<MockRecord> out;
    if (!fs::exists(mock_log_file())) return out;
    std::istringstream in(read_file(mock_log_file()));
    string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        auto f = split(line, '\t');
        if (f.size() < 10) continue;
        if (out.empty() || out.back().session != f[1]) {
            MockRecord rec;
            rec.date = f[0];
            rec.session = f[1];
            rec.limit = std::atoi(f[7].c_str());
            rec.used = std::atoi(f[8].c_str());
            for (auto& x : split(f[9], ',')) rec.rubric.push_back(std::atoi(x.c_str()));
            if (f.size() > 10) rec.improve = f[10];
            if (f.size() > 11) rec.note = f[11];
            out.push_back(rec);
        }
        out.back().rows.push_back({f[2], f[3], f[4], f[5], std::atoi(f[6].c_str())});
    }
    for (auto& rec : out) {
        bool all = true;
        for (auto& r : rec.rows) all = all && r.result == "pass";
        int lo = rec.rubric.empty() ? 0 : *std::min_element(rec.rubric.begin(), rec.rubric.end());
        rec.passed = all && lo >= kPassMinScore;
    }
    return out;
}

struct Readiness {
    int mocks = 0, streak = 0, recent = 0, recent_passed = 0;
    int medium = 0, medium_fast = 0, fresh_medium = 0, fresh_fast = 0;
    string marks;  // last kReadyWindow sessions, oldest first
    bool count_ok = false, streak_ok = false, medium_ok = false;
};

static Readiness assess(const vector<MockRecord>& recs) {
    Readiness r;
    r.mocks = static_cast<int>(recs.size());
    for (auto it = recs.rbegin(); it != recs.rend() && it->passed; ++it) ++r.streak;
    size_t from = recs.size() > static_cast<size_t>(kReadyWindow) ? recs.size() - kReadyWindow : 0;
    for (size_t i = from; i < recs.size(); ++i) {
        ++r.recent;
        r.recent_passed += recs[i].passed;
        r.marks += recs[i].passed ? "✓" : "✗";
        for (auto& row : recs[i].rows) {
            if (row.level != "M" || row.result == "none") continue;
            bool fast = row.result == "pass" && row.minutes <= kReadyMediumMinutes;
            ++r.medium;
            r.medium_fast += fast;
            if (row.fresh == "new") {
                ++r.fresh_medium;
                r.fresh_fast += fast;
            }
        }
    }
    r.count_ok = r.mocks >= kReadyMocks;
    r.streak_ok = r.streak >= kReadyStreak;
    r.medium_ok = r.medium > 0 && r.medium_fast * 100 >= r.medium * kReadyMediumPercent;
    return r;
}

static string ready_brief(const Readiness& r) {
    return "準備度：模擬面試 " + std::to_string(r.mocks) + "/" + std::to_string(kReadyMocks) + " 場，連續通過 " +
           std::to_string(r.streak) + "/" + std::to_string(kReadyStreak) + " 場，最近 " + std::to_string(kReadyWindow) +
           " 場的 Medium 在 " + std::to_string(kReadyMediumMinutes) + " 分鐘內通過 " + std::to_string(r.medium_fast) +
           "/" + std::to_string(r.medium) + (r.count_ok && r.streak_ok && r.medium_ok ? "，三項都達標" : "") +
           " → 詳見 pp ready";
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

static void failure_advice(int rc) {
    if (rc == 3)
        std::cout << "超過時間上限還沒結束：多半是指數時間（例如沒有記憶化的遞迴）或無窮迴圈。"
                     "先想複雜度，或用小輸入手算一次；要追蹤就按 F5，在迴圈裡設中斷點。\n";
    else if (rc != 1)
        std::cout << "程式崩潰（越界存取、空指標或遞迴太深）。按 F5 用除錯版重跑，_GLIBCXX_DEBUG 會停在越界的那一行。\n";
    else
        std::cout << "未通過：先看最小反例並手算，找出不變量在哪一步被破壞，再改程式。不要靠猜改到通過。\n";
}

// When pp test was started. Compiling takes a few seconds, and a run started before time ran out
// counts, like an answer handed in before the bell.
static long long g_test_started = 0;

// A test run during a mock counts only while the clock runs and the problem is the current one.
static void mock_after_run(MockSession m, size_t k, const Problem& p, bool ok, int rc) {
    MockItem& it = m.items[k];
    long long now = g_test_started ? g_test_started : now_s();
    bool frozen = m.ended != 0 || it.left != 0, over = now >= mock_deadline(m);
    if (!frozen && !over) {
        ++it.runs;
        if (!ok) {
            it.status = "fail";
            it.pass_at = 0;
        } else if (it.status != "pass") {
            it.status = "pass";
            it.pass_at = now;
        }
        save_mock(m);
    } else if (!frozen && ok && !it.late) {
        it.late = true;
        save_mock(m);
    }
    std::cout << "\n";
    if (frozen) {
        if (ok) std::cout << "✓ 全部通過\n";
        else failure_advice(rc);
        std::cout << "（這題在模擬面試裡已經結束：已換下一題或已停止計時，這次的結果不會改變紀錄）\n";
        return;
    }
    if (over) {
        if (ok) std::cout << "✓ 全部通過，但模擬面試的時間已經到了：這題記為時間到之後才通過，不算通過。\n";
        else failure_advice(rc);
        std::cout << mock_advice(m) << "\n";
        return;
    }
    size_t n = m.items.size();
    if (ok) {
        std::cout << "✓ 全部通過（模擬面試第 " << k + 1 << "/" << n << " 題，用時 " << item_minutes(m, it)
                  << " 分鐘；這場剩 " << mock_left_min(m) << " 分鐘）\n下一步：口頭講一次時間和空間複雜度，"
                  << (is_io(p) ? "在 tests/ 加一組自己出的極端測資（.in 和 .out），然後"
                               : "在 test.cpp 補一個你自己想到的邊界測資，然後")
                  << (k + 1 < n ? "執行 pp mock next 拿下一題（VS Code 工作：pp: 模擬面試下一題）。\n"
                                : "執行 pp mock end 停止計時、寫檢討（VS Code 工作：pp: 結束模擬面試）。\n");
        return;
    }
    failure_advice(rc);
    string a = mock_advice(m);
    if (a.empty())
        a = "模擬面試中沒有提示：用最小反例手算，邊做邊講你在檢查什麼。這題 " +
            std::to_string(mock_min(m, it.shown, now_s())) + " 分鐘，這場剩 " + std::to_string(mock_left_min(m)) + " 分鐘。";
    std::cout << a << "\n";
}

static void after_run(const Problem& p, bool ok, int rc) {
    if (auto m = load_mock(); m && in_mock(*m, p.id)) {
        mock_after_run(*m, static_cast<size_t>(mock_index(*m, p.id)), p, ok, rc);
        return;
    }
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
    failure_advice(rc);
    if (s.active && mins >= 20 && s.hints == 0)
        std::cout << "已經 " << mins << " 分鐘了。依時間盒規則：pp hint 看第一層提示（不是直接看解答）。\n";
}

// ---------------------------------------------------------------- commands

// Archives the current solution (unless it is still the blank stub) and restores the stub.
static void restore_blank(const Problem& p) {
    fs::path sol = solution_file(p), stub = stub_file(p);
    if (fs::exists(sol) && read_file(sol) != read_file(stub)) {
        fs::path dst = history_dir(p) / (stamp() + sol.extension().string());
        fs::create_directories(dst.parent_path());
        fs::copy_file(sol, dst, fs::copy_options::overwrite_existing);
        std::cout << "上一版已封存到 progress/history/" << p.id << "/（VS Code 已隱藏這個資料夾，重做時不要去翻）\n";
    }
    fs::copy_file(stub, sol, fs::copy_options::overwrite_existing);
}

static int cmd_start(const Problem& p, bool blind) {
    if (auto m = load_mock()) {
        std::cout << "模擬面試 " << m->id << " 還沒結束：先用 pp mock end 寫完檢討（或 pp mock abort 中止），再開始別的練習。\n";
        return 1;
    }
    fs::path sol = solution_file(p), stub = stub_file(p);
    if (!fs::exists(stub)) { std::cout << "缺少 " << ustr(stub) << "\n"; return 1; }
    restore_blank(p);
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
    open_in_editor(sol);
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
    g_test_started = now_s();
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
    if (auto m = load_mock()) {
        std::cout << "模擬面試 " << m->id << " 進行中，沒有提示：真正的面試也沒有。"
                  << (m->ended ? "先用 pp mock end 寫完檢討，"
                               : "卡住時先寫出正確的暴力解，講清楚卡在哪；要換題用 pp mock next。")
                  << "\n結束之後，沒通過的題目會排進今天的複習，到時候再用 pp hint。\n";
        return 1;
    }
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
    if (auto m = load_mock(); m && in_mock(*m, p.id)) {
        std::cout << "這題是模擬面試 " << m->id << " 的題目：結果由 pp mock end 記錄，也會一起排進複習。\n";
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
    Row last{fmt_date(today()), p.id, result, interval, fmt_date(next), mins};
    append_review(last, "");
    std::error_code ec;
    fs::remove(state_file(p), ec);

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

static string mock_brief(const MockSession& m) {
    if (m.ended) return "已停止計時，等你寫完檢討 → pp mock end";
    if (mock_time_up(m)) return "時間到 → pp mock end";
    return "第 " + std::to_string(mock_current(m) + 1) + "/" + std::to_string(m.items.size()) + " 題，剩 " +
           std::to_string(mock_left_min(m)) + " 分鐘 → pp mock status";
}

// Section 6.2 of the report: the first mock in weeks 5-6, then one every week.
static void suggest_mock() {
    int week = 0;
    auto latest = latest_by_id(load_log());
    for (auto& p : g_catalog)
        if (latest.count(p.id)) week = std::max(week, p.week);
    if (week < 5) return;
    auto recs = load_mock_records();
    if (!recs.empty() && (today() - parse_date(recs.back().date)).count() < 7) return;
    std::cout << "  模擬面試  pp mock    "
              << (recs.empty() ? "已經到第 " + std::to_string(week) + " 週，還沒做過" : "上一場是 " + recs.back().date)
              << "。研究報告第 6.2 節：第 5 到 6 週開始，之後每週一次\n";
}

static int cmd_today() {
    auto m = load_mock();
    if (m) std::cout << "模擬面試進行中：" << mock_brief(*m) << "\n";
    for (auto& p : g_catalog) {
        State s = load_state(p);
        if (!s.active || (m && s.mock == m->id)) continue;
        std::cout << "進行中：" << p.id << "（已 " << minutes_since(s) << " 分鐘）→ 先完成它，再 pp done\n";
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
        if (!m) suggest_mock();
        return 0;
    }
    bool found = false;
    for (auto& p : g_catalog) {
        if (latest.count(p.id) || load_state(p).active) continue;
        std::cout << "  新題  pp start " << p.id << "    第 " << p.week << " 週，" << p.pattern << "，"
                  << level_name(p.level) << "\n";
        found = true;
        break;
    }
    if (!found)
        std::cout << "  題單裡的新題都做過了：用 pp mix 做交錯練習、pp mock 做模擬面試，或用 pp new 加題"
                     "（例如 Grind 169 的下一題）。\n";
    if (!m) suggest_mock();
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
    struct Agg { int n = 0, pass = 0, hint = 0, sol = 0, fail = 0, minutes = 0; };
    std::map<string, Agg> by;
    std::map<string, string> pattern_of;
    for (auto& p : g_catalog) pattern_of[p.id] = p.pattern;
    auto rows = load_log();
    if (rows.empty()) { std::cout << "還沒有紀錄。\n"; return 0; }
    bool any_fail = false;
    for (auto& r : rows) {
        Agg& a = by[pattern_of.count(r.id) ? pattern_of[r.id] : "(未登錄)"];
        a.n++;
        a.minutes += r.minutes;
        if (r.result == "pass") a.pass++;
        else if (r.result == "hint") a.hint++;
        else if (r.result == "fail") { a.fail++; any_fail = true; }
        else a.sol++;
    }
    vector<std::pair<string, Agg>> v(by.begin(), by.end());
    auto weakness = [](const Agg& a) { return (a.hint + a.fail + 2.0 * a.sol) / a.n; };
    std::sort(v.begin(), v.end(), [&](auto& x, auto& y) { return weakness(x.second) > weakness(y.second); });
    std::cout << (any_fail ? "依弱項排序（hint 和模擬面試未過算 1、看解答算 2，取平均）：\n"
                           : "依弱項排序（hint 算 1、看解答算 2，取平均）：\n");
    for (auto& [pat, a] : v) {
        char line[256];
        std::snprintf(line, sizeof line, "  %5.2f  次數 %-3d pass %-3d hint %-3d solution %-3d ", weakness(a), a.n,
                      a.pass, a.hint, a.sol);
        std::cout << line;
        if (any_fail) {
            std::snprintf(line, sizeof line, "模擬未過 %-3d ", a.fail);
            std::cout << line;
        }
        std::snprintf(line, sizeof line, "平均 %d 分  ", a.minutes / a.n);
        std::cout << line << pat << "\n";
    }
    auto latest = latest_by_id(rows);
    int stable = 0;
    for (auto& [id, r] : latest) stable += retired(r);
    std::cout << "做過 " << latest.size() << "/" << g_catalog.size() << " 題，已穩定 " << stable
              << " 題。弱項前兩名：用 pp new 多加幾題同模式的題目。\n";
    auto recs = load_mock_records();
    if (!recs.empty()) std::cout << ready_brief(assess(recs)) << "\n";
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
                     "例：pp new lc0438-find-all-anagrams-in-a-string fn 滑動視窗 https://leetcode.com/problems/find-all-anagrams-in-a-string/ M 1\n";
        return 1;
    }
    string id = a[2], kind = a[3], pattern = a[4], url = a[5];
    // Without a week, a new problem goes after the 10-week list (weeks 11-12 are for reviews and mocks).
    string level = a.size() > 6 ? a[6] : "M", week = a.size() > 7 ? a[7] : "11";
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

// ---------------------------------------------------------------- mock interview commands

// The part of a pattern name before its parenthesis: 雙指針（排序後去重） and 雙指針 are one family.
static string family(const string& pattern) {
    size_t k = pattern.find("（");
    return trim(k == string::npos ? pattern : pattern.substr(0, k));
}

struct MockPick {
    const Problem* p;
    bool fresh;
};

// Section 4.2's readiness standard is about problems you have not seen, so the pool prefers, in
// order: problems never attempted, in a pattern family you have already practiced (or added with
// pp new for weeks 11-12); then problems not attempted for a week or more, older ones more likely;
// then anything seen this week. The patterns differ, the first problem is a Medium, and the others
// are drawn across the levels that fit the time per problem (Hard only with 30 minutes or more each).
static vector<MockPick> pick_mock(int n, int minutes) {
    auto rows = load_log();
    if (rows.empty()) return {};
    std::map<string, Days> last;
    for (auto& r : rows) {
        Days d = parse_date(r.date);
        auto found = last.find(r.id);
        if (found == last.end() || found->second < d) last[r.id] = d;
    }
    std::set<string> practiced;
    for (auto& p : g_catalog)
        if (last.count(p.id)) practiced.insert(family(p.pattern));

    std::mt19937 g(std::random_device{}());
    std::uniform_real_distribution<double> jitter(0.5, 1.5);
    struct Cand { const Problem* p; bool fresh; int tier; double key; };
    vector<Cand> pool;
    Days t = today();
    for (auto& p : g_catalog) {
        if (load_state(p).active || !fs::exists(stub_file(p))) continue;
        auto seen_at = last.find(p.id);
        bool seen = seen_at != last.end() || fs::exists(history_dir(p)) || fs::exists(solution_file(p));
        if (!seen) {
            if (p.week < 11 && !practiced.count(family(p.pattern))) continue;  // a pattern not studied yet
            pool.push_back({&p, true, 0, jitter(g)});
        } else {
            long long days = seen_at != last.end() ? (t - seen_at->second).count() : 0;
            pool.push_back({&p, false, days >= 7 ? 1 : 2, static_cast<double>(days + 1) * jitter(g)});
        }
    }
    std::sort(pool.begin(), pool.end(),
              [](const Cand& a, const Cand& b) { return a.tier != b.tier ? a.tier < b.tier : a.key > b.key; });

    int per = minutes / n;
    vector<string> levels{"E"};
    if (per >= 15) levels.push_back("M");
    if (per >= 30) levels.push_back("H");
    std::uniform_int_distribution<size_t> any_level(0, levels.size() - 1);
    vector<string> want;
    for (int i = 0; i < n; ++i) want.push_back(i == 0 ? levels.size() > 1 ? "M" : "E" : levels[any_level(g)]);

    // Relax one preference at a time: the wanted level, then distinct patterns, then a problem seen
    // this week, and last of all the levels that fit the time.
    struct Rule { int max_tier; bool level, distinct, fits; };
    const Rule rules[] = {{1, true, true, true},  {1, false, true, true},  {1, false, false, true},
                          {2, true, true, true},  {2, false, true, true},  {2, false, false, true},
                          {2, false, false, false}};
    vector<MockPick> out;
    std::set<string> families, ids;
    for (auto& lvl : want) {
        const Cand* pick = nullptr;
        for (auto& rule : rules) {
            for (auto& c : pool) {
                if (ids.count(c.p->id) || c.tier > rule.max_tier) continue;
                if (rule.level && c.p->level != lvl) continue;
                if (rule.distinct && families.count(family(c.p->pattern))) continue;
                if (rule.fits && std::find(levels.begin(), levels.end(), c.p->level) == levels.end()) continue;
                pick = &c;
                break;
            }
            if (pick) break;
        }
        if (!pick) break;
        ids.insert(pick->p->id);
        families.insert(family(pick->p->pattern));
        out.push_back({pick->p, pick->fresh});
    }
    // Easier first, as interviews usually go.
    auto rank = [](const string& l) { return l == "E" ? 0 : l == "M" ? 1 : 2; };
    std::stable_sort(out.begin(), out.end(),
                     [&](const MockPick& a, const MockPick& b) { return rank(a.p->level) < rank(b.p->level); });
    return out;
}

// Restores the blank stub and starts problem k's own clock. Pattern and level stay hidden.
static void mock_reveal(MockSession& m, size_t k) {
    MockItem& it = m.items[k];
    const Problem* p = by_id(it.id);
    it.shown = now_s();
    save_mock(m);
    if (!p) {
        std::cout << "題目 " << it.id << " 已經不在 problems/catalog.tsv 裡：pp mock next 跳過，或 pp mock abort 中止。\n";
        return;
    }
    restore_blank(*p);
    State s;
    s.active = true;
    s.start = it.shown;
    s.blind = true;
    s.mock = m.id;
    save_state(*p, s);
    std::cout << "══ 第 " << k + 1 << "/" << m.items.size() << " 題（模式和難度都隱藏）\n題目：" << p->url
              << "\n打開：" << ustr(fs::relative(solution_file(*p), g_root)) << "\n";
    open_in_editor(solution_file(*p));
}

static void print_mock_status(const MockSession& m) {
    long long now = now_s();
    std::cout << "模擬面試 " << m.id << "：" << m.items.size() << " 題 " << m.limit << " 分鐘，";
    if (m.ended) std::cout << "已停止計時（用了 " << mock_min(m, m.start, mock_stop(m)) << " 分鐘），等你寫完檢討\n";
    else if (mock_time_up(m)) std::cout << "時間到\n";
    else std::cout << "已過 " << mock_min(m, m.start, now) << " 分鐘，剩 " << mock_left_min(m) << " 分鐘\n";
    size_t cur = static_cast<size_t>(mock_current(m));
    for (size_t i = 0; i < m.items.size(); ++i) {
        const MockItem& it = m.items[i];
        std::cout << "  第 " << i + 1 << " 題 ";
        if (!it.shown) { std::cout << "還沒輪到\n"; continue; }
        std::cout << it.id << "：";
        bool running = i == cur && !it.left && !m.ended && !mock_time_up(m);
        if (it.status == "pass") std::cout << "通過（" << item_minutes(m, it) << " 分鐘）";
        else if (running)
            std::cout << "進行中，已 " << mock_min(m, it.shown, now) << " 分鐘，"
                      << (it.runs ? "最後一次測試未通過" : "還沒跑測試");
        else std::cout << result_text(it.late ? "late" : "fail");
        std::cout << "\n";
    }
    string a = mock_advice(m);
    if (!a.empty()) std::cout << a << "\n";
}

// A countdown for a terminal panel next to the editor. It only reads the session file, so closing
// it does not stop the session; it exits when time is up or the session ends.
static int cmd_mock_clock() {
    auto first = load_mock();
    if (!first) { std::cout << "沒有進行中的模擬面試。開始一場：pp mock\n"; return 1; }
    const string id = first->id;
    std::cout << "\n倒數計時：這個面板可以一直開著，關掉也不會停止計時。\n" << std::flush;
    int warned_item = -1, gone = 0;
    bool warned_five = false;
    for (;;) {
        auto m = load_mock();
        if (!m || m->id != id) {
            // Another pp command may be rewriting the file right now; only trust a repeated miss.
            if (++gone >= 3) { std::cout << "\n模擬面試已經記錄或中止。\n"; return 0; }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        gone = 0;
        if (m->ended) { std::cout << "\n" << mock_advice(*m) << "\n"; return 0; }
        long long now = now_s(), deadline = mock_deadline(*m);
        if (now >= deadline) { std::cout << "\a\n" << mock_advice(*m) << "\n"; return 0; }
        size_t k = static_cast<size_t>(mock_current(*m));
        const MockItem& it = m->items[k];
        int rem = static_cast<int>((deadline - now) * 60 / m->minute);  // in session seconds
        int used = mock_min(*m, it.shown, now);
        char line[200];
        std::snprintf(line, sizeof line, "\r  剩 %02d:%02d｜第 %d/%d 題，這題 %d 分鐘%s    ", rem / 60, rem % 60,
                      static_cast<int>(k) + 1, static_cast<int>(m->items.size()), used,
                      it.status == "pass" ? "，已通過" : "");
        std::cout << line << std::flush;
        if (!warned_five && m->limit > 5 && deadline - now <= 5LL * m->minute) {
            warned_five = true;
            warned_item = static_cast<int>(k);
            std::cout << "\a\n" << mock_advice(*m) << "\n";
        } else if (warned_item != static_cast<int>(k) && it.status != "pass" && k + 1 < m->items.size() &&
                   used >= mock_budget(*m)) {
            warned_item = static_cast<int>(k);
            std::cout << "\a\n" << mock_advice(*m) << "\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

static int cmd_mock_start(int n, int minutes, bool clock) {
    if (auto m = load_mock()) {
        std::cout << "已經有一場模擬面試在進行，不會再開一場。\n";
        print_mock_status(*m);
        return clock ? cmd_mock_clock() : 0;
    }
    if (n < 1 || n > 4 || minutes < 5 || minutes > 180) {
        std::cout << "題數 1 到 4，時間 5 到 180 分鐘。例：pp mock 2 45\n";
        return 1;
    }
    auto picks = pick_mock(n, minutes);
    if (picks.empty()) {
        std::cout << "還沒有做過的題目，抽不出模擬面試。先照 pp today 學幾個模式："
                     "研究報告第 6.2 節建議第 5 到 6 週開始第一次模擬面試。\n";
        return 1;
    }
    MockSession m;
    m.id = stamp();
    m.start = now_s();
    m.limit = minutes;
    if (const char* e = std::getenv("PP_MOCK_MINUTE"); e && *e) m.minute = std::clamp(std::atoi(e), 1, 60);
    for (auto& pk : picks) {
        MockItem it;
        it.id = pk.p->id;
        it.fresh = pk.fresh;
        m.items.push_back(it);
    }
    std::cout << "══ 模擬面試 " << m.id << "：" << m.items.size() << " 題，共 " << minutes
              << " 分鐘，一次給一題（像面試官出下一題）\n";
    if (static_cast<int>(picks.size()) < n) std::cout << "（只抽得到 " << picks.size() << " 題）\n";
    if (m.minute != 60)
        std::cout << "（PP_MOCK_MINUTE=" << m.minute << "：這場的一分鐘只有 " << m.minute << " 秒，只用來測試 pp 本身）\n";
    auto recs = load_mock_records();
    if (!recs.empty() && !recs.back().improve.empty())
        std::cout << "上一場（" << recs.back().date << "）寫下要改的：" << recs.back().improve << "\n";
    std::cout << "規則：\n"
                 "  1. 沒有提示（pp hint 停用）。不查資料、不看舊解答，也不要讀 test.cpp 裡的 oracle\n"
                 "  2. 出聲講：先複述題目、問澄清問題，講暴力解和複雜度，再寫程式。最好錄影或錄音，事後檢討\n"
                 "  3. 測試照常按 Ctrl+Shift+B（pp test）。時間內最後一次測試的結果，就是這題的結果\n"
                 "  4. 平均每題 "
              << mock_budget(m)
              << " 分鐘。卡住時先寫出正確的暴力解；要換題用 pp mock next，做完用 pp mock end\n";
    save_mock(m);
    mock_reveal(m, 0);
    return clock ? cmd_mock_clock() : 0;
}

static int cmd_mock_next() {
    auto m = load_mock();
    if (!m) { std::cout << "沒有進行中的模擬面試。開始一場：pp mock\n"; return 1; }
    if (m->ended || mock_time_up(*m)) {
        std::cout << (m->ended ? "計時已經停止" : "時間到了") << "，不再出新題：pp mock end 寫檢討。\n";
        return 1;
    }
    size_t k = static_cast<size_t>(mock_current(*m));
    if (k + 1 >= m->items.size()) {
        std::cout << "這已經是最後一題。做完或放棄，都用 pp mock end 停止計時、寫檢討。\n";
        return 1;
    }
    MockItem& it = m->items[k];
    it.left = now_s();
    if (it.status == "pass") std::cout << "第 " << k + 1 << " 題通過，用時 " << item_minutes(*m, it) << " 分鐘。";
    else std::cout << "第 " << k + 1 << " 題記為未通過，之後再測試也不會改變紀錄。";
    std::cout << "這場剩 " << mock_left_min(*m) << " 分鐘。\n";
    mock_reveal(*m, k + 1);
    return 0;
}

static void write_review_template(const MockSession& m) {
    std::ostringstream o;
    o << "# 模擬面試 " << m.id << " 的檢討表。填完之後再執行一次 pp mock end（VS Code 工作：pp: 結束模擬面試）。\n"
      << "# 用了 " << mock_min(m, m.start, mock_stop(m)) << "/" << m.limit << " 分鐘。\n";
    for (size_t i = 0; i < m.items.size(); ++i) {
        const MockItem& it = m.items[i];
        o << "# 第 " << i + 1 << " 題" << (it.shown ? " " + it.id : "") << "：" << result_text(item_result(it));
        if (it.shown) o << "，" << item_minutes(m, it) << " 分鐘";
        o << "\n";
    }
    o << "#\n"
         "# 評分表：在每個標籤後面先寫 1 到 4 分，再寫一句證據，例如「[測試] 2 只跑了範例，沒有自己出空陣列的測資」。\n"
         "# 4 = 做得好，面試官會明確加分；3 = 合格；2 = 有缺失，面試官會注意到；1 = 沒做到或明顯不足。\n"
         "# 有錄影或錄音，先重看一次再打分數（研究報告第 9 節：最好錄影，事後檢討）。\n";
    for (auto& r : kRubric) o << "\n# " << r.question << "\n" << r.tag << " \n";
    o << "\n# 必填：下一場最該改的一件事，一句話。下一場開始時 pp mock 會再顯示給你看。\n"
      << kImprove << " \n"
      << "\n# 選填：模擬面試夥伴是誰、在哪個平台，或錄影檔放在哪裡。\n"
      << kPartner << " \n";
    write_file(review_file(m.id), o.str());
}

// 1-4, typed as an ASCII or full-width digit (a Chinese input method gives full-width).
static int parse_score(const string& v) {
    int s = 0;
    size_t len = 0;
    if (!v.empty() && v[0] >= '1' && v[0] <= '4') {
        s = v[0] - '0';
        len = 1;
    } else if (v.size() >= 3 && static_cast<unsigned char>(v[0]) == 0xEF && static_cast<unsigned char>(v[1]) == 0xBC &&
               static_cast<unsigned char>(v[2]) >= 0x91 && static_cast<unsigned char>(v[2]) <= 0x94) {
        s = static_cast<unsigned char>(v[2]) - 0x90;
        len = 3;
    }
    if (len && v.size() > len && v[len] >= '0' && v[len] <= '9') return 0;  // 10, 34, ...
    return s;
}

static int record_mock(const MockSession& m, const vector<int>& scores, const string& improve, const string& note) {
    string date = fmt_date(today()), rubric;
    for (size_t i = 0; i < scores.size(); ++i) rubric += (i ? "," : "") + std::to_string(scores[i]);
    int used = mock_min(m, m.start, mock_stop(m));
    if (!fs::exists(mock_log_file()))
        write_file(mock_log_file(),
                   "# date\tsession\tid\tlevel\tfresh\tresult\tminutes\tlimit\tused\trubric\timprove\tnote\n"
                   "# 每場模擬面試每題一行。fresh：new 是這場之前沒做過的題。result：pass、fail、late（時間到之後才通過）、none（沒輪到）。\n"
                   "# minutes：這題從出題到通過（或換題、時間到）的分鐘數；used：整場用掉的分鐘數；rubric：澄清,思路,正確,測試,溝通，各 1 到 4 分。\n");
    bool all = true;
    {
        std::ofstream out(mock_log_file(), std::ios::app);
        for (auto& it : m.items) {
            const Problem* p = by_id(it.id);
            string res = item_result(it);
            all = all && res == "pass";
            out << date << '\t' << m.id << '\t' << it.id << '\t' << (p ? p->level : "M") << '\t'
                << (it.fresh ? "new" : "seen") << '\t' << res << '\t' << item_minutes(m, it) << '\t' << m.limit << '\t'
                << used << '\t' << rubric << '\t' << improve << '\t' << note << '\n';
        }
    }
    int lowest = *std::min_element(scores.begin(), scores.end());
    bool passed = all && lowest >= kPassMinScore;

    std::cout << "══ 模擬面試 " << m.id << " 已記錄：" << (passed ? "通過 ✓" : "未通過 ✗") << "\n";
    for (size_t i = 0; i < m.items.size(); ++i) {
        const MockItem& it = m.items[i];
        const Problem* p = by_id(it.id);
        std::cout << "  第 " << i + 1 << " 題 " << it.id;
        if (p) std::cout << "（" << level_name(p->level) << "，" << p->pattern << "，" << (it.fresh ? "這場之前沒做過" : "做過的題") << "）";
        std::cout << "：" << result_text(item_result(it));
        if (it.shown) std::cout << "，" << item_minutes(m, it) << " 分鐘";
        std::cout << "\n";
    }
    std::cout << "  時間：用了 " << used << "/" << m.limit << " 分鐘\n  評分表：";
    for (size_t i = 0; i < scores.size(); ++i) std::cout << (i ? "、" : "") << rubric_name(kRubric[i]) << " " << scores[i];
    std::cout << "\n";
    if (!passed)
        std::cout << "  沒通過的原因：" << (all ? "" : "不是每一題都在時間內通過測試")
                  << (!all && lowest < kPassMinScore ? "；" : "") << (lowest < kPassMinScore ? "評分表有 1 分的項目" : "")
                  << "\n";

    // Feed the spaced-review schedule: a pass moves up the ladder like pp done pass; anything else
    // is due today, so the first thing after a failed mock is working that problem through.
    vector<Row> rows = load_log();
    std::cout << "排進複習：\n";
    for (auto& it : m.items) {
        if (!it.shown) continue;
        bool ok = item_result(it) == "pass";
        vector<Row> prev;
        for (auto& r : rows)
            if (r.id == it.id) prev.push_back(r);
        int interval = ok ? next_interval(prev, "pass") : 0;
        Days next = today() + std::chrono::days{interval};
        Row row{date, it.id, ok ? "pass" : "fail", interval, fmt_date(next), item_minutes(m, it)};
        append_review(row, "mock");
        if (!ok)
            std::cout << "  " << it.id << "：今天到期。先不計時重做到通過（可以用 pp hint），再 pp done 記錄。\n";
        else if (retired(row))
            std::cout << "  " << it.id << "：已穩定，不會出現在到期清單\n";
        else
            std::cout << "  " << it.id << "：下次複習 " << fmt_date(next) << "（" << interval << " 天後）\n";
    }
    std::error_code ec;
    for (auto& it : m.items)
        if (const Problem* p = by_id(it.id); p && load_state(*p).mock == m.id) fs::remove(state_file(*p), ec);
    fs::remove(mock_file(), ec);
    for (auto& it : m.items) {
        const Problem* p = by_id(it.id);
        if (!p || item_result(it) != "pass") continue;
        std::cout << "── " << p->id << " 的學習要點 ──\n關鍵洞見：" << p->lesson << "\nC++ 要點：" << p->cpp << "\n";
    }
    std::cout << "檢討表留在 " << ustr(fs::relative(review_file(m.id), g_root)) << "\n"
              << ready_brief(assess(load_mock_records())) << "\n";
    return 0;
}

static int cmd_mock_end() {
    auto found = load_mock();
    if (!found) { std::cout << "沒有進行中的模擬面試。看過去的紀錄：pp ready\n"; return 1; }
    MockSession& m = *found;
    if (!m.ended) {
        m.ended = now_s();
        save_mock(m);
        std::cout << "計時停止：用了 " << mock_min(m, m.start, mock_stop(m)) << "/" << m.limit << " 分鐘"
                  << (m.ended >= mock_deadline(m) ? "（時間到）" : "") << "。\n";
    }
    fs::path rf = review_file(m.id);
    string shown_rf = ustr(fs::relative(rf, g_root));
    if (!fs::exists(rf)) {
        write_review_template(m);
        for (size_t i = 0; i < m.items.size(); ++i) {
            const MockItem& it = m.items[i];
            std::cout << "  第 " << i + 1 << " 題" << (it.shown ? " " + it.id : "") << "：" << result_text(item_result(it));
            if (it.shown) std::cout << "，" << item_minutes(m, it) << " 分鐘";
            std::cout << "\n";
        }
        std::cout << "還沒記錄。接著寫檢討：" << shown_rf << "\n"
                  << "評分表五項各寫 1 到 4 分和一句證據，再寫 [下次改進]。有錄影就先重看一次。"
                     "填完再執行一次 pp mock end。\n";
        open_in_editor(rf);
        return 1;
    }
    vector<string> tags{kImprove, kPartner};
    for (auto& r : kRubric) tags.push_back(r.tag);
    auto review = read_fields(rf, tags);
    vector<int> scores;
    vector<string> missing;
    for (auto& r : kRubric) {
        int s = parse_score(review[r.tag]);
        scores.push_back(s);
        if (!s) missing.push_back(r.tag);
    }
    if (review[kImprove].empty()) missing.push_back(kImprove);
    if (!missing.empty()) {
        std::cout << "檢討表還沒填完：";
        for (auto& x : missing) std::cout << x << " ";
        std::cout << "\n" << shown_rf << "：評分表每一項在標籤後面先寫 1 到 4 分，再寫一句證據；"
                  << kImprove << " 寫下一場最該改的一件事。填完再執行一次 pp mock end。\n";
        return 1;
    }
    return record_mock(m, scores, one_line(review[kImprove]), one_line(review[kPartner]));
}

static int cmd_mock_abort() {
    auto m = load_mock();
    if (!m) { std::cout << "沒有進行中的模擬面試。\n"; return 1; }
    std::error_code ec;
    for (auto& it : m->items)
        if (const Problem* p = by_id(it.id); p && load_state(*p).mock == m->id) fs::remove(state_file(*p), ec);
    fs::remove(mock_file(), ec);
    std::cout << "已中止模擬面試 " << m->id << "：不記錄，也不影響複習排程。被打斷的場次不算數，"
                 "找一段不會被打擾的時間重來：pp mock\n";
    return 0;
}

static void print_thresholds() {
    std::cout << "門檻的來源（研究報告）：\n"
                 "  至少 " << kReadyMocks << " 場：第 1 節（interviewing.io：做過 5 次以上模擬面試的人，通過 Facebook 面試的機率約 2 倍）、"
                 "第 6.2 節第 9 到 12 週、第 9 節。\n"
                 "  連續 " << kReadyStreak << " 場：第 4.2 節要連續幾場都通過，不是只過一場（只有約 20% 到 25% 的人每場表現穩定）；"
                 "第 9 節：看的是多場的穩定度。報告沒有說幾場，這裡取 3。\n"
                 "  Medium " << kReadyMediumMinutes << " 分鐘、" << kReadyMediumPercent << "%：第 4.2 節，沒見過的 Medium 大多能在 20 到 25 分鐘內解出"
                 "（NeetCode 的基準）。「大多」取七成，時間取上限，看最近 " << kReadyWindow << " 場。\n"
                 "  一場算通過：時間內每題的測試都通過，而且評分表沒有任何一項是 1 分。\n";
}

static int cmd_ready() {
    auto recs = load_mock_records();
    if (recs.empty()) {
        std::cout << "還沒有模擬面試紀錄。研究報告第 6.2 節建議第 5 到 6 週開始第一次，第 9 節建議至少做 5 次：pp mock\n";
        print_thresholds();
        return 0;
    }
    Readiness r = assess(recs);
    auto mark = [](bool ok) { return ok ? "✓" : "✗"; };
    std::cout << "準備度（研究報告第 4.2 節「準備好了的標準」、第 9 節）\n"
              << "  " << mark(r.count_ok) << " 模擬面試至少 " << kReadyMocks << " 場：已做 " << r.mocks << " 場\n"
              << "  " << mark(r.streak_ok) << " 最近連續通過至少 " << kReadyStreak << " 場：目前連續 " << r.streak
              << " 場；最近 " << r.recent << " 場 " << r.marks << "（由舊到新），通過 " << r.recent_passed << " 場\n"
              << "  " << mark(r.medium_ok) << " 最近 " << kReadyWindow << " 場的 Medium 至少 " << kReadyMediumPercent
              << "% 在 " << kReadyMediumMinutes << " 分鐘內通過：" << r.medium_fast << "/" << r.medium;
    if (r.medium) std::cout << "（" << r.medium_fast * 100 / r.medium << "%）";
    bool mostly_seen = r.fresh_medium * 2 < r.medium;
    std::cout << "\n      其中這場之前沒見過的題：" << r.fresh_fast << "/" << r.fresh_medium;
    if (mostly_seen)
        std::cout << "。做過的題會比較快，這一項要靠沒見過的題才準：用 pp new 加題（例如目標公司的標籤題）";
    std::cout << "\n";
    if (r.count_ok && r.streak_ok && r.medium_ok) {
        std::cout << "結論：三項都達標，照報告的標準算是準備好了。繼續每週一場，維持穩定度。\n";
        if (mostly_seen) std::cout << "      但 Medium 大多是做過的題：加幾題沒見過的題，再確認一次速度。\n";
    } else {
        std::cout << "結論：還沒準備好，還差：";
        vector<string> gaps;
        if (!r.count_ok) gaps.push_back("再做 " + std::to_string(kReadyMocks - r.mocks) + " 場");
        if (!r.streak_ok) gaps.push_back("再連續通過 " + std::to_string(kReadyStreak - r.streak) + " 場");
        if (!r.medium_ok) gaps.push_back("Medium 的速度");
        for (size_t i = 0; i < gaps.size(); ++i) std::cout << (i ? "、" : "") << gaps[i];
        std::cout << "\n";
    }

    std::cout << "最近的模擬面試（由新到舊）：\n";
    size_t from = recs.size() > static_cast<size_t>(kReadyWindow) ? recs.size() - kReadyWindow : 0;
    std::map<size_t, std::pair<int, int>> sums;  // rubric item -> (total, count)
    int with_partner = 0;
    for (auto& rec : recs) with_partner += !rec.note.empty();
    for (size_t i = recs.size(); i-- > from;) {
        const MockRecord& rec = recs[i];
        int solved = 0;
        for (auto& row : rec.rows) solved += row.result == "pass";
        std::cout << "  " << rec.date << "  " << mark(rec.passed) << "  " << solved << "/" << rec.rows.size()
                  << " 題  用了 " << rec.used << "/" << rec.limit << " 分鐘  評分";
        for (size_t j = 0; j < rec.rubric.size(); ++j) {
            std::cout << " " << rec.rubric[j];
            sums[j].first += rec.rubric[j];
            sums[j].second += 1;
        }
        if (!rec.note.empty()) std::cout << "  有夥伴或錄影";
        std::cout << "\n";
    }
    std::cout << "評分表平均（最近 " << r.recent << " 場）：";
    vector<double> avgs;
    for (size_t j = 0; j < std::size(kRubric) && sums.count(j); ++j) {
        avgs.push_back(static_cast<double>(sums[j].first) / sums[j].second);
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.1f", avgs.back());
        std::cout << (j ? "、" : "") << rubric_name(kRubric[j]) << " " << buf;
    }
    double low = avgs.empty() ? 0 : *std::min_element(avgs.begin(), avgs.end());
    if (low >= 3) {
        std::cout << "；每一項都在 3 分（合格）以上\n";
    } else {
        std::cout << "；最弱、而且低於 3 分（合格）的是";
        for (size_t j = 0; j < avgs.size(); ++j)
            if (avgs[j] == low) std::cout << "「" << rubric_name(kRubric[j]) << "」";
        std::cout << "，下一場特別注意\n";
    }
    std::cout << "有夥伴或錄影的場次：" << with_partner << "/" << recs.size()
              << "。研究報告第 9 節建議錄影、事後檢討；自己練的時候也要出聲講。\n";
    print_thresholds();
    return 0;
}

static int cmd_mock(const vector<string>& a) {
    bool clock = std::find(a.begin(), a.end(), "--clock") != a.end();
    vector<string> pos;
    for (size_t i = 2; i < a.size(); ++i)
        if (a[i].rfind("--", 0) != 0) pos.push_back(a[i]);
    auto number = [](const string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
    };
    string sub = "start";
    if (!pos.empty() && !number(pos[0])) {
        sub = pos[0];
        pos.erase(pos.begin());
    }
    if (sub == "start") {
        int n = pos.size() > 0 ? std::atoi(pos[0].c_str()) : 2, minutes = pos.size() > 1 ? std::atoi(pos[1].c_str()) : 45;
        return cmd_mock_start(n, minutes, clock);
    }
    if (sub == "status") {
        auto m = load_mock();
        if (!m) { std::cout << "沒有進行中的模擬面試。開始一場：pp mock\n"; return 1; }
        print_mock_status(*m);
        return clock ? cmd_mock_clock() : 0;
    }
    if (sub == "next") return cmd_mock_next();
    if (sub == "end") return cmd_mock_end();
    if (sub == "abort") return cmd_mock_abort();
    if (sub == "clock") return cmd_mock_clock();
    std::cout << "用法：pp mock [題數] [分鐘] [--clock]｜pp mock status｜next｜end｜abort｜clock\n";
    return 1;
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
                 "  pp mock [題數] [分鐘] [--clock]  模擬面試（預設 2 題 45 分鐘）：一次給一題，模式和難度隱藏，沒有提示\n"
                 "  pp mock status|next|end|abort|clock  看狀態、換下一題、停止計時並寫檢討、中止不記錄、倒數計時\n"
                 "  pp ready                    依研究報告的標準看準備度：模擬面試場數、連續通過、Medium 用時\n"
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
            std::cout << "（tools/pp.cpp 比 pp.exe 新：執行工作「pp: 重新建置工具」重新建置）\n";
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
    if (cmd == "mock") return cmd_mock(args);
    if (cmd == "ready") return cmd_ready();
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
