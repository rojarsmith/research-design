// est.cpp: capacity-estimation drill CLI. It needs nothing beyond g++.
// Build: g++ -std=c++20 -O2 -static -o build/est.exe tools/est.cpp   (VS Code tasks do this for you)
//
//   est list                     the drill bank by quantity type, with your status
//   est today                    due reviews first, then new drills
//   est drill [id|type] [-n N]   timed drills; without an id they are interleaved: due ones first,
//                                then new ones, never the same quantity type twice in a row
//   est review [-n N]            due drills only (--plan on either lists the picks without starting)
//   est chain [id]               one system's numbers in order: users -> QPS -> storage -> bandwidth -> servers
//   est stats                    hit rate and log-ratio bias by quantity type
//   est parse <text> ...         try the unit parser (--raw prints machine-readable lines)
//   est --self-test              unit-parser tests and a check of the drill bank
//
// Options: --minutes M (time per drill, default 5), --seed S (fixed drill order).
// Environment: EST_PROGRESS=<dir> keeps progress elsewhere; EST_TODAY=YYYY-MM-DD fakes the date.
//
// Each step enforces a rule from section 3.3 of the research report: assumptions are written
// before the reference is shown, every answer needs a unit, scoring is by order of magnitude,
// and an attempt is recorded only after you name the step where your estimate diverged.
#include <algorithm>
#include <chrono>
#include <cmath>
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
static bool g_console_in = false;

static string narrow(const std::wstring& w) {
    if (w.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    string s(static_cast<size_t>(len > 0 ? len : 0), '\0');
    if (len > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), len, nullptr, nullptr);
    return s;
}
#endif

// Reads one line of UTF-8. A Windows console is read as UTF-16 so Chinese input survives any
// code page; piped input is taken as UTF-8 bytes. Returns false at end of input.
static bool read_line(string& out) {
    std::cout.flush();
#ifdef _WIN32
    if (g_console_in) {
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
        std::wstring w;
        wchar_t buf[256];
        for (;;) {
            DWORD n = 0;
            if (!ReadConsoleW(h, buf, 256, &n, nullptr) || n == 0) {
                if (w.empty()) return false;
                break;
            }
            w.append(buf, n);
            if (w.back() == L'\n') break;
        }
        if (!w.empty() && w[0] == 26) return false;  // Ctrl+Z
        while (!w.empty() && (w.back() == L'\n' || w.back() == L'\r')) w.pop_back();
        out = narrow(w);
        return true;
    }
#endif
    if (!std::getline(std::cin, out)) return false;
    if (!out.empty() && out.back() == '\r') out.pop_back();
    if (out.rfind("\xEF\xBB\xBF", 0) == 0) out.erase(0, 3);
    return true;
}

// ---------------------------------------------------------------- small utilities

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
        if (c == sep) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

static string lower_ascii(string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    return s;
}

static bool starts_with(const string& s, const string& p) { return s.rfind(p, 0) == 0; }
static bool ends_with(const string& s, const string& p) {
    return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
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

static void append_text(const fs::path& p, const string& s) {
    if (p.has_parent_path()) fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary | std::ios::app);
    out << s;
}

// One line of free text for a TSV cell.
static string one_line(string s) {
    for (char& c : s)
        if (c == '\t' || c == '\r' || c == '\n') c = ' ';
    return trim(s);
}

static string replace_all(string s, const string& from, const string& to) {
    for (size_t pos = 0; (pos = s.find(from, pos)) != string::npos; pos += to.size()) s.replace(pos, from.size(), to);
    return s;
}

// Terminal columns: CJK and full-width characters (3-byte UTF-8) take two.
static int display_width(const string& s) {
    int w = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : 4;
        w += len >= 3 ? 2 : 1;
        i += len;
    }
    return w;
}

static string pad(const string& s, int width) {
    int w = display_width(s);
    return s + string((size_t)(w < width ? width - w : 0), ' ');
}

[[noreturn]] static void die(const string& msg) {
    std::cout << msg << "\n";
    std::exit(2);
}

// ---------------------------------------------------------------- dates

using Days = std::chrono::sys_days;

static Days parse_date(const string& s) {
    int y = 1970, m = 1, d = 1;
    std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
    return Days{std::chrono::year{y} / m / d};
}

static Days today() {
    if (const char* fake = std::getenv("EST_TODAY"); fake && *fake) return parse_date(fake);
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return Days{std::chrono::year{tm.tm_year + 1900} / (tm.tm_mon + 1) / tm.tm_mday};
}

static string fmt_date(Days d) {
    std::chrono::year_month_day ymd{d};
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02u-%02u", static_cast<int>(ymd.year()), static_cast<unsigned>(ymd.month()),
                  static_cast<unsigned>(ymd.day()));
    return buf;
}

static string clock_text(long long seconds) {
    if (seconds < 0) seconds = 0;
    char buf[32];
    std::snprintf(buf, sizeof buf, "%lld 分 %02lld 秒", seconds / 60, seconds % 60);
    return buf;
}

// ---------------------------------------------------------------- quantities and units
//
// Every quantity is stored in a base unit: a plain count, events per second, bytes, bytes per
// second, or seconds. Decimal prefixes (1 KB = 1,000 B) follow the report's powers-of-ten
// shortcuts; KiB, MiB ... are binary. Bits are converted to bytes (8 bits = 1 byte).

enum class Dim { None, Count, Rate, Bytes, ByteRate, Time, Ratio };

struct Quantity {
    double v = 0;
    Dim d = Dim::None;
    string unit;  // the unit text as written, for messages
};

static const char* dim_name(Dim d) {
    switch (d) {
        case Dim::Count: return "數量";
        case Dim::Rate: return "速率（每秒多少次）";
        case Dim::Bytes: return "資料量";
        case Dim::ByteRate: return "頻寬（每秒多少資料）";
        case Dim::Time: return "時間";
        case Dim::Ratio: return "比例";
        default: return "沒有單位的數字";
    }
}

static const char* dim_example(Dim d) {
    switch (d) {
        case Dim::Count: return "12 台、200 個分片、40 servers、1億 人";
        case Dim::Rate: return "3k QPS、200 /s、5億 次/天";
        case Dim::Bytes: return "1.2 TB、500 GB、64 MB";
        case Dim::ByteRate: return "40 MB/s、10 Gbps";
        case Dim::Time: return "300 ms、2.5 s、22 min、1.8 h";
        case Dim::Ratio: return "3 倍、40%";
        default: return "";
    }
}

static std::optional<Dim> dim_from_key(const string& k) {
    if (k == "count") return Dim::Count;
    if (k == "rate") return Dim::Rate;
    if (k == "bytes") return Dim::Bytes;
    if (k == "byterate") return Dim::ByteRate;
    if (k == "time") return Dim::Time;
    if (k == "ratio") return Dim::Ratio;
    return std::nullopt;
}

static const char* dim_key(Dim d) {
    switch (d) {
        case Dim::Count: return "count";
        case Dim::Rate: return "rate";
        case Dim::Bytes: return "bytes";
        case Dim::ByteRate: return "byterate";
        case Dim::Time: return "time";
        case Dim::Ratio: return "ratio";
        default: return "none";
    }
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Reads a number at the start of s: 3, 3.5, 86,400, 3e5, 2^30, 3×10^5. Returns the bytes consumed.
static size_t read_number(const string& s, double& v) {
    size_t i = 0;
    string m;
    bool any = false, dot = false;
    while (i < s.size()) {
        char c = s[i];
        if (is_digit(c)) {
            m += c;
            any = true;
            ++i;
        } else if (c == '.' && !dot && i + 1 < s.size() && is_digit(s[i + 1])) {
            m += c;
            dot = true;
            ++i;
        } else if (c == ',' && any && !dot && i + 3 < s.size() + 0 && is_digit(s[i + 1]) && is_digit(s[i + 2]) &&
                   is_digit(s[i + 3]) && (i + 4 >= s.size() || !is_digit(s[i + 4]))) {
            ++i;  // thousands separator: 86,400
        } else {
            break;
        }
    }
    if (!any) return 0;
    v = std::stod(m);
    auto read_int = [&](size_t from, int& out) -> size_t {
        size_t j = from;
        bool neg = false;
        if (j < s.size() && (s[j] == '+' || s[j] == '-')) neg = s[j++] == '-';
        size_t k = j;
        while (k < s.size() && is_digit(s[k])) ++k;
        if (k == j || k - j > 3) return 0;
        out = std::stoi(s.substr(j, k - j)) * (neg ? -1 : 1);
        return k;
    };
    int e = 0;
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        if (size_t k = read_int(i + 1, e)) {
            v *= std::pow(10.0, e);
            i = k;
        }
    } else if (i < s.size() && s[i] == '^') {
        if (size_t k = read_int(i + 1, e)) {
            v = std::pow(v, e);
            i = k;
        }
    }
    // 3×10^5, 3x10^5, 3*10^5
    size_t j = i;
    while (j < s.size() && s[j] == ' ') ++j;
    size_t mul = 0;
    if (s.compare(j, 2, "\xC3\x97") == 0) mul = 2;
    else if (j < s.size() && (s[j] == 'x' || s[j] == 'X' || s[j] == '*')) mul = 1;
    if (mul) {
        size_t k = j + mul;
        while (k < s.size() && s[k] == ' ') ++k;
        if (s.compare(k, 3, "10^") == 0) {
            if (size_t end = read_int(k + 3, e)) {
                v *= std::pow(10.0, e);
                i = end;
            }
        }
    }
    return i;
}

