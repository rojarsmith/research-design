// drill.cpp: practice CLI for AI-assisted coding rounds. It needs nothing beyond g++.
// Build: g++ -std=c++20 -O2 -static -o build/drill.exe tools/drill.cpp   (VS Code tasks do this for you)
//
//   list                         scenarios and your past sessions
//   start <scenario> [--mode chat|open|none] [--out <dir>] [--no-open]
//                                copy the scenario into its own folder, show phase 1, start the clock
//   test [<dir>] [filter ...]    build and run the tests of the phases revealed so far
//   next [<dir>] [--skip]        close this phase and reveal the next one
//   status [<dir>]               time spent per phase against the budget
//   run [<dir>] [args ...]       build and run the scenario's command-line app
//   debug [<dir>]                build build/tests-debug.exe for F5 (-O0 -g -D_GLIBCXX_DEBUG)
//   done [<dir>]                 write REVIEW.md, check it, record the session, show the debrief
//
// Each step enforces one habit from the research report (section 6.6 and 6.7):
//   phases are revealed one at a time, the clock runs per phase, a phase only closes when its
//   tests pass (or you say --skip), and a session only counts once the review is written.
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
#include <unistd.h>
#endif

namespace fs = std::filesystem;
using std::string;
using std::vector;

// ---------------------------------------------------------------- UTF-8 at the edges

static fs::path upath(const string& utf8) { return fs::path(std::u8string(utf8.begin(), utf8.end())); }

static string ustr(const fs::path& p) {
    auto s = p.u8string();
    return string(s.begin(), s.end());
}

// Forward slashes, for JSON files and messages.
static string slashed(const fs::path& p) {
    string s = ustr(p);
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
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

#ifdef _WIN32
static std::wstring wide(const string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(static_cast<size_t>(len > 0 ? len - 1 : 0), L'\0');
    if (len > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    return w;
}
#endif

// ---------------------------------------------------------------- small utilities

static string trim(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static string read_text(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void write_text(const fs::path& p, const string& s) {
    if (p.has_parent_path()) fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << s;
}

static string q(const fs::path& p) { return "\"" + ustr(p) + "\""; }

// Runs a shell command (UTF-8) and returns the program's exit code.
static int run(const string& cmd) {
    std::cout.flush();
#ifdef _WIN32
    // cmd.exe strips the outer quotes when a command starts with a quote, so wrap it once more.
    return _wsystem(wide("\"" + cmd + "\"").c_str());
#else
    int rc = std::system(cmd.c_str());
    if (rc == -1) return -1;
    if (WIFEXITED(rc)) return WEXITSTATUS(rc);
    return 128 + (WIFSIGNALED(rc) ? WTERMSIG(rc) : 0);
#endif
}

// Runs a command, echoes its output live, and returns the output too.
static int run_capture(const string& cmd, string& output) {
    std::cout.flush();
#ifdef _WIN32
    // Text mode turns the child's CRLF back into '\n', so std::cout does not double the CR.
    FILE* pipe = _wpopen(wide("\"" + cmd + " 2>&1\"").c_str(), L"rt");
#else
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
#endif
    if (!pipe) return -1;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, pipe)) > 0) {
        output.append(buf, n);
        std::cout.write(buf, (std::streamsize)n);
        std::cout.flush();
    }
#ifdef _WIN32
    return _pclose(pipe);
#else
    int rc = pclose(pipe);
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : 128;
#endif
}

#ifdef _WIN32
static const char* kNull = "nul";
#else
static const char* kNull = "/dev/null";
#endif

static long long now_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

static string stamp(const char* fmt) {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof buf, fmt, &tm);
    return buf;
}

static string minutes(long long seconds) {
    if (seconds < 0) seconds = 0;
    char buf[32];
    std::snprintf(buf, sizeof buf, "%lld 分 %02lld 秒", seconds / 60, seconds % 60);
    return buf;
}

[[noreturn]] static void die(const string& msg) {
    std::cout << msg << "\n";
    std::exit(2);
}

// ---------------------------------------------------------------- kit and catalog

static fs::path g_kit;   // .../ai-round-practice
static fs::path g_self;  // .../ai-round-practice/build/drill.exe

static void locate_kit(const string& argv0) {
#ifdef _WIN32
    (void)argv0;
    wchar_t buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof buf / sizeof buf[0]));
    g_self = fs::path(std::wstring(buf, n));
#else
    std::error_code ec;
    g_self = fs::canonical("/proc/self/exe", ec);
    if (ec) g_self = fs::absolute(argv0);
#endif
    g_kit = g_self.parent_path().parent_path();
    if (!fs::exists(g_kit / "scenarios" / "catalog.tsv"))
        die("找不到 scenarios/catalog.tsv。drill 要放在 ai-round-practice/build/ 底下執行。");
}

