// Existing tests for logalerts. Do not edit them to make them pass.
#include <string>
#include <vector>

#include "check.hpp"
#include "event.hpp"
#include "merge.hpp"
#include "rules.hpp"
#include "summary.hpp"
#include "timeparse.hpp"

namespace {

std::vector<Event> sample_timeline() {
    std::vector<LogFile> files;
    for (const char* f : {"data/api.log", "data/billing.log", "data/worker.log"})
        files.push_back(parse_log(check::read_file(f)));
    return merge_logs(files);
}

}  // namespace

TEST("parse_timestamp_ms: whole seconds and milliseconds", 1) {
    EXPECT_EQ(parse_timestamp_ms("1970-01-01T00:00:00Z"), std::int64_t(0));
    EXPECT_EQ(parse_timestamp_ms("2000-02-29T23:59:59Z"), std::int64_t(951868799000));
    EXPECT_EQ(parse_timestamp_ms("2026-11-15T10:00:00Z"), std::int64_t(1794736800000));
    EXPECT_EQ(parse_timestamp_ms("2026-11-15T10:00:00.480Z"), std::int64_t(1794736800480));
}

TEST("parse_timestamp_ms rejects malformed timestamps", 1) {
    for (const char* bad : {"2026-11-15 10:00:00Z", "2026-13-01T00:00:00Z", "2026-11-15T10:00:00", "2026-11-15T10:00:00.Z",
                            "2026-11-15T24:00:00Z", "2026-11-15T10:00:00Zjunk", ""}) {
        bool threw = false;
        try { parse_timestamp_ms(bad); } catch (const TimeError&) { threw = true; }
        if (!threw) check::fail(__FILE__, __LINE__, std::string("accepted bad timestamp \"") + bad + "\"");
    }
}

TEST("format_timestamp round-trips", 1) {
    EXPECT_EQ(format_timestamp(0), std::string("1970-01-01T00:00:00.000Z"));
    EXPECT_EQ(format_timestamp(1794736800480), std::string("2026-11-15T10:00:00.480Z"));
    EXPECT_EQ(parse_timestamp_ms(format_timestamp(951868799123)), std::int64_t(951868799123));
}

TEST("parse_log skips malformed lines and keeps the message", 1) {
    LogFile f = parse_log(check::read_file("data/api.log"));
    EXPECT_EQ(f.events.size(), std::size_t(5));
    EXPECT_EQ(f.malformed, 1);
    EXPECT_EQ(f.events[1].code, std::string("E5003"));
    EXPECT_EQ(f.events[1].message, std::string("upstream timeout"));
    EXPECT_TRUE(f.events[1].level == Level::Error);
    EXPECT_TRUE(!parse_event("2026-11-15T10:00:00Z api LOUD E1 x").has_value());
}

TEST("parse_rules reads every field and reports bad lines", 1) {
    auto rules = parse_rules(check::read_file("data/rules.txt"));
    EXPECT_EQ(rules.size(), std::size_t(3));
    EXPECT_EQ(rules[1].name, std::string("declines"));
    EXPECT_EQ(rules[1].service, std::string("billing"));
    EXPECT_EQ(rules[1].window_ms, std::int64_t(5000));
    EXPECT_EQ(rules[0].cooldown_ms, std::int64_t(60000));
    EXPECT_TRUE(rules[2].min_level == Level::Fatal);
    std::string err;
    try { parse_rules("# header\nrule x level=LOUD window=1s\n"); } catch (const RuleError& e) { err = e.what(); }
    EXPECT_EQ(err, std::string("line 2: unknown level 'LOUD'"));
}

TEST("merge_logs keeps file order for equal timestamps", 1) {
    LogFile a = parse_log("2026-11-15T10:00:01Z a INFO X1 first\n2026-11-15T10:00:00Z a INFO X2 early\n");
    LogFile b = parse_log("2026-11-15T10:00:01Z b INFO X3 second\n");
    auto t = merge_logs({a, b});
    std::vector<std::string> codes;
    for (auto& e : t) codes.push_back(e.code);
    EXPECT_EQ(codes, (std::vector<std::string>{"X2", "X1", "X3"}));
}

TEST("merged timeline of the sample logs", 1) {
    std::vector<std::string> got;
    for (const Event& e : sample_timeline()) got.push_back(e.service + ":" + e.code);
    EXPECT_EQ(got, (std::vector<std::string>{
                       "api:I1000", "worker:I1000", "billing:I1000", "api:E5003", "billing:E4100", "api:W2001",
                       "worker:E5003", "api:E5003", "billing:E4100", "worker:I1000", "api:I1000", "billing:F9000"}));
}

TEST("on-call summary of the sample logs", 1) {
    EXPECT_EQ(summary_report(sample_timeline()), std::string(
        "Events: 12\n"
        "From 2026-11-15T10:00:00.120Z to 2026-11-15T10:00:04.000Z\n"
        "By level: DEBUG 0, INFO 5, WARN 1, ERROR 5, FATAL 1\n"
        "Top error codes: E5003 x3, E4100 x2, F9000 x1\n"));
}