static bool time_unit(const string& raw, double& sec) {
    static const std::map<string, double> t = {
        {"ns", 1e-9}, {"奈秒", 1e-9}, {"納秒", 1e-9}, {"us", 1e-6}, {"\xC2\xB5s", 1e-6}, {"\xCE\xBCs", 1e-6},
        {"微秒", 1e-6}, {"ms", 1e-3}, {"毫秒", 1e-3}, {"s", 1}, {"sec", 1}, {"secs", 1}, {"second", 1},
        {"seconds", 1}, {"秒", 1}, {"秒鐘", 1}, {"min", 60}, {"mins", 60}, {"minute", 60}, {"minutes", 60},
        {"分", 60}, {"分鐘", 60}, {"h", 3600}, {"hr", 3600}, {"hrs", 3600}, {"hour", 3600}, {"hours", 3600},
        {"小時", 3600}, {"時", 3600}, {"d", 86400}, {"day", 86400}, {"days", 86400}, {"天", 86400},
        {"日", 86400}, {"wk", 604800}, {"week", 604800}, {"weeks", 604800}, {"週", 604800},
        {"mo", 2592000}, {"month", 2592000}, {"months", 2592000}, {"月", 2592000}, {"個月", 2592000},
        {"y", 31536000}, {"yr", 31536000}, {"yrs", 31536000}, {"year", 31536000}, {"years", 31536000},
        {"年", 31536000}};
    auto it = t.find(lower_ascii(trim(raw)));
    if (it == t.end()) return false;
    sec = it->second;
    return true;
}

// 1 = bytes, 2 = bits (factor already in bytes), 0 = not a data unit, -1 = ambiguous (err set).
static int byte_unit(const string& u, double& bytes, string& err) {
    string lu = lower_ascii(u);
    if (u == "B" || lu == "byte" || lu == "bytes" || u == "位元組" || u == "字節") {
        bytes = 1;
        return 1;
    }
    if (u == "b" || lu == "bit" || lu == "bits" || u == "位元") {
        bytes = 0.125;
        return 2;
    }
    if (u.size() < 2) return 0;
    char p = u[0];
    static const string order = "kmgtpe";
    size_t idx = order.find((char)std::tolower((unsigned char)p));
    if (idx == string::npos) return 0;
    string rest = u.substr(1);
    double f = std::pow(1000.0, (double)(idx + 1));
    if (!rest.empty() && rest[0] == 'i') {
        f = std::pow(1024.0, (double)(idx + 1));
        rest = rest.substr(1);
    }
    string lr = lower_ascii(rest);
    bool is_byte = rest == "B" || lr == "byte" || lr == "bytes";
    bool is_bit = rest == "b" || lr == "bit" || lr == "bits";
    if (!is_byte && !is_bit) return 0;
    if (p == 'm') {
        err = "「" + u + "」：小寫 m 是「毫」。百萬請用大寫 M：MB 是百萬位元組，Mb 是百萬位元。";
        return -1;
    }
    if (rest == "b" && p >= 'a' && p <= 'z') {
        char up = (char)std::toupper((unsigned char)p);
        err = "「" + u + "」分不出是位元組還是位元：位元組寫大寫 B（例如 " + string(1, up) + "B），位元寫 " +
              string(1, up) + "b 或 " + string(1, up) + "bit。";
        return -1;
    }
    bytes = f * (is_bit ? 0.125 : 1.0);
    return is_bit ? 2 : 1;
}

static bool count_noun(const string& u) {
    static const std::set<string> en = {
        "server", "servers", "machine", "machines", "node", "nodes", "host", "hosts", "instance", "instances",
        "box", "boxes", "vm", "vms", "pod", "pods", "container", "containers", "partition", "partitions",
        "shard", "shards", "replica", "replicas", "worker", "workers", "gpu", "gpus", "core", "cores", "cpu",
        "cpus", "disk", "disks", "broker", "brokers", "consumer", "consumers", "connection", "connections",
        "conn", "conns", "user", "users", "people", "person", "dau", "mau", "driver", "drivers", "rider",
        "riders", "device", "devices", "request", "requests", "req", "reqs", "query", "queries", "call", "calls",
        "op", "ops", "operation", "operations", "read", "reads", "write", "writes", "msg", "msgs", "message",
        "messages", "event", "events", "point", "points", "sample", "samples", "update", "updates", "delivery",
        "deliveries", "push", "pushes", "send", "sends", "notification", "notifications", "item", "items", "key",
        "keys", "row", "rows", "record", "records", "entry", "entries", "token", "tokens", "photo", "photos",
        "image", "images", "video", "videos", "post", "posts", "file", "files", "link", "links", "url", "urls",
        "page", "pages", "click", "clicks", "view", "views", "submission", "submissions", "comment", "comments",
        "game", "games", "move", "moves", "doc", "docs", "document", "documents", "order", "orders", "ticket",
        "tickets", "block", "blocks", "chunk", "chunks", "object", "objects", "counter", "counters", "insert",
        "inserts", "timeline", "timelines", "copy", "copies", "unit", "units", "job", "jobs", "task", "tasks"};
    static const vector<string> zh_measure = {"台", "臺", "個", "个", "部", "片", "份", "次", "人", "則", "筆", "張",
                                              "支", "條", "封", "篇", "名", "位", "組", "顆", "塊", "項", "件", "輪"};
    static const vector<string> zh_noun = {"伺服器", "機器", "節點", "分片", "分區", "副本", "連線", "訊息", "請求",
                                           "使用者", "用戶", "工作者", "執行個體", "貼文", "照片", "網頁", "裝置",
                                           "司機", "推播", "寫入", "讀取", "事件", "點"};
    string s = trim(u);
    if (s.empty()) return false;
    bool ascii = std::all_of(s.begin(), s.end(), [](char c) { return (unsigned char)c < 0x80; });
    if (ascii) {
        std::istringstream in(s);
        vector<string> words;
        for (string w; in >> w;) words.push_back(lower_ascii(w));
        if (words.empty() || words.size() > 3) return false;
        // "2.5M users" and "2.5B users": a magnitude letter is not part of the noun.
        static const std::set<string> not_words = {"k", "m", "b", "bn", "g", "t"};
        for (size_t i = 0; i < words.size(); ++i) {
            const string& w = words[i];
            if (!std::all_of(w.begin(), w.end(), [](char c) { return std::isalnum((unsigned char)c) || c == '-'; }))
                return false;
            if (i + 1 < words.size() && (not_words.count(w) || is_digit(w[0]))) return false;
        }
        return en.count(words.back()) > 0;
    }
    for (auto& m : zh_measure)
        if (starts_with(s, m)) return true;
    for (auto& n : zh_noun)
        if (ends_with(s, n)) return true;
    return false;
}

static const char* kMissingUnit = "缺單位。數字後面要寫單位，例如 3k QPS、1.2 TB、40 MB/s、22 min、12 台。";

static bool parse_unit(const string& raw, double v, Quantity& q, string& err) {
    string u = trim(raw);
    q.unit = u;
    if (u.empty()) {
        err = kMissingUnit;
        return false;
    }
    // "3k 次每秒", "3k req per second" -> slash form
    u = replace_all(u, "每", "/");
    u = replace_all(u, " per ", "/");
    string lu = lower_ascii(u);
    if (u == "%") {
        q.v = v / 100;
        q.d = Dim::Ratio;
        return true;
    }
    if (u == "倍" || lu == "x" || u == "\xC3\x97") {
        q.v = v;
        q.d = Dim::Ratio;
        return true;
    }
    if (lu == "qps" || lu == "rps" || lu == "tps") {
        q.v = v;
        q.d = Dim::Rate;
        return true;
    }
    if (lu.size() >= 3 && ends_with(lu, "bps") && lu.size() <= 4) {
        static const std::map<char, double> pre = {{'k', 1e3}, {'m', 1e6}, {'g', 1e9}, {'t', 1e12}, {'p', 1e15}};
        double f = 1;
        if (lu.size() == 4) {
            auto it = pre.find(lu[0]);
            if (it == pre.end()) {
                err = "看不懂「" + u + "」。位元速率請寫 Kbps、Mbps、Gbps 或 Tbps。";
                return false;
            }
            f = it->second;
        }
        q.v = v * f / 8;
        q.d = Dim::ByteRate;
        return true;
    }
    if (auto slash = u.find('/'); slash != string::npos) {
        string left = trim(u.substr(0, slash)), right = trim(u.substr(slash + 1));
        double sec = 0;
        if (!time_unit(right, sec)) {
            err = "「/" + right + "」不是時間單位。斜線後面請寫 s、min、h 或 day，例如 req/s、MB/s、次/天。";
            return false;
        }
        if (left.empty() || count_noun(left)) {
            q.v = v / sec;
            q.d = Dim::Rate;
            return true;
        }
        double b = 0;
        int k = byte_unit(left, b, err);
        if (k > 0) {
            q.v = v * b / sec;
            q.d = Dim::ByteRate;
            return true;
        }
        if (k < 0) return false;
        err = "看不懂「" + left + "」。斜線前面要是資料量（MB/s）或計數（req/s、次/秒），也可以只寫 /s。";
        return false;
    }
    double b = 0;
    int k = byte_unit(u, b, err);
    if (k > 0) {
        q.v = v * b;
        q.d = Dim::Bytes;
        return true;
    }
    if (k < 0) return false;
    double sec = 0;
    if (time_unit(u, sec)) {
        q.v = v * sec;
        q.d = Dim::Time;
        return true;
    }
    if (count_noun(u)) {
        q.v = v;
        q.d = Dim::Count;
        return true;
    }
    if (starts_with(u, "B ") || u == "b")
        err = "「" + u + "」：B 是位元組（byte）。十億請寫 1e9、1bn 或 10億，例如 2.5bn users。";
    else if (u == "m" || u == "M")
        err = "「" + u + "」不是單位：百萬請寫 M 再加單位（例如 2M users），分鐘請寫 min，毫秒請寫 ms。";
    else
        err = "看不懂單位「" + u + "」。可以用：QPS、/s、/day、B、KB、MB、GB、TB、PB、MB/s、Gbps、ms、s、min、h、台、個、servers。";
    return false;
}

static const std::pair<const char*, double> kSuffixes[] = {
    {"bn", 1e9}, {"k", 1e3}, {"K", 1e3}, {"M", 1e6}, {"十", 10}, {"百", 100}, {"千", 1e3},
    {"萬", 1e4}, {"万", 1e4}, {"億", 1e8}, {"亿", 1e8}};