struct Scenario {
    string id, title, blurb;
    int budget[4] = {0, 15, 25, 15};  // minutes for phases 1..3
};

static vector<Scenario> catalog() {
    vector<Scenario> out;
    std::istringstream in(read_text(g_kit / "scenarios" / "catalog.tsv"));
    string line;
    bool header = true;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (header) { header = false; continue; }
        vector<string> f;
        std::istringstream ls(line);
        string cell;
        while (std::getline(ls, cell, '\t')) f.push_back(trim(cell));
        if (f.size() < 4) continue;
        Scenario s;
        s.id = f[0];
        s.title = f[1];
        s.blurb = f[2];
        std::istringstream bs(f[3]);
        string b;
        for (int i = 1; i <= 3 && std::getline(bs, b, ','); ++i) s.budget[i] = std::stoi(b);
        out.push_back(s);
    }
    return out;
}

static Scenario find_scenario(const string& id) {
    for (auto& s : catalog())
        if (s.id == id) return s;
    die("沒有這個情境：" + id + "。用 drill list 看清單。");
}

// ---------------------------------------------------------------- session state

struct State {
    fs::path dir;
    std::map<string, string> kv;

    string get(const string& k, const string& def = "") const {
        auto it = kv.find(k);
        return it == kv.end() ? def : it->second;
    }
    long long num(const string& k) const {
        string v = get(k);
        return v.empty() ? 0 : std::stoll(v);
    }
    int phase() const { return (int)num("phase"); }
    void set(const string& k, const string& v) { kv[k] = v; }
    void save() const {
        string out;
        for (auto& [k, v] : kv) out += k + "=" + v + "\n";
        write_text(dir / ".drill" / "state", out);
    }
};

static State load_state(const fs::path& start) {
    std::error_code ec;
    fs::path d = fs::absolute(start, ec);
    for (fs::path p = d; !p.empty(); p = p.parent_path()) {
        if (fs::exists(p / ".drill" / "state")) {
            State s;
            s.dir = p;
            std::istringstream in(read_text(p / ".drill" / "state"));
            string line;
            while (std::getline(in, line)) {
                auto eq = line.find('=');
                if (eq != string::npos) s.kv[line.substr(0, eq)] = trim(line.substr(eq + 1));
            }
            return s;
        }
        if (p == p.parent_path()) break;
    }
    die("這裡不是練習資料夾（找不到 .drill/state）。先用 drill start <情境> 建立一個，或在那個資料夾裡執行。");
}

// Spent seconds in phase p (running phases count up to now).
static long long phase_seconds(const State& s, int p) {
    long long a = s.num("p" + std::to_string(p) + "_start");
    if (!a) return 0;
    long long b = s.num("p" + std::to_string(p) + "_end");
    return (b ? b : now_s()) - a;
}

static void print_clock(const State& s) {
    Scenario sc = find_scenario(s.get("scenario"));
    int cur = s.phase();
    long long total = 0;
    std::cout << "\n  時間：";
    for (int p = 1; p <= 3; ++p) {
        long long sec = phase_seconds(s, p);
        total += sec;
        if (!s.num("p" + std::to_string(p) + "_start")) continue;
        bool over = sec > sc.budget[p] * 60;
        std::cout << (p > 1 ? "；" : "") << "第 " << p << " 階段 " << minutes(sec) << " / 建議 " << sc.budget[p] << " 分"
                  << (over ? "（超時）" : "") << (p == cur && !s.num("p" + std::to_string(p) + "_end") ? "（進行中）" : "");
    }
    std::cout << "\n  整場 " << minutes(total) << " / 60 分\n";
    if (cur >= 1 && phase_seconds(s, cur) > sc.budget[cur] * 60 && !s.num("p" + std::to_string(cur) + "_end"))
        std::cout << "  本階段已超過建議時間。真正的面試官這時會請你往下走：考慮 drill next --skip。\n";
}

// ---------------------------------------------------------------- building

struct BuildKind {
    string name;       // "tests", "app", "debug"
    vector<string> source_dirs;
    string flags;
    fs::path exe;
    fs::path objdir;
};

static BuildKind kind(const fs::path& work, const string& name) {
    BuildKind k;
    k.name = name;
    string common = "-std=c++20 -Wall -Wextra -Isrc -Itests";
    if (name == "debug") {
        k.source_dirs = {"src", "tests"};
        k.flags = common + " -O0 -g -D_GLIBCXX_DEBUG";
        k.exe = work / "build" / "tests-debug.exe";
        k.objdir = work / "build" / "obj-debug";
    } else if (name == "app") {
        k.source_dirs = {"src", "app"};
        k.flags = common + " -O2 -D_GLIBCXX_ASSERTIONS";
        k.exe = work / "build" / "app.exe";
        k.objdir = work / "build" / "obj";
    } else {
        k.source_dirs = {"src", "tests"};
        k.flags = common + " -O2 -D_GLIBCXX_ASSERTIONS";
        k.exe = work / "build" / "tests.exe";
        k.objdir = work / "build" / "obj";
    }
    return k;
}

