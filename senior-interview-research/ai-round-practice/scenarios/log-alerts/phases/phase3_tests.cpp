// Phase 3 tests: large logs. Each test writes its log files to data/generated/
// (open them: their shapes differ), parses them, and then times merge_logs + evaluate.
// Parsing is not timed.
#include <string>
#include <vector>

#include "alerts.hpp"
#include "check.hpp"
#include "event.hpp"
#include "merge.hpp"
#include "rules.hpp"
#include "timeparse.hpp"

namespace {

std::uint64_t alerts_hash(const std::vector<Alert>& alerts) {
    std::string text;
    for (const Alert& a : alerts)
        text += a.rule + "|" + a.service + "|" + a.code + "|" + std::to_string(a.at_ms) + "|" + std::to_string(a.count) + "\n";
    return check::fnv1a(text);
}

const char* kLevels[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
const std::int64_t kStart = 1794736800000;  // 2026-11-15T10:00:00Z

std::string line(std::int64_t ms, const std::string& svc, const char* level, const std::string& code) {
    return format_timestamp(ms) + " " + svc + " " + level + " " + code + " msg\n";
}

// One file, two hours, 70% of events are api/ERROR/E5003.
std::vector<std::string> hot_key_logs() {
    check::Rng rng(401);
    std::string text;
    std::int64_t t = kStart;
    for (int i = 0; i < 400000; ++i) {
        t += rng.range(0, 36);
        if (rng.range(0, 9) < 7) text += line(t, "api", "ERROR", "E5003");
        else text += line(t, "svc" + std::to_string(rng.range(0, 4)), kLevels[rng.range(0, 4)], "C" + std::to_string(rng.range(0, 49)));
    }
    return {text};
}

// One file, 40 services and 1,000 distinct codes: 40,000 keys, few events per key.
std::vector<std::string> many_keys_logs() {
    check::Rng rng(402);
    std::string text;
    std::int64_t t = kStart;
    for (int i = 0; i < 400000; ++i) {
        t += rng.range(0, 36);
        text += line(t, "svc" + std::to_string(rng.range(0, 39)), kLevels[rng.range(1, 4)], "K" + std::to_string(rng.range(0, 999)));
    }
    return {text};
}

// Four services, one file each, covering the same two hours: the files interleave.
std::vector<std::string> four_files_logs() {
    check::Rng rng(403);
    std::vector<std::string> files;
    for (int f = 0; f < 4; ++f) {
        std::string text;
        std::int64_t t = kStart;
        std::string svc = "svc" + std::to_string(f);
        for (int i = 0; i < 100000; ++i) {
            t += rng.range(0, 144);
            text += line(t, svc, kLevels[rng.range(1, 4)], "C" + std::to_string(rng.range(0, 99)));
        }
        files.push_back(text);
    }
    return files;
}

void run_large(const std::string& name, const std::vector<std::string>& texts, const std::string& rules_text,
               double limit_ms, std::size_t alerts, std::uint64_t hash) {
    std::vector<LogFile> files;
    for (std::size_t i = 0; i < texts.size(); ++i) {
        std::string path = check::write_generated(name + "-" + std::to_string(i + 1) + ".log", texts[i]);
        files.push_back(parse_log(check::read_file(path)));
    }
    auto rules = parse_rules(rules_text);
    std::vector<Alert> got;
    check::measure(limit_ms, [&] {
        std::vector<Event> timeline = merge_logs(files);
        got = evaluate(timeline, rules);
    });
    EXPECT_EQ(got.size(), alerts);
    EXPECT_EQ(alerts_hash(got), hash);
}

}  // namespace

TEST("large-1 one hot key with a one-hour window", 3) {
    run_large("large-1-hot", hot_key_logs(),
              "rule hot   service=api level=ERROR code=E5003 threshold=50000 window=1h  cooldown=10m\n"
              "rule err   service=*   level=ERROR code=*     threshold=40    window=10m cooldown=5m\n"
              "rule fatal service=*   level=FATAL code=*     threshold=1     window=1s  cooldown=1m\n",
              1500, 13244, 8004197120902861616ULL);
}

TEST("large-2 many distinct keys", 3) {
    run_large("large-2-keys", many_keys_logs(),
              "rule burst service=*    level=WARN  code=* threshold=4  window=30m cooldown=2h\n"
              "rule err   service=*    level=ERROR code=* threshold=3  window=1h  cooldown=1h\n"
              "rule svc7  service=svc7 level=INFO  code=* threshold=20 window=1m  cooldown=1m\n",
              1500, 62252, 7467082420233499207ULL);
}

TEST("large-3 four interleaved files", 3) {
    run_large("large-3-merge", four_files_logs(),
              "rule err service=* level=ERROR code=* threshold=30 window=10m cooldown=10m\n"
              "rule any service=* level=INFO  code=* threshold=200 window=10m cooldown=30m\n",
              1500, 4604, 16138832781944818347ULL);
}