// Parses "300k QPS", "1.2 TB", "40 MB/s", "2000萬 人", "3e5 /s" ... into a quantity in base units.
static bool parse_quantity(const string& text, Quantity& q, string& err) {
    string s = trim(text);
    for (const char* approx : {"~", "≈", "約", "大約"})
        if (starts_with(s, approx)) s = trim(s.substr(string(approx).size()));
    double v = 0;
    size_t used = read_number(s, v);
    if (!used) {
        err = s.empty() ? "沒有輸入。請寫數字加單位，例如 3k QPS。" : "開頭要是數字，例如 3k QPS、1.2 TB、3e5 /s。";
        return false;
    }
    string rest = s.substr(used);
    string first_err;
    if (parse_unit(rest, v, q, first_err)) return true;
    // Magnitude words: 300k, 2.5M, 2000萬, 1.5億, 2千萬 (Chinese ones may stack).
    string r = trim(rest);
    double mult = 1;
    bool took = false;
    for (int round = 0; round < 3; ++round) {
        bool matched = false;
        for (auto& [suf, f] : kSuffixes) {
            string sf = suf;
            if (!starts_with(r, sf)) continue;
            // "M" followed by "B", "b", "bps", "iB" is a unit prefix, not "million".
            if (sf == "M" || sf == "k" || sf == "K") {
                string after = r.substr(sf.size());
                double dummy;
                string e2;
                if (!after.empty() && after[0] != ' ' && (byte_unit(r, dummy, e2) != 0 || ends_with(lower_ascii(r), "bps")))
                    continue;
                if (took) continue;  // ASCII magnitude only once
            }
            r = trim(r.substr(sf.size()));
            mult *= f;
            took = matched = true;
            break;
        }
        if (!matched) break;
        string e3;
        if (parse_unit(r, v * mult, q, e3)) return true;
        first_err = e3;
    }
    err = first_err;
    return false;
}

// Message for an answer whose unit does not fit the question, or "" if it fits.
static string dim_mismatch(const Quantity& q, Dim want) {
    if (q.d == want) return "";
    string msg = "這題問的是" + string(dim_name(want)) + "，例如 " + dim_example(want) + "。你寫的「" + q.unit +
                 "」是" + dim_name(q.d) + "。";
    if (q.unit == "B" || starts_with(q.unit, "B "))
        msg += "B 是位元組（byte）；十億請寫 1e9、1bn 或 10億。";
    else if (want == Dim::Rate && q.d == Dim::Count)
        msg += "速率要寫「每秒」：QPS、/s、/day（例如 3k req/s）。";
    else if (want == Dim::Bytes && q.d == Dim::ByteRate)
        msg += "要的是總量，不是每秒的量：乘上時間再寫 GB、TB。";
    else if (want == Dim::ByteRate && q.d == Dim::Bytes)
        msg += "頻寬要寫「每秒」：MB/s 或 Gbps。";
    return msg;
}

// ---------------------------------------------------------------- formatting

static string num(double v, int sig = 3) {
    if (v == 0) return "0";
    double a = std::fabs(v);
    int digits = (int)std::floor(std::log10(a)) + 1;
    int decimals = std::max(0, sig - digits);
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", decimals, v);
    string s = buf;
    if (s.find('.') != string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

static string scaled(double v) {
    double a = std::fabs(v);
    if (a >= 1e12) {
        int e = (int)std::floor(std::log10(a));
        return num(v / std::pow(10.0, e)) + "e" + std::to_string(e);
    }
    if (a >= 1e9) return num(v / 1e9) + "bn";
    if (a >= 1e6) return num(v / 1e6) + "M";
    if (a >= 1e3) return num(v / 1e3) + "k";
    return num(v);
}

static string with_prefix(double v, const vector<string>& units) {
    size_t i = 0;
    while (i + 1 < units.size() && std::fabs(v) >= 1000) {
        v /= 1000;
        ++i;
    }
    return num(v) + " " + units[i];
}

static string fmt_q(Dim d, double v) {
    static const vector<string> bytes = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    static const vector<string> bits = {"bps", "Kbps", "Mbps", "Gbps", "Tbps", "Pbps"};
    switch (d) {
        case Dim::Count: return scaled(v);
        case Dim::Rate: return scaled(v) + " /s";
        case Dim::Bytes: return with_prefix(v, bytes);
        case Dim::ByteRate: return with_prefix(v, bytes) + "/s（" + with_prefix(v * 8, bits) + "）";
        case Dim::Time: {
            double a = std::fabs(v);
            if (a < 1e-6) return num(v * 1e9) + " ns";
            if (a < 1e-3) return num(v * 1e6) + " µs";
            if (a < 1) return num(v * 1e3) + " ms";
            if (a < 120) return num(v) + " s";
            if (a < 7200) return num(v / 60) + " min";
            if (a < 172800) return num(v / 3600) + " h";
            return num(v / 86400) + " 天";
        }
        case Dim::Ratio: return num(v) + " 倍";
        default: return num(v);
    }
}

// ---------------------------------------------------------------- the drill bank
//
// drills/*.txt hold records like this (the README describes the format):
//   == drill <id>            a standalone drill
//   == chain <id>            a chain's scenario; its steps follow as
//   == step <chain-id> <n>
//   title: / kind: / scenario:            text (scenario may repeat; lines are joined)
//   given: key = <quantity> | label       shown before you answer
//   ask: key | type | dim | label [| tol] what you estimate; tol is the allowed factor (default 3)
//   answer: key = <quantity>              the reference answer, in round numbers
//   assume: text                          the reference's assumptions
//   calc: tag | text                      one derivation step; the tag names the kind of step
//   design: text                          what the number means for the design

struct Given {
    string key, text, label;
    Quantity q;
};

struct Ask {
    string key, type, label;
    Dim dim = Dim::None;
    double tol = 3;
};

struct Calc {
    string tag, text;
};

struct Item {
    string rec;    // drill, chain, step
    string id;     // drill or chain id; for a step, "<chain>#<n>"
    string chain;  // steps only
    int order = 0;
    string title, kind, type;  // type: the quantity type it is filed under (default: the first ask's)
    vector<string> scenario;
    vector<Given> givens;
    vector<Ask> asks;
    std::map<string, string> answer_text;
    std::map<string, Quantity> answer;
    vector<string> assumes;
    vector<Calc> calcs;
    vector<string> design;
    string where;  // file:line, for error messages
};

static const vector<std::pair<string, string>> kTypes = {
    {"users", "使用者規模"}, {"qps", "QPS"},     {"storage", "儲存量"},         {"bandwidth", "頻寬"},
    {"memory", "記憶體"},    {"servers", "伺服器與分片"}, {"fanout", "扇出"}, {"time", "時間與延遲"}};

static const vector<std::pair<string, string>> kTags = {
    {"assume", "假設本身"},     {"unit", "單位或時間換算"}, {"peak", "尖峰倍數"},
    {"mix", "讀寫比或比例"},    {"size", "單筆大小"},       {"mult", "乘上數量或時間"},
    {"copy", "副本、索引與額外負擔"}, {"cap", "單機容量與餘裕"}, {"fan", "扇出倍數"}};

static string type_label(const string& t) {
    for (auto& [k, v] : kTypes)
        if (k == t) return v;
    return t;
}

static string tag_label(const string& t) {
    for (auto& [k, v] : kTags)
        if (k == t) return v;
    return t;
}

static fs::path g_kit;
static fs::path g_progress;
static vector<Item> g_items;
static vector<string> g_bank_errors;

static void locate_kit() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof buf / sizeof buf[0]));
    fs::path self = fs::path(std::wstring(buf, n));
#else
    std::error_code ec;
    fs::path self = fs::canonical("/proc/self/exe", ec);
#endif
    g_kit = self.parent_path().parent_path();
    if (!fs::exists(g_kit / "drills")) {
        if (fs::exists(fs::current_path() / "drills")) g_kit = fs::current_path();
        else die("找不到 drills/ 資料夾。est 要放在 estimation-practice/build/ 底下執行。");
    }
    if (const char* p = std::getenv("EST_PROGRESS"); p && *p) g_progress = upath(p);
    else g_progress = g_kit / "progress";
}

static void bank_error(const string& where, const string& msg) { g_bank_errors.push_back(where + "：" + msg); }

static void parse_field(Item* cur, const string& where, const string& key, const string& val) {
    if (key == "title") cur->title = val;
    else if (key == "kind") cur->kind = val;
    else if (key == "type") cur->type = val;
    else if (key == "scenario") cur->scenario.push_back(val);
    else if (key == "assume") cur->assumes.push_back(val);
    else if (key == "design") cur->design.push_back(val);
    else if (key == "given") {
        auto eq = val.find('='), bar = val.find('|');
        if (eq == string::npos || bar == string::npos || bar < eq) {
            bank_error(where, "given 要寫成 key = 數量 | 說明");
            return;
        }
        Given g{trim(val.substr(0, eq)), trim(val.substr(eq + 1, bar - eq - 1)), trim(val.substr(bar + 1)), {}};
        string err;
        if (!parse_quantity(g.text, g.q, err)) bank_error(where, "given「" + g.text + "」：" + err);
        cur->givens.push_back(g);
    } else if (key == "ask") {
        auto f = split(val, '|');
        if (f.size() < 4) {
            bank_error(where, "ask 要寫成 key | type | dim | 說明 [| 容許倍數]");
            return;
        }
        Ask a;
        a.key = trim(f[0]);
        a.type = trim(f[1]);
        a.label = trim(f[3]);
        if (auto d = dim_from_key(trim(f[2]))) a.dim = *d;
        else bank_error(where, "不認得的 dim「" + trim(f[2]) + "」");
        if (std::none_of(kTypes.begin(), kTypes.end(), [&](auto& p) { return p.first == a.type; }))
            bank_error(where, "不認得的題型「" + a.type + "」");
        if (f.size() >= 5) a.tol = std::atof(trim(f[4]).c_str());
        if (a.tol < 1.5) bank_error(where, "容許倍數至少 1.5");
        cur->asks.push_back(a);
    } else if (key == "answer") {
        auto eq = val.find('=');
        if (eq == string::npos) {
            bank_error(where, "answer 要寫成 key = 數量");
            return;
        }
        string k = trim(val.substr(0, eq)), text = trim(val.substr(eq + 1));
        Quantity q;
        string err;
        if (!parse_quantity(text, q, err)) bank_error(where, "answer「" + text + "」：" + err);
        cur->answer_text[k] = text;
        cur->answer[k] = q;
    } else if (key == "calc") {
        auto bar = val.find('|');
        if (bar == string::npos) {
            bank_error(where, "calc 要寫成 tag | 內容");
            return;
        }
        Calc c{trim(val.substr(0, bar)), trim(val.substr(bar + 1))};
        if (std::none_of(kTags.begin(), kTags.end(), [&](auto& p) { return p.first == c.tag; }))
            bank_error(where, "不認得的步驟類型「" + c.tag + "」");
        cur->calcs.push_back(c);
    } else {
        bank_error(where, "不認得的欄位「" + key + "」");
    }
}