// Compiles changed sources in parallel, links, and prints compiler output. Returns success.
static bool build(const fs::path& work, const string& name) {
    BuildKind k = kind(work, name);
    fs::create_directories(k.objdir);
    // Not file_time_type{}: with MinGW the file clock's epoch is in the future, so real
    // timestamps are negative and a zero time would look newer than every file.
    fs::file_time_type newest_header = fs::file_time_type::min();
    for (const char* d : {"src", "tests", "app"}) {
        if (!fs::exists(work / d)) continue;
        for (auto& e : fs::directory_iterator(work / d))
            if (e.path().extension() == ".hpp" || e.path().extension() == ".h")
                newest_header = std::max(newest_header, fs::last_write_time(e.path()));
    }
    // Each object records the source and header times it was built from, so a file that
    // changes in either direction (an edit, or a copy with an older timestamp) is rebuilt.
    struct Unit { fs::path src, obj, log, stamp; string key; bool stale; };
    vector<Unit> units;
    for (const string& d : k.source_dirs) {
        if (!fs::exists(work / d)) continue;
        vector<fs::path> files;
        for (auto& e : fs::directory_iterator(work / d))
            if (e.path().extension() == ".cpp") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (auto& f : files) {
            fs::path obj = k.objdir / (d + "_" + ustr(f.stem()) + ".o");
            fs::path stamp = fs::path(ustr(obj) + ".stamp");
            // File times can have one-second resolution, so the size goes into the key too.
            string key = std::to_string(fs::last_write_time(f).time_since_epoch().count()) + " " +
                         std::to_string(fs::file_size(f)) + " " +
                         std::to_string(newest_header.time_since_epoch().count()) + " " + k.flags;
            bool stale = !fs::exists(obj) || read_text(stamp) != key;
            units.push_back({fs::relative(f, work), obj, fs::path(ustr(obj) + ".log"), stamp, key, stale});
        }
    }
    // Objects whose source file was deleted or renamed would still be linked; drop them.
    vector<fs::path> orphans;
    for (auto& e : fs::directory_iterator(k.objdir)) {
        if (e.path().extension() != ".o") continue;
        string stem = ustr(e.path().stem());
        auto us = stem.find('_');
        if (us == string::npos || !fs::exists(work / stem.substr(0, us) / (stem.substr(us + 1) + ".cpp")))
            orphans.push_back(e.path());
    }
    for (auto& o : orphans) fs::remove(o);

    fs::current_path(work);
    vector<Unit*> todo;
    for (auto& u : units)
        if (u.stale) todo.push_back(&u);
    if (!todo.empty()) std::cout << "  編譯 " << todo.size() << " 個檔案……\n";
    vector<int> rc(todo.size(), 0);
    unsigned workers = std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
    for (size_t start = 0; start < todo.size(); start += workers) {
        vector<std::thread> threads;
        for (size_t i = start; i < todo.size() && i < start + workers; ++i)
            threads.emplace_back([&, i] {
                Unit& u = *todo[i];
                rc[i] = run("g++ " + k.flags + " -c " + q(u.src) + " -o " + q(u.obj) + " > " + q(u.log) + " 2>&1");
            });
        for (auto& t : threads) t.join();
    }
    bool ok = true;
    for (size_t i = 0; i < todo.size(); ++i) {
        string log = read_text(todo[i]->log);
        if (!log.empty()) std::cout << log;
        if (rc[i] != 0) {
            ok = false;
            fs::remove(todo[i]->obj);
            fs::remove(todo[i]->stamp);
        } else {
            write_text(todo[i]->stamp, todo[i]->key);
        }
    }
    if (!ok) {
        std::cout << "  編譯失敗。\n";
        return false;
    }
    bool relink = !fs::exists(k.exe) || !todo.empty();
    if (!relink)
        for (auto& u : units) relink |= fs::last_write_time(u.obj) > fs::last_write_time(k.exe);
    if (relink) {
        string objs;
        for (auto& u : units) objs += " " + q(fs::relative(u.obj, work));
#ifdef _WIN32
        string link = "g++ -static" + objs + " -o " + q(fs::relative(k.exe, work));
#else
        string link = "g++" + objs + " -o " + q(fs::relative(k.exe, work));
#endif
        if (run(link) != 0) {
            std::cout << "  連結失敗。\n";
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------- work folder files

static string ai_rules(const string& mode) {
    if (mode == "open")
        return "# Notes for AI assistants\n\n"
               "This folder is a timed practice session for an AI-assisted coding interview.\n"
               "You may read and edit files and run the tests. Keep each change small, say what you changed and why,\n"
               "and leave decisions about the approach to the candidate.\n";
    if (mode == "none")
        return "# Notes for AI assistants\n\n"
               "This folder is a timed practice session without AI help.\n"
               "Do not answer questions about this code. Reply only: \"This session is set to no AI.\"\n";
    return "# Notes for AI assistants\n\n"
           "This folder is a timed practice session for an AI-assisted coding interview, set up like an interview\n"
           "environment where the assistant can only chat.\n\n"
           "- You may read any file in this folder and answer in chat, including with code.\n"
           "- Do not create, edit or delete files, and do not run commands. The candidate types every change.\n";
}

static void write_workspace(const fs::path& work, const string& mode) {
    string drill = slashed(g_self);
    auto make = [&](const string& label, const string& args, bool def, const string& detail) {
        string t = "        {\n            \"label\": \"" + label + "\",\n";
        if (!detail.empty()) t += "            \"detail\": \"" + detail + "\",\n";
        t += "            \"type\": \"process\",\n            \"command\": \"" + drill + "\",\n            \"args\": [" + args + "],\n"
             "            \"options\": { \"cwd\": \"${workspaceFolder}\" },\n";
        if (def) t += "            \"group\": { \"kind\": \"build\", \"isDefault\": true },\n";
        t += "            \"presentation\": { \"clear\": true, \"showReuseMessage\": false },\n"
             "            \"problemMatcher\": [\"$gcc\"]\n        }";
        return t;
    };
    string tasks = "{\n    // drill 練習工作。Ctrl+Shift+B 執行測試。\n    \"version\": \"2.0.0\",\n    \"tasks\": [\n" +
                   make("drill: 測試", "\"test\", \"${workspaceFolder}\"", true, "編譯並執行目前已揭曉階段的測試") + ",\n" +
                   make("drill: 只跑大型測試", "\"test\", \"${workspaceFolder}\", \"large\"", false, "第 3 階段用") + ",\n" +
                   make("drill: 下一階段", "\"next\", \"${workspaceFolder}\"", false, "本階段測試通過後，揭曉下一階段") + ",\n" +
                   make("drill: 跳過並前往下一階段", "\"next\", \"${workspaceFolder}\", \"--skip\"", false, "時間到了就往下走，跟真正的面試一樣") + ",\n" +
                   make("drill: 狀態", "\"status\", \"${workspaceFolder}\"", false, "各階段用時與建議時間") + ",\n" +
                   make("drill: 執行程式", "\"run\", \"${workspaceFolder}\"", false, "編譯並執行 app/main.cpp（不帶參數）") + ",\n" +
                   make("drill: 建置除錯版", "\"debug\", \"${workspaceFolder}\"", false, "F5 會自動執行") + ",\n" +
                   make("drill: 結束並複盤", "\"done\", \"${workspaceFolder}\"", false, "寫 REVIEW.md、記錄這次練習、顯示複盤要點") +
                   "\n    ]\n}\n";
    write_text(work / ".vscode" / "tasks.json", tasks);

    string settings = "{\n";
    if (mode == "open") {
        settings += "    // 模式 open：任何 AI 工具都可以用，包括會直接改檔案的 agent。\n";
    } else {
        settings += "    // 模式 " + mode + "：關掉 AI 行內補全。chat 模式只用聊天面板問 AI，程式碼自己打。\n"
                    "    \"editor.inlineSuggest.enabled\": false,\n"
                    "    \"github.copilot.enable\": { \"*\": false },\n";
    }
    settings += "    \"files.exclude\": { \"build\": true, \".drill\": true },\n"
                "    \"search.exclude\": { \"build\": true, \".drill\": true, \"data/generated\": true },\n"
                "    \"files.encoding\": \"utf8\",\n"
                "    \"C_Cpp.default.cppStandard\": \"c++20\"\n}\n";
    write_text(work / ".vscode" / "settings.json", settings);

    write_text(work / ".vscode" / "launch.json",
               "{\n    \"version\": \"0.2.0\",\n    \"configurations\": [\n        {\n"
               "            \"name\": \"除錯測試\",\n            \"type\": \"cppdbg\",\n            \"request\": \"launch\",\n"
               "            \"program\": \"${workspaceFolder}/build/tests-debug.exe\",\n"
               "            // 只跑某些測試：填測試名稱的一部分，例如 [\"zero\"]\n"
               "            \"args\": [],\n            \"cwd\": \"${workspaceFolder}\",\n            \"MIMode\": \"gdb\",\n"
               "            \"miDebuggerPath\": \"gdb\",\n            \"preLaunchTask\": \"drill: 建置除錯版\",\n"
               "            \"setupCommands\": [{ \"text\": \"-enable-pretty-printing\", \"ignoreFailures\": true }]\n"
               "        }\n    ]\n}\n");
    write_text(work / ".vscode" / "c_cpp_properties.json",
               "{\n    \"configurations\": [\n        {\n            \"name\": \"WinLibs g++\",\n            \"compilerPath\": \"g++\",\n"
               "            \"cppStandard\": \"c++20\",\n            \"intelliSenseMode\": \"windows-gcc-x64\",\n"
               "            \"includePath\": [\"${workspaceFolder}/src\", \"${workspaceFolder}/tests\"]\n        }\n    ],\n    \"version\": 4\n}\n");
    write_text(work / ".vscode" / "extensions.json", "{\n    \"recommendations\": [\"ms-vscode.cpptools\"]\n}\n");

    string rules = ai_rules(mode);
    write_text(work / "AGENTS.md", rules);
    write_text(work / "CLAUDE.md", rules);
    write_text(work / ".github" / "copilot-instructions.md", rules);
    write_text(work / ".gitignore", "build/\ndata/generated/\n.drill/\n");
}

static string mode_line(const string& mode) {
    if (mode == "open") return "open: any AI tool is allowed, including agents that edit files (like Canva's or HackerRank's AI rounds).";
    if (mode == "none") return "none: no AI at all. Use this to measure your unaided baseline.";
    return "chat: ask the AI in the chat panel only; it can read the files but you type every change (like Meta's AI-enabled round).";
}

static string task_header(const Scenario& sc, const string& mode) {
    return "# " + sc.id + ": practice session\n\n"
           "- Mode: " + mode_line(mode) + "\n"
           "- Time: about 60 minutes. Phase 1 about " + std::to_string(sc.budget[1]) + ", phase 2 about " +
           std::to_string(sc.budget[2]) + ", phase 3 about " + std::to_string(sc.budget[3]) + ", then 5 for the wrap-up.\n"
           "- Run the tests with Ctrl+Shift+B (task \"drill: 測試\"). When this phase's tests pass, run task \"drill: 下一階段\".\n"
           "- Talk out loud the whole time, as if an interviewer were listening. Record yourself if you can.\n"
           "- Start by reading `README.md`, then skim `src/` and `tests/` before you change anything.\n\n";
}

// ---------------------------------------------------------------- commands

static int cmd_list() {
    std::map<string, string> last;
    std::istringstream in(read_text(g_kit / "progress" / "sessions.tsv"));
    string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        vector<string> f;
        string cell;
        while (std::getline(ls, cell, '\t')) f.push_back(cell);
        if (f.size() >= 11 && f[0] != "date")
            last[f[1]] = f[0] + "，" + f[2] + " 模式，到第 " + f[3] + " 階段（" + f[8] + "/" + f[9] + "/" + f[10] + "）";
    }
    std::cout << "情境（依建議順序）：\n";
    int i = 0;
    for (auto& s : catalog()) {
        std::cout << "  " << ++i << ". " << s.id << "  " << s.title << "\n     " << s.blurb << "\n     上次："
                  << (last.count(s.id) ? last[s.id] : "還沒練過") << "\n";
    }
    std::cout << "\n開始：drill start <情境> [--mode chat|open|none]\n"
                 "三個情境都做過之後，改用自己挑的開源專案練習，見 README 的「自己帶程式庫」。\n";
    return 0;
}

static int cmd_start(const vector<string>& a) {
    if (a.empty()) die("用法：drill start <情境> [--mode chat|open|none] [--out <資料夾>] [--no-open]");
    Scenario sc = find_scenario(a[0]);
    string mode = "chat";
    fs::path parent = g_kit / "work";
    bool open_editor = true;
    for (size_t i = 1; i < a.size(); ++i) {
        if (a[i] == "--mode" && i + 1 < a.size()) mode = a[++i];
        else if (a[i] == "--out" && i + 1 < a.size()) parent = upath(a[++i]);
        else if (a[i] == "--no-open") open_editor = false;
        else die("看不懂的參數：" + a[i]);
    }
    if (mode != "chat" && mode != "open" && mode != "none") die("模式只能是 chat、open 或 none。");

    fs::path src = g_kit / "scenarios" / sc.id;
    fs::path work = fs::absolute(parent) / (sc.id + "-" + stamp("%Y%m%d-%H%M%S"));
    fs::create_directories(work);
    fs::copy(src / "code", work, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    fs::copy_file(g_kit / "common" / "check.hpp", work / "tests" / "check.hpp", fs::copy_options::overwrite_existing);
    fs::copy_file(g_kit / "common" / "test_main.cpp", work / "tests" / "test_main.cpp", fs::copy_options::overwrite_existing);
    write_workspace(work, mode);
    write_text(work / "TASK.md", task_header(sc, mode) + read_text(src / "phases" / "phase1.md"));

    State s;
    s.dir = work;
    s.set("scenario", sc.id);
    s.set("mode", mode);
    s.set("created", std::to_string(now_s()));
    s.set("phase", "1");
    s.set("p1_start", std::to_string(now_s()));
    s.save();

    // A git repository of its own: the AI tool treats this folder as the project root,
    // and `git diff` shows everything you changed when you review the session.
    if (run("git --version > " + string(kNull) + " 2>&1") == 0) {
        fs::current_path(work);
        bool ok = run("git init -q > " + string(kNull) + " 2>&1") == 0 && run("git add -A > " + string(kNull) + " 2>&1") == 0 &&
                  run("git commit -q -m \"starting point\" > " + string(kNull) + " 2>&1") == 0;
        if (!ok) std::cout << "（git 初始化或第一次提交沒有成功，略過。沒有 git 也能練習。）\n";
    }

    std::cout << "已建立練習資料夾：\n  " << slashed(work) << "\n\n"
              << "模式：" << mode << "\n"
              << "第 1 階段的題目在 TASK.md。時鐘已經開始。\n\n"
              << "請用「新視窗」打開這個資料夾，讓 AI 助手只看得到這次的程式庫：\n"
              << "  code -n \"" << slashed(work) << "\"\n";
    if (open_editor && run("code -n " + q(work) + " > " + string(kNull) + " 2>&1") == 0)
        std::cout << "（已嘗試用 VS Code 打開。）\n";
    return 0;
}

static vector<string> dir_and_rest(const vector<string>& a, fs::path& dir) {
    dir = fs::current_path();
    vector<string> rest = a;
    if (!rest.empty() && rest[0].rfind("--", 0) != 0 && fs::is_directory(upath(rest[0]))) {
        dir = upath(rest[0]);
        rest.erase(rest.begin());
    }
    return rest;
}

static int cmd_test(const vector<string>& a) {
    fs::path dir;
    vector<string> filters = dir_and_rest(a, dir);
    State s = load_state(dir);
    std::cout << "[" << s.get("scenario") << "] 第 " << s.phase() << " 階段\n";
    if (!build(s.dir, "tests")) return 1;
    fs::current_path(s.dir);
    string cmd = q(s.dir / "build" / "tests.exe");
    for (auto& f : filters) cmd += " " + q(upath(f));
    string out;
    int rc = run_capture(cmd, out);
    if (filters.empty()) {
        for (int p = 1; p <= 3; ++p) {
            string key = "  phase " + std::to_string(p) + ": ";
            auto pos = out.find(key);
            if (pos == string::npos) continue;
            auto line_end = out.find('\n', pos);
            string res = out.substr(pos + key.size(), line_end - pos - key.size());
            auto slash = res.find('/');
            auto space = res.find(' ');
            bool pass = slash != string::npos && res.substr(0, slash) == res.substr(slash + 1, space - slash - 1);
            s.set("last_p" + std::to_string(p), pass ? "pass" : "fail");
        }
        s.save();
    }
    print_clock(s);
    if (rc == 0 && filters.empty() && s.phase() < 3 && !s.num("p" + std::to_string(s.phase()) + "_end"))
        std::cout << "  全部通過。準備好就執行工作「drill: 下一階段」。\n";
    else if (rc == 0 && filters.empty() && s.phase() == 3)
        std::cout << "  全部通過。準備好討論題的答案，然後執行工作「drill: 結束並複盤」。\n";
    return rc == 0 ? 0 : 1;
}

static int cmd_next(const vector<string>& a) {
    fs::path dir;
    vector<string> rest = dir_and_rest(a, dir);
    bool skip = std::find(rest.begin(), rest.end(), "--skip") != rest.end();
    State s = load_state(dir);
    int p = s.phase();
    if (p >= 3) {
        std::cout << "已經是最後一個階段。做完後執行工作「drill: 結束並複盤」（drill done）。\n";
        return 0;
    }
    string last = s.get("last_p" + std::to_string(p));
    if (last != "pass" && !skip) {
        std::cout << "第 " << p << " 階段的測試" << (last.empty() ? "還沒跑過" : "還沒全部通過") << "。\n"
                  << "先執行測試；如果時間到了想直接往下，執行工作「drill: 跳過並前往下一階段」（drill next --skip）。\n";
        return 1;
    }
    Scenario sc = find_scenario(s.get("scenario"));
    fs::path phases = g_kit / "scenarios" / sc.id / "phases";
    long long t = now_s();
    s.set("p" + std::to_string(p) + "_end", std::to_string(t));
    if (last != "pass") s.set("skipped_p" + std::to_string(p), "1");
    int n = p + 1;
    s.set("phase", std::to_string(n));
    s.set("p" + std::to_string(n) + "_start", std::to_string(t));
    s.save();
    fs::path tests = phases / ("phase" + std::to_string(n) + "_tests.cpp");
    if (fs::exists(tests))
        fs::copy_file(tests, s.dir / "tests" / tests.filename(), fs::copy_options::overwrite_existing);
    string task = read_text(s.dir / "TASK.md");
    write_text(s.dir / "TASK.md", task + "\n---\n\n" + read_text(phases / ("phase" + std::to_string(n) + ".md")));
    std::cout << "第 " << p << " 階段結束，用時 " << minutes(phase_seconds(s, p)) << (last != "pass" ? "（跳過）" : "") << "。\n"
              << "第 " << n << " 階段的題目已經加到 TASK.md 的最後，新的測試在 tests/" << ustr(tests.filename()) << "。\n";
    return 0;
}

static int cmd_status(const vector<string>& a) {
    fs::path dir;
    dir_and_rest(a, dir);
    State s = load_state(dir);
    std::cout << "[" << s.get("scenario") << "] 模式 " << s.get("mode") << "，目前第 " << s.phase() << " 階段\n";
    for (int p = 1; p <= 3; ++p) {
        string r = s.get("last_p" + std::to_string(p));
        if (!r.empty()) std::cout << "  第 " << p << " 階段最近一次測試：" << (r == "pass" ? "全部通過" : "有失敗") << "\n";
    }
    print_clock(s);
    return 0;
}

static int cmd_run(const vector<string>& a) {
    fs::path dir;
    vector<string> args = dir_and_rest(a, dir);
    State s = load_state(dir);
    if (!build(s.dir, "app")) return 1;
    fs::current_path(s.dir);
    string cmd = q(s.dir / "build" / "app.exe");
    for (auto& x : args) cmd += " " + q(upath(x));
    return run(cmd);
}

static int cmd_debug(const vector<string>& a) {
    fs::path dir;
    dir_and_rest(a, dir);
    State s = load_state(dir);
    return build(s.dir, "debug") ? 0 : 1;
}

static const char* kReviewFields[] = {"[先講方案]：", "[驗證]：", "[AI 出錯]：", "[下次要改]："};

static string field_value(const string& text, const string& marker) {
    auto pos = text.find(marker);
    if (pos == string::npos) return "";
    pos += marker.size();
    auto eol = text.find('\n', pos);
    string v = trim(text.substr(pos, eol == string::npos ? string::npos : eol - pos));
    if (!v.empty() || eol == string::npos) return v;
    auto next_eol = text.find('\n', eol + 1);
    string next = trim(text.substr(eol + 1, next_eol == string::npos ? string::npos : next_eol - eol - 1));
    if (next.empty() || next[0] == '[' || next[0] == '#' || next.rfind("<!--", 0) == 0 || next[0] == '-') return "";
    return next;
}

static string review_template(const State& s) {
    string times;
    for (int p = 1; p <= 3; ++p) {
        if (!s.num("p" + std::to_string(p) + "_start")) { times += "、第 " + std::to_string(p) + " 階段未開始"; continue; }
        times += (p > 1 ? "、" : "") + string("第 ") + std::to_string(p) + " 階段 " + minutes(phase_seconds(s, p)) +
                 (s.get("skipped_p" + std::to_string(p)) == "1" ? "（跳過）" : "");
    }
    return "# 複盤\n\n"
           "- 情境：" + s.get("scenario") + "\n- 模式：" + s.get("mode") + "\n- 日期：" + stamp("%Y-%m-%d") + "\n- 用時：" + times + "\n\n"
           "先回想整場，有錄影就看錄影，再填下面的欄位。前四欄必填，`drill done` 會檢查。\n"
           "冒號後面直接寫，或寫在下一行。\n\n"
           "## 先講方案再下 prompt\n\n<!-- 每次請 AI 寫程式之前，有沒有先說出做法和複雜度？舉一個做到或沒做到的例子。 -->\n[先講方案]：\n\n"
           "## 驗證\n\n<!-- AI 產出的程式，有沒有每一段都讀過、執行過才往下？哪一次沒有？ -->\n[驗證]：\n\n"
           "## AI 出錯\n\n<!-- AI 哪裡給錯了？你多久發現、怎麼發現的？如果它都沒錯，寫你怎麼確認它沒錯。 -->\n[AI 出錯]：\n\n"
           "## 下次要改\n\n<!-- 一件下次要做得不一樣的事。 -->\n[下次要改]：\n\n"
           "## 出現過的反模式（把 [ ] 改成 [x]）\n\n"
           "- [ ] AI 秀場：花時間展示提示技巧，而不是解題\n"
           "- [ ] 功能馬拉松：一直加功能，快速接受 AI 的輸出\n"
           "- [ ] 完全放手：把整個階段丟給 AI\n"
           "- [ ] 沉默地下 prompt：沒有先講再做\n"
           "- [ ] 說不出 AI 寫的某一段在做什麼\n"
           "- [ ] 第 3 階段沒有先量測就改\n\n"
           "## 討論題（選填）\n\n<!-- TASK.md 第 3 階段最後的討論題，你會怎麼回答？ -->\n";
}

static int cmd_done(const vector<string>& a) {
    fs::path dir;
    dir_and_rest(a, dir);
    State s = load_state(dir);
    int p = s.phase();
    fs::path review = s.dir / "REVIEW.md";
    if (!fs::exists(review)) {
        if (!s.num("p" + std::to_string(p) + "_end")) {
            s.set("p" + std::to_string(p) + "_end", std::to_string(now_s()));
            s.save();
        }
        write_text(review, review_template(s));
        std::cout << "時鐘已停止。已建立 REVIEW.md：\n  " << slashed(review) << "\n"
                  << "填好四個必填欄位後，再執行一次工作「drill: 結束並複盤」。\n";
        return 0;
    }
    string text = read_text(review);
    vector<string> missing;
    for (const char* f : kReviewFields)
        if (field_value(text, f).empty()) missing.push_back(f);
    if (!missing.empty()) {
        std::cout << "REVIEW.md 還有欄位沒填：";
        for (auto& m : missing) std::cout << " " << m;
        std::cout << "\n寫下來才會記住。填好後再執行一次。\n";
        return 1;
    }
    if (s.get("recorded") != "1") {
        fs::path log = g_kit / "progress" / "sessions.tsv";
        string out = read_text(log);
        if (out.empty()) out = "date\tscenario\tmode\tphase\tp1_min\tp2_min\tp3_min\ttotal_min\tp1\tp2\tp3\tfolder\n";
        long long total = 0;
        string row = stamp("%Y-%m-%d") + "\t" + s.get("scenario") + "\t" + s.get("mode") + "\t" + std::to_string(p);
        for (int i = 1; i <= 3; ++i) {
            long long sec = phase_seconds(s, i);
            total += sec;
            row += "\t" + std::to_string((sec + 30) / 60);
        }
        row += "\t" + std::to_string((total + 30) / 60);
        for (int i = 1; i <= 3; ++i) {
            string r = s.get("last_p" + std::to_string(i));
            if (s.get("skipped_p" + std::to_string(i)) == "1") r = "skip";
            row += "\t" + (r.empty() ? string("-") : r);
        }
        row += "\t" + slashed(s.dir) + "\n";
        write_text(log, out + row);
        s.set("recorded", "1");
        s.save();
    }
    fs::path debrief = g_kit / "scenarios" / s.get("scenario") / "debrief.zh-TW.md";
    fs::copy_file(debrief, s.dir / "DEBRIEF.zh-TW.md", fs::copy_options::overwrite_existing);
    std::cout << "已記錄到 progress/sessions.tsv。複盤要點（也複製到 DEBRIEF.zh-TW.md）：\n\n" << read_text(debrief) << "\n";
    std::cout << "回放檢查（研究報告第 6.7 節）：prompt 之前有沒有先講方案、每段 AI 產出有沒有執行驗證、AI 寫錯時多久發現。\n";
    return 0;
}

static void usage() {
    std::cout << "drill：AI 輔助程式回合的練習工具\n\n"
                 "  drill list                                  情境清單與過去的練習紀錄\n"
                 "  drill start <情境> [--mode chat|open|none]  建立練習資料夾，開始計時\n"
                 "  drill test [資料夾] [測試名稱的一部分 ...]  編譯並執行測試\n"
                 "  drill next [資料夾] [--skip]                結束本階段，揭曉下一階段\n"
                 "  drill status [資料夾]                       各階段用時\n"
                 "  drill run [資料夾] [參數 ...]               編譯並執行程式\n"
                 "  drill debug [資料夾]                        建置除錯版測試（F5）\n"
                 "  drill done [資料夾]                         寫複盤、記錄、看複盤要點\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    vector<string> args = utf8_args(argc, argv);
    locate_kit(args.empty() ? "" : args[0]);
    if (args.size() < 2) {
        usage();
        return 0;
    }
    string cmd = args[1];
    vector<string> rest(args.begin() + 2, args.end());
    try {
        if (cmd == "list") return cmd_list();
        if (cmd == "start") return cmd_start(rest);
        if (cmd == "test") return cmd_test(rest);
        if (cmd == "next") return cmd_next(rest);
        if (cmd == "status") return cmd_status(rest);
        if (cmd == "run") return cmd_run(rest);
        if (cmd == "debug") return cmd_debug(rest);
        if (cmd == "done") return cmd_done(rest);
    } catch (const std::exception& e) {
        std::cout << "錯誤：" << e.what() << "\n";
        return 2;
    }
    usage();
    return 2;
}