static void check_bank() {
    std::set<string> ids;
    for (auto& it : g_items) {
        if (!ids.insert(it.id).second) bank_error(it.where, "id 重複：" + it.id);
        if (it.rec == "chain") {
            if (it.title.empty() || it.scenario.empty() || it.kind.empty())
                bank_error(it.where, it.id + " 缺 title、kind 或 scenario");
            if (it.assumes.empty()) bank_error(it.where, it.id + " 沒有 assume");
            continue;
        }
        if (it.rec == "drill" && (it.title.empty() || it.scenario.empty() || it.kind.empty()))
            bank_error(it.where, it.id + " 缺 title、kind 或 scenario");
        if (it.rec == "drill" && it.assumes.empty()) bank_error(it.where, it.id + " 沒有 assume");
        if (!it.type.empty() && std::none_of(kTypes.begin(), kTypes.end(), [&](auto& p) { return p.first == it.type; }))
            bank_error(it.where, it.id + " 的 type「" + it.type + "」不認得");
        if (it.rec == "step" && it.title.empty()) bank_error(it.where, it.id + " 缺 title");
        if (it.asks.empty()) bank_error(it.where, it.id + " 沒有 ask");
        if (it.calcs.empty()) bank_error(it.where, it.id + " 沒有 calc");
        for (auto& a : it.asks) {
            if (!it.answer.count(a.key)) {
                bank_error(it.where, it.id + " 的 " + a.key + " 沒有 answer");
                continue;
            }
            const Quantity& q = it.answer.at(a.key);
            if (q.d != a.dim) bank_error(it.where, it.id + " 的 " + a.key + "：答案的單位是" + dim_name(q.d));
            if (!(q.v > 0)) bank_error(it.where, it.id + " 的 " + a.key + "：答案要大於 0");
        }
        for (auto& [k, text] : it.answer_text)
            if (std::none_of(it.asks.begin(), it.asks.end(), [&](const Ask& a) { return a.key == k; }))
                bank_error(it.where, it.id + " 的 answer " + k + " 沒有對應的 ask");
    }
    for (auto& it : g_items) {
        if (it.rec != "chain") continue;
        int n = 0;
        for (auto& s : g_items)
            if (s.rec == "step" && s.chain == it.id && s.order != ++n)
                bank_error(s.where, it.id + " 的步驟要從 1 依序編號");
        if (n < 2) bank_error(it.where, it.id + " 至少要 2 步");
    }
    for (auto& s : g_items)
        if (s.rec == "step" && std::none_of(g_items.begin(), g_items.end(), [&](const Item& c) {
                return c.rec == "chain" && c.id == s.chain;
            }))
            bank_error(s.where, "找不到串連 " + s.chain);
}

static void load_bank() {
    vector<fs::path> files;
    for (auto& e : fs::directory_iterator(g_kit / "drills"))
        if (e.path().extension() == ".txt") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (auto& f : files) {
        std::istringstream in(read_text(f));
        string line;
        int ln = 0;
        bool have = false;
        while (std::getline(in, line)) {
            ++ln;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (ln == 1 && starts_with(line, "\xEF\xBB\xBF")) line.erase(0, 3);
            string t = trim(line);
            string where = ustr(f.filename()) + ":" + std::to_string(ln);
            if (t.empty() || t[0] == '#') continue;
            if (starts_with(t, "== ")) {
                std::istringstream hs(t.substr(3));
                Item it;
                hs >> it.rec;
                it.where = where;
                if (it.rec == "drill" || it.rec == "chain") {
                    hs >> it.id;
                } else if (it.rec == "step") {
                    hs >> it.chain >> it.order;
                    it.id = it.chain + "#" + std::to_string(it.order);
                } else {
                    bank_error(where, "不認得的紀錄種類「" + it.rec + "」");
                    have = false;
                    continue;
                }
                if (it.id.empty()) bank_error(where, "缺 id");
                g_items.push_back(it);
                have = true;
                continue;
            }
            if (!have) {
                bank_error(where, "在 == 標題之前出現內容");
                continue;
            }
            auto colon = t.find(':');
            if (colon == string::npos) {
                bank_error(where, "缺「key: value」");
                continue;
            }
            parse_field(&g_items.back(), where, trim(t.substr(0, colon)), trim(t.substr(colon + 1)));
        }
    }
    check_bank();
}

static const Item* find_item(const string& id) {
    for (auto& it : g_items)
        if (it.id == id && it.rec != "step") return &it;
    return nullptr;
}

static vector<const Item*> items_of(const string& rec) {
    vector<const Item*> out;
    for (auto& it : g_items)
        if (it.rec == rec) out.push_back(&it);
    return out;
}

static vector<const Item*> steps_of(const string& chain) {
    vector<const Item*> out;
    for (auto& it : g_items)
        if (it.rec == "step" && it.chain == chain) out.push_back(&it);
    return out;
}

// The quantity type a drill is filed under: the type of its first question.
static string primary_type(const Item& it) {
    if (!it.type.empty()) return it.type;
    return it.asks.empty() ? "" : it.asks[0].type;
}

// ---------------------------------------------------------------- progress: logs and scheduling

struct Review {
    string date, id, result;  // result: hit, slow, miss
    int interval = 0;
    string next;
    long long seconds = 0;
    string mode;
};

struct Attempt {
    string date, id, q, type;
    double answer = 0, ref = 0, lr = 0;
    string result;
    long long seconds = 0;
    int rejects = 0;
    string diverge, tag, mode;
};

static fs::path reviews_file() { return g_progress / "reviews.tsv"; }
static fs::path attempts_file() { return g_progress / "attempts.tsv"; }
static fs::path journal_file() { return g_progress / "journal.md"; }

static vector<vector<string>> read_tsv(const fs::path& p, size_t min_cols) {
    vector<vector<string>> rows;
    std::istringstream in(read_text(p));
    string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        auto f = split(line, '\t');
        if (f.size() >= min_cols) rows.push_back(f);
    }
    return rows;
}

static vector<Review> load_reviews() {
    vector<Review> out;
    for (auto& f : read_tsv(reviews_file(), 7))
        out.push_back({f[0], f[1], f[2], std::atoi(f[3].c_str()), f[4], std::atoll(f[5].c_str()), f[6]});
    return out;
}

static vector<Attempt> load_attempts() {
    vector<Attempt> out;
    for (auto& f : read_tsv(attempts_file(), 13)) {
        Attempt a;
        a.date = f[0];
        a.id = f[1];
        a.q = f[2];
        a.type = f[3];
        a.answer = std::atof(f[4].c_str());
        a.ref = std::atof(f[5].c_str());
        a.lr = std::atof(f[6].c_str());
        a.result = f[7];
        a.seconds = std::atoll(f[8].c_str());
        a.rejects = std::atoi(f[9].c_str());
        a.diverge = f[10];
        a.tag = f[11];
        a.mode = f[12];
        out.push_back(a);
    }
    return out;
}

static std::map<string, Review> latest_reviews() {
    std::map<string, Review> m;
    for (auto& r : load_reviews()) m[r.id] = r;
    return m;
}

// The same ladder as cpp-practice's pp: a hit moves up 3 -> 7 -> 14 -> 30 -> 60 days; a hit over
// the time limit halves the interval; a miss comes back tomorrow.
static int next_interval(const string& id, const string& result) {
    static const int ladder[] = {1, 3, 7, 14, 30, 60, 120};
    auto latest = latest_reviews();
    if (!latest.count(id)) return result == "hit" ? 3 : 1;
    int last = latest[id].interval;
    if (result == "hit") {
        for (int x : ladder)
            if (x > last) return x;
        return last * 2;
    }
    if (result == "slow") return std::max(1, last / 2);
    return 1;
}

static bool retired(const Review& r) { return r.result == "hit" && r.interval >= 60; }

static bool is_due(const Review& r) { return !retired(r) && parse_date(r.next) <= today(); }

static string status_of(const string& id, const std::map<string, Review>& latest) {
    auto it = latest.find(id);
    if (it == latest.end()) return "新題";
    const Review& r = it->second;
    if (retired(r)) return "已穩定";
    if (is_due(r)) return "到期（上次 " + r.result + "）";
    return "下次 " + r.next + "（上次 " + r.result + "）";
}

static void record_review(const string& id, const string& result, long long seconds, const string& mode) {
    int interval = next_interval(id, result);
    Days next = today() + std::chrono::days{interval};
    if (!fs::exists(reviews_file())) write_text(reviews_file(), "# date\tid\tresult\tinterval\tnext\tseconds\tmode\n");
    append_text(reviews_file(), fmt_date(today()) + "\t" + id + "\t" + result + "\t" + std::to_string(interval) + "\t" +
                                    fmt_date(next) + "\t" + std::to_string(seconds) + "\t" + mode + "\n");
    std::cout << "  下次複習：" << fmt_date(next) << "（" << interval << " 天後）\n";
}

static void record_attempt(const Attempt& a) {
    if (!fs::exists(attempts_file()))
        write_text(attempts_file(),
                   "# date\tid\tq\ttype\tanswer\tref\tlog10\tresult\tseconds\tunit_rejects\tdiverge\ttag\tmode\n");
    char nums[160];
    std::snprintf(nums, sizeof nums, "%.6g\t%.6g\t%.4f", a.answer, a.ref, a.lr);
    append_text(attempts_file(), a.date + "\t" + a.id + "\t" + a.q + "\t" + a.type + "\t" + nums + "\t" + a.result +
                                     "\t" + std::to_string(a.seconds) + "\t" + std::to_string(a.rejects) + "\t" +
                                     one_line(a.diverge) + "\t" + a.tag + "\t" + a.mode + "\n");
}

// ---------------------------------------------------------------- one attempt: show, answer, score, reflect

static int g_minutes = 5;
static bool g_minutes_set = false;
static std::mt19937 g_rng{std::random_device{}()};

struct EndOfInput {};

static string must_line(const string& prompt) {
    std::cout << prompt;
    string s;
    if (!read_line(s)) throw EndOfInput{};
    return s;
}

// Shows an answer as the learner sees it: counts keep their noun ("50 台").
static string shown(const Quantity& q) {
    string f = fmt_q(q.d, q.v);
    if (q.d == Dim::Count && !q.unit.empty()) f += " " + q.unit;
    return f;
}

// The reference answer as written in the bank, plus its conversion when that adds something.
static string shown_ref(const string& text, const Quantity& q) {
    string f = fmt_q(q.d, q.v);
    if (q.d == Dim::Count || starts_with(f, text)) return q.d == Dim::Count ? text : f;
    return text + "（＝ " + f + "）";
}

struct QResult {
    string key, type, label;
    Quantity ans, ref;
    double lr = 0, tol = 3;
    bool hit = false;
    int rejects = 0;
};

static void show_givens(const Item& it) {
    if (it.givens.empty()) return;
    std::cout << "已知：\n";
    for (auto& g : it.givens) std::cout << "  - " << g.label << "：" << g.text << "\n";
}

static void show_asks(const Item& it) {
    std::cout << "要估（每個答案都要寫單位）：\n";
    for (size_t i = 0; i < it.asks.size(); ++i) {
        const Ask& a = it.asks[i];
        std::cout << "  " << i + 1 << ". " << a.label << "（" << dim_name(a.dim) << "；容許 " << num(a.tol) << " 倍）\n";
    }
}

static bool has_digit(const string& s) { return std::any_of(s.begin(), s.end(), is_digit); }

static vector<string> ask_assumptions(const string& intro) {
    std::cout << intro << "\n";
    vector<string> lines;
    for (;;) {
        string s = trim(must_line("  假設> "));
        if (!s.empty()) {
            lines.push_back(s);
            continue;
        }
        if (std::any_of(lines.begin(), lines.end(), has_digit)) break;
        std::cout << "  還沒有寫下含數字的假設。分數取決於假設：先寫下來（例如「尖峰 = 平均 × 3」「每筆 1 KB」），再作答。\n";
    }
    return lines;
}

static vector<QResult> ask_answers(const Item& it) {
    std::cout << "作答（數字加單位，例如 3e5 /s、300k QPS、2.5M users、1.2 TB、40 MB/s）：\n";
    vector<QResult> out;
    for (size_t i = 0; i < it.asks.size(); ++i) {
        const Ask& a = it.asks[i];
        QResult r;
        r.key = a.key;
        r.type = a.type;
        r.label = a.label;
        r.tol = a.tol;
        r.ref = it.answer.at(a.key);
        for (;;) {
            string s = must_line("  " + std::to_string(i + 1) + ". " + a.label + " = ");
            Quantity q;
            string err;
            if (!parse_quantity(s, q, err)) {
                std::cout << "     " << err << "\n";
                r.rejects++;
                continue;
            }
            if (string mm = dim_mismatch(q, a.dim); !mm.empty()) {
                std::cout << "     " << mm << "\n";
                r.rejects++;
                continue;
            }
            if (!(q.v > 0)) {
                std::cout << "     答案要大於 0。\n";
                continue;
            }
            r.ans = q;
            std::cout << "     收到：" << shown(q) << "\n";
            break;
        }
        r.lr = std::log10(r.ans.v / r.ref.v);
        r.hit = std::fabs(r.lr) <= std::log10(r.tol) + 1e-9;
        out.push_back(r);
    }
    return out;
}

static string signed2(double x) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%+.2f", x);
    return buf;
}

static string verdict(const QResult& r) {
    double f = std::pow(10.0, std::fabs(r.lr));
    string dir = r.lr > 0 ? "高估 " : "低估 ";
    if (r.hit) return f < 1.25 ? "命中" : "命中（" + dir + num(f, 2) + " 倍，在容許的 " + num(r.tol) + " 倍內）";
    return "沒命中：" + dir + num(f, 2) + " 倍（容許 " + num(r.tol) + " 倍）";
}

static void print_results(const Item& it, const vector<QResult>& rs) {
    for (size_t i = 0; i < rs.size(); ++i) {
        const QResult& r = rs[i];
        std::cout << "  " << i + 1 << ". " << r.label << "\n     你的答案 " << shown(r.ans) << "；參考答案 "
                  << shown_ref(it.answer_text.at(r.key), r.ref) << "\n     log10(你的 / 參考) = "
                  << signed2(r.lr) << " → " << verdict(r) << "\n";
    }
}

static void print_reference(const Item& it) {
    if (!it.assumes.empty()) {
        std::cout << "參考答案的假設：\n";
        for (auto& a : it.assumes) std::cout << "  - " << a << "\n";
    }
    std::cout << "推導（整數、標單位）：\n";
    for (size_t i = 0; i < it.calcs.size(); ++i)
        std::cout << "  " << i + 1 << ". " << it.calcs[i].text << "　〔" << tag_label(it.calcs[i].tag) << "〕\n";
    if (!it.design.empty()) {
        std::cout << "對設計的意義：\n";
        for (auto& d : it.design) std::cout << "  - " << d << "\n";
    }
}

struct Reflection {
    string step = "-", tag, note;
};

// Asks where the estimate diverged. The note is required after a miss or an overtime attempt;
// without it nothing is recorded (input ending here throws EndOfInput).
static Reflection reflect(const Item& it, bool missed, bool slow) {
    Reflection out;
    int n = (int)it.calcs.size();
    std::cout << "\n" << (missed ? "你的估算從哪一步開始偏離參考？" : "你的數字跟參考差最多的是哪一步？")
              << "輸入上面推導的步驟編號 1-" << n << "，0 = 假設本身就不同" << (missed ? "" : "，- = 幾乎一樣") << "。\n";
    for (;;) {
        string s = trim(must_line("  步驟> "));
        if (!missed && s == "-") break;
        if (!s.empty() && std::all_of(s.begin(), s.end(), is_digit) && s.size() < 4) {
            int k = std::atoi(s.c_str());
            if (k >= 0 && k <= n) {
                out.step = s;
                out.tag = k == 0 ? "assume" : it.calcs[(size_t)k - 1].tag;
                break;
            }
        }
        std::cout << "  請輸入 0 到 " << n << (missed ? "" : "，或 -") << "。\n";
    }
    bool need = missed || slow;
    std::cout << (missed ? "一句話：為什麼在那裡偏離，下次怎麼避免？（必填，寫下來才會記錄）\n"
                  : slow ? "一句話：超時了，哪一步花最多時間、下次怎麼更快？（必填，寫下來才會記錄）\n"
                         : "一句話心得（可以留空）：\n");
    for (;;) {
        string s = trim(must_line("  原因> "));
        if (!s.empty() || !need) {
            out.note = s;
            break;
        }
        std::cout << "  沒有寫原因，這次不記錄。說得出偏離的那一步，才知道下次要練什麼。\n";
    }
    return out;
}

static string outcome_name(bool all_hit, bool slow) { return !all_hit ? "miss" : slow ? "slow" : "hit"; }

static void record_results(const Item& it, const vector<QResult>& rs, long long secs, const Reflection& rf,
                           const string& mode) {
    for (auto& r : rs) {
        Attempt a;
        a.date = fmt_date(today());
        a.id = it.id;
        a.q = r.key;
        a.type = r.type;
        a.answer = r.ans.v;
        a.ref = r.ref.v;
        a.lr = r.lr;
        a.result = r.hit ? "hit" : "miss";
        a.seconds = secs;
        a.rejects = r.rejects;
        a.diverge = rf.step + (rf.note.empty() ? "" : "：" + rf.note);
        a.tag = rf.tag.empty() ? "-" : rf.tag;
        a.mode = mode;
        record_attempt(a);
    }
}

static void journal(const Item& it, const string& result, long long secs, const string& mode,
                    const vector<string>& assumptions, const vector<QResult>& rs, const Reflection& rf) {
    std::ostringstream o;
    o << "## " << fmt_date(today()) << " " << it.id << "（" << result << "，" << clock_text(secs) << "，" << mode << "）\n\n";
    o << "- 假設：\n";
    for (auto& a : assumptions) o << "  - " << a << "\n";
    for (auto& r : rs)
        o << "- " << r.label << "：" << shown(r.ans) << "（參考 " << fmt_q(r.ref.d, r.ref.v) << "，log10 "
          << signed2(r.lr) << "）\n";
    o << "- 偏離：" << (rf.step == "-" ? "沒有" : "第 " + rf.step + " 步（" + tag_label(rf.tag) + "）")
      << (rf.note.empty() ? "" : "：" + rf.note) << "\n\n";
    append_text(journal_file(), o.str());
}

enum class Outcome { Hit, Slow, Miss, Aborted };

static Outcome run_drill(const Item& it, const string& mode, size_t index, size_t total) {
    try {
        std::cout << "\n━━ ";
        if (total > 1) std::cout << "第 " << index << "/" << total << " 題　";
        std::cout << "[" << it.id << "] " << it.title << "\n類型：" << it.kind << "　題型："
                  << type_label(primary_type(it)) << "\n\n";
        for (auto& s : it.scenario) std::cout << s << "\n";
        std::cout << "\n";
        show_givens(it);
        show_asks(it);
        long long budget = (long long)g_minutes * 60;
        std::cout << "\n限時 " << g_minutes << " 分鐘，時鐘開始。\n";
        auto t0 = std::chrono::steady_clock::now();
        auto assumptions = ask_assumptions("先寫假設，一行一個，空白行結束。至少一行要有數字和單位。");
        auto rs = ask_answers(it);
        long long secs =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - t0).count();
        bool slow = secs > budget;
        bool all_hit = std::all_of(rs.begin(), rs.end(), [](const QResult& r) { return r.hit; });
        std::cout << "\n用時 " << clock_text(secs) << " / " << g_minutes << " 分鐘" << (slow ? "（超時）" : "")
                  << "\n\n── 結果\n";
        print_results(it, rs);
        std::cout << "\n── 參考推導\n";
        print_reference(it);
        Reflection rf = reflect(it, !all_hit, slow);
        string result = outcome_name(all_hit, slow);
        record_results(it, rs, secs, rf, mode);
        journal(it, result, secs, mode, assumptions, rs, rf);
        std::cout << "已記錄：" << it.id << " " << result << "\n";
        record_review(it.id, result, secs, mode);
        return !all_hit ? Outcome::Miss : slow ? Outcome::Slow : Outcome::Hit;
    } catch (const EndOfInput&) {
        std::cout << "\n輸入結束，這題不記錄。\n";
        return Outcome::Aborted;
    }
}

static Outcome run_chain(const Item& ch) {
    auto steps = steps_of(ch.id);
    int per_step = g_minutes_set ? g_minutes : 3;
    try {
        std::cout << "\n━━ 串連練習 [" << ch.id << "] " << ch.title << "\n類型：" << ch.kind << "\n\n";
        for (auto& s : ch.scenario) std::cout << s << "\n";
        std::cout << "\n";
        show_givens(ch);
        std::cout << "步驟：";
        for (size_t i = 0; i < steps.size(); ++i)
            std::cout << (i ? " → " : "") << i + 1 << " " << type_label(primary_type(*steps[i]));
        std::cout << "\n每一步作答後會揭曉參考答案，下一步用參考答案繼續：每一步只量那一步的估算，一步錯不會拖累後面。\n"
                  << "每步建議 " << per_step << " 分鐘。\n\n";
        auto assumptions = ask_assumptions("先寫整條鏈的假設，一行一個，空白行結束。至少一行要有數字和單位。");
        vector<vector<QResult>> all;
        vector<long long> times;
        for (size_t k = 0; k < steps.size(); ++k) {
            const Item& st = *steps[k];
            std::cout << "\n── 第 " << k + 1 << "/" << steps.size() << " 步：" << st.title << "\n";
            for (auto& s : st.scenario) std::cout << s << "\n";
            show_givens(st);
            show_asks(st);
            auto t0 = std::chrono::steady_clock::now();
            auto rs = ask_answers(st);
            long long secs =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - t0).count();
            std::cout << "  用時 " << clock_text(secs) << (secs > per_step * 60 ? "（超時）" : "") << "\n";
            print_results(st, rs);
            print_reference(st);
            all.push_back(rs);
            times.push_back(secs);
        }
        long long total = 0;
        bool any_miss = false, any_slow = false;
        std::cout << "\n── 整條鏈\n";
        for (size_t k = 0; k < steps.size(); ++k) {
            total += times[k];
            any_slow |= times[k] > per_step * 60;
            for (auto& r : all[k]) {
                any_miss |= !r.hit;
                std::cout << "  " << k + 1 << ". " << r.label << "：你的 " << shown(r.ans) << "，參考 "
                          << shown_ref(steps[k]->answer_text.at(r.key), r.ref) << "，log10 " << signed2(r.lr) << (r.hit ? "　命中" : "　沒命中")
                          << "\n";
            }
        }
        std::cout << "  總用時 " << clock_text(total) << "（建議 " << per_step * steps.size() << " 分鐘）\n";
        if (!ch.assumes.empty()) {
            std::cout << "參考答案整條鏈共用的假設：\n";
            for (auto& a : ch.assumes) std::cout << "  - " << a << "\n";
        }
        std::cout << "\n" << (any_miss ? "從第幾步開始偏離？" : "差最多的是第幾步？") << "輸入 1-" << steps.size()
                  << (any_miss ? "" : "，- = 都很接近") << "。\n";
        size_t pick = 0;
        for (;;) {
            string s = trim(must_line("  第幾步> "));
            if (!any_miss && s == "-") break;
            int k = std::atoi(s.c_str());
            if (has_digit(s) && k >= 1 && k <= (int)steps.size()) {
                pick = (size_t)k;
                break;
            }
            std::cout << "  請輸入 1 到 " << steps.size() << (any_miss ? "" : "，或 -") << "。\n";
        }
        Reflection rf;
        if (pick) {
            const Item& st = *steps[pick - 1];
            bool missed_here = std::any_of(all[pick - 1].begin(), all[pick - 1].end(), [](auto& r) { return !r.hit; });
            std::cout << "第 " << pick << " 步的推導：\n";
            for (size_t i = 0; i < st.calcs.size(); ++i)
                std::cout << "  " << i + 1 << ". " << st.calcs[i].text << "\n";
            rf = reflect(st, missed_here || any_miss, any_slow);
            rf.step = std::to_string(pick) + "." + rf.step;
        } else if (any_slow) {
            for (;;) {
                string s = trim(must_line("  超時了：一句話寫下哪一步花最多時間（必填）> "));
                if (!s.empty()) {
                    rf.note = s;
                    break;
                }
                std::cout << "  沒有寫原因，這次不記錄。\n";
            }
        }
        string result = outcome_name(!any_miss, any_slow);
        for (size_t k = 0; k < steps.size(); ++k) {
            Reflection r2 = rf;
            if (pick != k + 1) r2.tag = "";
            record_results(*steps[k], all[k], times[k], r2, "chain");
        }
        vector<QResult> flat;
        for (auto& v : all) flat.insert(flat.end(), v.begin(), v.end());
        journal(ch, result, total, "chain", assumptions, flat, rf);
        std::cout << "已記錄：" << ch.id << " " << result << "\n";
        record_review(ch.id, result, total, "chain");
        return any_miss ? Outcome::Miss : any_slow ? Outcome::Slow : Outcome::Hit;
    } catch (const EndOfInput&) {
        std::cout << "\n輸入結束，這次串連不記錄。\n";
        return Outcome::Aborted;
    }
}

// ---------------------------------------------------------------- choosing drills: due first, interleaved

static std::map<string, int> attempts_by_type() {
    std::map<string, int> m;
    for (auto& a : load_attempts()) m[a.type]++;
    return m;
}

static vector<const Item*> pick_session(size_t n, const string& type, bool due_only) {
    auto latest = latest_reviews();
    vector<const Item*> due, fresh, rest;
    for (auto* it : items_of("drill")) {
        if (!type.empty() && primary_type(*it) != type) continue;
        auto f = latest.find(it->id);
        if (f == latest.end()) fresh.push_back(it);
        else if (is_due(f->second)) due.push_back(it);
        else if (!retired(f->second)) rest.push_back(it);
    }
    auto by_next = [&](const Item* a, const Item* b) { return latest[a->id].next < latest[b->id].next; };
    std::stable_sort(due.begin(), due.end(), by_next);
    std::shuffle(fresh.begin(), fresh.end(), g_rng);
    std::stable_sort(rest.begin(), rest.end(), by_next);
    auto practiced = attempts_by_type();
    vector<const Item*> out;
    string last;
    // Takes the first item whose type differs from the previous pick; among new drills, the least
    // practiced type wins. Falls back to the same type only when nothing else is left in the pool.
    auto take = [&](vector<const Item*>& pool, bool least_practiced) {
        int best = -1;
        for (int i = 0; i < (int)pool.size(); ++i) {
            if (primary_type(*pool[i]) == last) continue;
            if (best < 0) best = i;
            if (!least_practiced) break;
            if (practiced[primary_type(*pool[i])] < practiced[primary_type(*pool[best])]) best = i;
        }
        if (best < 0 && !pool.empty()) best = 0;
        if (best < 0) return false;
        out.push_back(pool[(size_t)best]);
        last = primary_type(*pool[(size_t)best]);
        practiced[last]++;
        pool.erase(pool.begin() + best);
        return true;
    };
    while (out.size() < n) {
        if (take(due, false)) continue;
        if (due_only) break;
        if (take(fresh, true)) continue;
        if (!take(rest, false)) break;
    }
    return out;
}

static int session(const vector<const Item*>& list, const string& mode) {
    int hit = 0, slow = 0, miss = 0, aborted = 0;
    for (size_t i = 0; i < list.size(); ++i) {
        Outcome o = run_drill(*list[i], mode, i + 1, list.size());
        if (o == Outcome::Hit) ++hit;
        else if (o == Outcome::Slow) ++slow;
        else if (o == Outcome::Miss) ++miss;
        else {
            ++aborted;
            break;
        }
    }
    if (list.size() > 1)
        std::cout << "\n本次：命中 " << hit << "、超時 " << slow << "、沒命中 " << miss
                  << (aborted ? "、中斷 1（之後的題目沒有做）" : "") << "。看偏差：est stats\n";
    return aborted ? 1 : 0;
}

// ---------------------------------------------------------------- commands

static bool is_type(const string& t) {
    return std::any_of(kTypes.begin(), kTypes.end(), [&](auto& p) { return p.first == t; });
}

static int cmd_drill(const vector<string>& a, bool review) {
    size_t n = review ? 5 : 3;
    string target;
    bool plan = false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] == "-n" && i + 1 < a.size()) n = (size_t)std::max(1, std::atoi(a[++i].c_str()));
        else if (a[i] == "--plan") plan = true;
        else if (target.empty() && !review) target = a[i];
        else die("看不懂的參數：" + a[i]);
    }
    if (!target.empty() && !is_type(target)) {
        const Item* it = find_item(target);
        if (!it) die("沒有這題或題型：" + target + "。用 est list 看清單。");
        if (it->rec == "chain") die(target + " 是串連練習：est chain " + target);
        return session({it}, "drill");
    }
    auto list = pick_session(n, target, review);
    if (list.empty()) {
        std::cout << (review ? "今天沒有到期的複習。做新題：est drill\n" : "沒有可以練的題目。\n");
        return 0;
    }
    std::cout << (review ? "到期複習 " : "交錯練習 ") << list.size() << " 題（題型不連續重複；先到期、再新題）。\n";
    if (plan) {
        auto latest = latest_reviews();
        for (auto* it : list)
            std::cout << "  " << it->id << "　" << type_label(primary_type(*it)) << "　[" << status_of(it->id, latest) << "]\n";
        return 0;
    }
    return session(list, review ? "review" : "drill");
}

static int cmd_chain(const vector<string>& a) {
    const Item* ch = nullptr;
    if (!a.empty()) {
        ch = find_item(a[0]);
        if (!ch || ch->rec != "chain") die("沒有這個串連：" + a[0] + "。用 est list 看清單。");
    } else {
        auto latest = latest_reviews();
        const Item* fresh = nullptr;
        const Item* soonest = nullptr;
        for (auto* c : items_of("chain")) {
            auto f = latest.find(c->id);
            if (f != latest.end() && is_due(f->second) && !ch) ch = c;
            if (f == latest.end() && !fresh) fresh = c;
            if (f != latest.end() && (!soonest || f->second.next < latest[soonest->id].next)) soonest = c;
        }
        if (!ch) ch = fresh ? fresh : soonest;
        if (!ch) die("題庫裡沒有串連練習。");
    }
    return run_chain(*ch) == Outcome::Aborted ? 1 : 0;
}

static int cmd_list() {
    auto latest = latest_reviews();
    auto all = items_of("drill");
    std::cout << "題庫：" << all.size() << " 題單題、" << items_of("chain").size() << " 條串連\n";
    for (auto& [t, label] : kTypes) {
        vector<const Item*> v;
        for (auto* it : all)
            if (primary_type(*it) == t) v.push_back(it);
        if (v.empty()) continue;
        std::cout << "\n" << label << "（" << v.size() << " 題）\n";
        for (auto* it : v) std::cout << "  " << it->id << "　" << it->title << "　[" << status_of(it->id, latest) << "]\n";
    }
    std::cout << "\n串連（使用者 → QPS → 儲存 → 頻寬 → 伺服器）\n";
    for (auto* c : items_of("chain"))
        std::cout << "  " << c->id << "　" << c->title << "　[" << status_of(c->id, latest) << "]\n";
    std::cout << "\n練一題：est drill <id>；交錯練 3 題：est drill；只練某題型：est drill qps\n";
    return 0;
}

static int cmd_today() {
    auto latest = latest_reviews();
    vector<string> due;
    int fresh = 0;
    for (auto* it : items_of("drill")) {
        auto f = latest.find(it->id);
        if (f == latest.end()) ++fresh;
        else if (is_due(f->second)) due.push_back(it->id + "（上次 " + f->second.result + "，到期 " + f->second.next + "）");
    }
    vector<string> chain_due;
    int chain_fresh = 0;
    for (auto* c : items_of("chain")) {
        auto f = latest.find(c->id);
        if (f == latest.end()) ++chain_fresh;
        else if (is_due(f->second)) chain_due.push_back(c->id);
    }
    std::cout << "今天 " << fmt_date(today()) << "\n";
    if (!due.empty()) {
        std::cout << "  到期複習 " << due.size() << " 題 → est review\n";
        for (auto& d : due) std::cout << "    - " << d << "\n";
    } else {
        std::cout << "  沒有到期的複習。\n";
    }
    if (due.size() > 5) std::cout << "  積壓超過 5 題：今天只做複習，不開新題。\n";
    else if (fresh) std::cout << "  新題還有 " << fresh << " 題 → est drill（交錯選 3 題，每題 " << g_minutes << " 分鐘）\n";
    else std::cout << "  單題都做過了：est drill 會挑最早到期的題目提前複習。\n";
    if (!chain_due.empty()) std::cout << "  到期的串連：" << chain_due[0] << " → est chain " << chain_due[0] << "\n";
    else if (chain_fresh) std::cout << "  還有 " << chain_fresh << " 條串連沒做過 → est chain（約 15 分鐘）\n";
    return 0;
}

static int cmd_stats() {
    auto as = load_attempts();
    if (as.empty()) {
        std::cout << "還沒有紀錄。先做幾題：est drill\n";
        return 0;
    }
    struct Agg {
        int n = 0, hit = 0, pos = 0, neg = 0, rejects = 0;
        double sum = 0, abs = 0;
    };
    std::map<string, Agg> by;
    Agg total;
    for (auto& a : as) {
        for (Agg* g : {&by[a.type], &total}) {
            g->n++;
            g->hit += a.result == "hit";
            g->sum += a.lr;
            g->abs += std::fabs(a.lr);
            g->pos += a.lr > 0.05;
            g->neg += a.lr < -0.05;
            g->rejects += a.rejects;
        }
    }
    auto line = [](const string& label, const Agg& g) {
        double mean = g.sum / g.n;
        string judge;
        if (g.n < 3) judge = "樣本少，做滿 3 次再看";
        else if (std::fabs(mean) < 0.15) judge = "沒有明顯偏差";
        else if (mean > 0 && g.pos * 3 >= g.n * 2) judge = "系統性高估";
        else if (mean < 0 && g.neg * 3 >= g.n * 2) judge = "系統性低估";
        else judge = "誤差大、方向不一致";
        char buf[256];
        std::snprintf(buf, sizeof buf, "次數 %-3d 命中 %-3d（%3.0f%%）  偏差 %s", g.n, g.hit, 100.0 * g.hit / g.n,
                      signed2(mean).c_str());
        string factor = std::fabs(mean) < 0.05 ? "（幾乎沒有）"
                                               : string(mean > 0 ? "（平均高估 " : "（平均低估 ") +
                                                     num(std::pow(10.0, std::fabs(mean)), 2) + " 倍）";
        std::cout << "  " << pad(label, 14) << buf << pad(factor, 22) << "平均誤差 " << pad(num(g.abs / g.n, 2), 6)
                  << "→ " << judge << "\n";
    };
    std::cout << "各題型的偏差：log10(你的答案 / 參考答案) 的平均。正 = 高估，負 = 低估；0.48 ≈ 3 倍，1 = 10 倍。\n";
    for (auto& [t, label] : kTypes)
        if (by.count(t)) line(label, by[t]);
    line("全部", total);
    std::cout << "  答案因為缺單位或單位不對被退回 " << total.rejects << " 次。\n";

    // Divergence tags, one per recorded attempt (rows of the same attempt share the tag).
    std::map<string, int> tags;
    std::set<string> seen;
    vector<string> notes;
    for (auto& a : as) {
        string key = a.date + a.id + a.diverge;
        if (a.result != "miss" || !seen.insert(key).second) continue;
        if (a.tag != "-" && !a.tag.empty()) tags[a.tag]++;
        notes.push_back(a.date + " " + a.id + "：" + a.diverge);
    }
    if (!tags.empty()) {
        vector<std::pair<int, string>> v;
        for (auto& [t, c] : tags) v.push_back({c, t});
        std::sort(v.rbegin(), v.rend());
        std::cout << "\n沒命中時，偏離發生在哪類步驟：\n";
        for (auto& [c, t] : v) std::cout << "  " << c << " 次　" << tag_label(t) << "\n";
    }
    if (!notes.empty()) {
        std::cout << "\n最近沒命中的題目（你寫的原因）：\n";
        for (size_t i = notes.size() > 5 ? notes.size() - 5 : 0; i < notes.size(); ++i) std::cout << "  " << notes[i] << "\n";
    }
    std::cout << "\n系統性高估：檢查是否把尖峰倍數或副本乘了兩次。系統性低估：檢查是否漏了副本、索引、保存年限，"
                 "或把位元當成位元組（差 8 倍）。\n";
    return 0;
}

static int cmd_parse(const vector<string>& a) {
    bool raw = !a.empty() && a[0] == "--raw";
    for (size_t i = raw ? 1 : 0; i < a.size(); ++i) {
        Quantity q;
        string err;
        bool ok = parse_quantity(a[i], q, err);
        if (raw) {
            char buf[64];
            std::snprintf(buf, sizeof buf, "%.12g", q.v);
            std::cout << (ok ? string("ok\t") + dim_key(q.d) + "\t" + buf : "err\t" + err) << "\n";
        } else {
            std::cout << "「" << a[i] << "」→ " << (ok ? fmt_q(q.d, q.v) + "（" + dim_name(q.d) + "）" : err) << "\n";
        }
    }
    return 0;
}

// ---------------------------------------------------------------- self-test

static int self_test() {
    struct Ok {
        const char* in;
        Dim d;
        double v;
    };
    const double day = 86400;
    const Ok oks[] = {
        {"3e5 QPS", Dim::Rate, 3e5},           {"3E5 qps", Dim::Rate, 3e5},
        {"300k QPS", Dim::Rate, 3e5},          {"300K req/s", Dim::Rate, 3e5},
        {"300k/s", Dim::Rate, 3e5},            {"2.5M users", Dim::Count, 2.5e6},
        {"2.5M 人", Dim::Count, 2.5e6},        {"1.2 TB", Dim::Bytes, 1.2e12},
        {"1.2TB", Dim::Bytes, 1.2e12},         {"500 GB", Dim::Bytes, 5e11},
        {"64 kB", Dim::Bytes, 64e3},           {"64 KB", Dim::Bytes, 64e3},
        {"1 GiB", Dim::Bytes, 1073741824.0},   {"64 KiB", Dim::Bytes, 65536},
        {"2^30 B", Dim::Bytes, 1073741824.0},  {"55 PB", Dim::Bytes, 5.5e16},
        {"8 Tb", Dim::Bytes, 1e12},            {"40 MB/s", Dim::ByteRate, 4e7},
        {"40MB/s", Dim::ByteRate, 4e7},        {"40 MB / s", Dim::ByteRate, 4e7},
        {"320 Mbps", Dim::ByteRate, 4e7},      {"10 Gbps", Dim::ByteRate, 1.25e9},
        {"10 gbps", Dim::ByteRate, 1.25e9},    {"10 Gb/s", Dim::ByteRate, 1.25e9},
        {"30 Tbps", Dim::ByteRate, 3.75e12},   {"2 TB/day", Dim::ByteRate, 2e12 / day},
        {"86,400 s", Dim::Time, 86400},        {"1,000,000 req/day", Dim::Rate, 1e6 / day},
        {"5億 次/天", Dim::Rate, 5e8 / day},   {"2000萬 人", Dim::Count, 2e7},
        {"2千萬人", Dim::Count, 2e7},          {"1.5億 個", Dim::Count, 1.5e8},
        {"3×10^5 /s", Dim::Rate, 3e5},         {"3x10^5 /s", Dim::Rate, 3e5},
        {"3 * 10^5 /s", Dim::Rate, 3e5},       {"10^5 s", Dim::Time, 1e5},
        {"500 µs", Dim::Time, 5e-4},           {"500 us", Dim::Time, 5e-4},
        {"150 ms", Dim::Time, 0.15},           {"2.5 s", Dim::Time, 2.5},
        {"22 min", Dim::Time, 1320},           {"1.8 h", Dim::Time, 6480},
        {"3 天", Dim::Time, 3 * day},          {"43 台", Dim::Count, 43},
        {"200 partitions", Dim::Count, 200},   {"200 個分片", Dim::Count, 200},
        {"34 GPU servers", Dim::Count, 34},    {"18 redis nodes", Dim::Count, 18},
        {"~15k QPS", Dim::Rate, 1.5e4},        {"≈ 50 QPS", Dim::Rate, 50},
        {"約 2 GB", Dim::Bytes, 2e9},          {"1bn /s", Dim::Rate, 1e9},
        {"2.5bn users", Dim::Count, 2.5e9},    {"4e9 pushes/s", Dim::Rate, 4e9},
        {"200k writes/s", Dim::Rate, 2e5},     {"3k 次每秒", Dim::Rate, 3000},
        {"3k req per second", Dim::Rate, 3000}, {"12 req/min", Dim::Rate, 0.2},
        {"40%", Dim::Ratio, 0.4},              {"3 倍", Dim::Ratio, 3},
        {"100 /s", Dim::Rate, 100},            {"0.5 /s", Dim::Rate, 0.5},
        {"2.5B", Dim::Bytes, 2.5},
    };
    struct Bad {
        const char* in;
        const char* err_part;
    };
    const Bad bads[] = {
        {"300k", "缺單位"},           {"3500", "缺單位"},           {"2.5M", "缺單位"},
        {"", "沒有輸入"},             {"QPS", "開頭要是數字"},      {"1.2 tb", "分不出"},
        {"40 mb/s", "小寫 m"},        {"3 gb", "分不出"},           {"5m", "min"},
        {"40 MB/kg", "不是時間單位"}, {"12 furlongs", "看不懂單位"}, {"3,00 QPS", "看不懂單位"},
        {"2.5B users", "十億"},       {"7 parsecs/s", "斜線前面"},  {"3 xyzbps", "看不懂"},
    };
    struct Mis {
        const char* in;
        Dim want;
        const char* err_part;
    };
    const Mis mis[] = {
        {"2.5B", Dim::Count, "十億"},         {"3000 requests", Dim::Rate, "每秒"},
        {"40 MB/s", Dim::Bytes, "總量"},      {"1.2 TB", Dim::ByteRate, "每秒"},
        {"12 台", Dim::Time, "時間"},         {"300 ms", Dim::Count, "數量"},
    };
    int fail = 0, n = 0;
    for (auto& c : oks) {
        ++n;
        Quantity q;
        string err;
        bool ok = parse_quantity(c.in, q, err);
        if (!ok || q.d != c.d || std::fabs(q.v - c.v) > 1e-9 * std::max(1.0, std::fabs(c.v))) {
            ++fail;
            std::cout << "  失敗：「" << c.in << "」→ " << (ok ? string(dim_key(q.d)) + " " + num(q.v, 6) : err) << "，應為 "
                      << dim_key(c.d) << " " << num(c.v, 6) << "\n";
        }
    }
    for (auto& c : bads) {
        ++n;
        Quantity q;
        string err;
        if (parse_quantity(c.in, q, err) || err.find(c.err_part) == string::npos) {
            ++fail;
            std::cout << "  失敗：「" << c.in << "」應該被拒絕並提到「" << c.err_part << "」，實際：" << err << "\n";
        }
    }
    for (auto& c : mis) {
        ++n;
        Quantity q;
        string err;
        string m = parse_quantity(c.in, q, err) ? dim_mismatch(q, c.want) : "";
        if (m.find(c.err_part) == string::npos) {
            ++fail;
            std::cout << "  失敗：「" << c.in << "」答在 " << dim_key(c.want) << " 題應該被退回並提到「" << c.err_part
                      << "」，實際：" << m << "\n";
        }
    }
    // Formatting spot checks.
    const std::pair<string, string> fmts[] = {{fmt_q(Dim::Bytes, 1.2e12), "1.2 TB"},
                                              {fmt_q(Dim::ByteRate, 4e7), "40 MB/s（320 Mbps）"},
                                              {fmt_q(Dim::Rate, 1.5e4), "15k /s"},
                                              {fmt_q(Dim::Count, 4e12), "4e12"},
                                              {fmt_q(Dim::Time, 1320), "22 min"}};
    for (auto& [got, want] : fmts) {
        ++n;
        if (got != want) {
            ++fail;
            std::cout << "  失敗：格式化得到「" << got << "」，應為「" << want << "」\n";
        }
    }
    std::cout << "單位解析：" << n - fail << "/" << n << " 通過（" << std::size(oks) << " 個可接受的寫法、"
              << std::size(bads) << " 個錯誤寫法、" << std::size(mis) << " 個單位種類不符、" << std::size(fmts)
              << " 個格式化）\n";

    if (!g_bank_errors.empty()) {
        std::cout << "題庫有 " << g_bank_errors.size() << " 個問題：\n";
        for (auto& e : g_bank_errors) std::cout << "  " << e << "\n";
        ++fail;
    }
    std::map<string, int> qs;
    for (auto& it : g_items)
        for (auto& a : it.asks) qs[a.type + (it.rec == "step" ? "（串連）" : "")]++;
    std::cout << "題庫：" << items_of("drill").size() << " 題單題、" << items_of("chain").size() << " 條串連（"
              << std::count_if(g_items.begin(), g_items.end(), [](const Item& i) { return i.rec == "step"; })
              << " 步）。各題型的提問數：";
    bool first = true;
    for (auto& [t, c] : qs) {
        std::cout << (first ? "" : "、") << t << " " << c;
        first = false;
    }
    std::cout << "\n" << (fail ? "自我測試失敗。\n" : "自我測試全部通過。\n");
    return fail ? 1 : 0;
}

// ---------------------------------------------------------------- main

static void usage() {
    std::cout << "est：容量估算練習工具（研究報告第 3.3 節）\n\n"
                 "  est today                     今天該做什麼：到期複習、新題、串連\n"
                 "  est drill [題目|題型] [-n N]   計時練習；不指定題目時交錯選 N 題（預設 3）\n"
                 "                                加 --plan 只列出會選到的題目，不開始\n"
                 "  est review [-n N]             只做到期的複習\n"
                 "  est chain [串連]              串連：使用者 → QPS → 儲存 → 頻寬 → 伺服器\n"
                 "  est stats                     各題型的命中率與高估／低估偏差\n"
                 "  est list                      題庫與每題的狀態\n"
                 "  est parse <文字> ...           試試單位怎麼解析\n"
                 "  est --self-test               單位解析測試與題庫檢查\n\n"
                 "選項：--minutes M（每題限時，預設 5）、--seed S（固定選題順序）\n"
                 "題型：users qps storage bandwidth memory servers fanout time\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    g_console_in = GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR;
#endif
    vector<string> all = utf8_args(argc, argv);
    vector<string> args;
    for (size_t i = 1; i < all.size(); ++i) {
        if (all[i] == "--minutes" && i + 1 < all.size()) {
            g_minutes = std::max(0, std::atoi(all[++i].c_str()));
            g_minutes_set = true;
        } else if (all[i] == "--seed" && i + 1 < all.size()) {
            g_rng.seed((unsigned)std::strtoul(all[++i].c_str(), nullptr, 10));
        } else {
            args.push_back(all[i]);
        }
    }
    try {
        locate_kit();
        load_bank();
        if (args.empty() || args[0] == "help" || args[0] == "--help") {
            usage();
            return 0;
        }
        string cmd = args[0];
        vector<string> rest(args.begin() + 1, args.end());
        if (cmd == "--self-test" || cmd == "self-test") return self_test();
        if (cmd == "parse") return cmd_parse(rest);
        if (!g_bank_errors.empty()) {
            std::cout << "題庫有問題，先修好再練習：\n";
            for (auto& e : g_bank_errors) std::cout << "  " << e << "\n";
            return 2;
        }
        if (cmd == "list") return cmd_list();
        if (cmd == "today") return cmd_today();
        if (cmd == "drill") return cmd_drill(rest, false);
        if (cmd == "review") return cmd_drill(rest, true);
        if (cmd == "chain") return cmd_chain(rest);
        if (cmd == "stats") return cmd_stats();
    } catch (const std::exception& e) {
        std::cout << "錯誤：" << e.what() << "\n";
        return 2;
    }
    usage();
    return 2;
}
