// Existing tests for gwroute. Do not edit them to make them pass.
#include <string>
#include <vector>

#include "accesslog.hpp"
#include "check.hpp"
#include "pathnorm.hpp"
#include "routes.hpp"
#include "traffic.hpp"

namespace {

using Segs = std::vector<std::string>;

template <class E, class F>
std::string error_of(F&& f) {
    try {
        f();
    } catch (const E& e) {
        return e.what();
    }
    return "(no error)";
}

}  // namespace

TEST("normalize_path drops empty and dot segments", 1) {
    EXPECT_EQ(normalize_path("/a//b/./c/"), (Segs{"a", "b", "c"}));
    EXPECT_EQ(normalize_path("/a/b/../c"), (Segs{"a", "c"}));
    EXPECT_EQ(normalize_path("/../a"), (Segs{"a"}));
    EXPECT_EQ(normalize_path("a/b"), (Segs{"a", "b"}));
    EXPECT_EQ(normalize_path("/"), Segs{});
    EXPECT_EQ(normalize_path(""), Segs{});
}

TEST("normalize_path drops the query string and the fragment", 1) {
    EXPECT_EQ(normalize_path("/a/b?x=1#frag"), (Segs{"a", "b"}));
    EXPECT_EQ(normalize_path("/a?b/c"), (Segs{"a"}));
    EXPECT_EQ(normalize_path("/a#b?c"), (Segs{"a"}));
    EXPECT_EQ(normalize_path("/?only=query"), Segs{});
}

TEST("normalize_path decodes each segment on its own", 1) {
    EXPECT_EQ(normalize_path("/caf%C3%A9/%7Ebob"), (Segs{"caf\xC3\xA9", "~bob"}));
    EXPECT_EQ(normalize_path("/files/q3%2Freport.pdf"), (Segs{"files", "q3/report.pdf"}));
    EXPECT_EQ(normalize_path("/docs/why%3F"), (Segs{"docs", "why?"}));
    EXPECT_EQ(normalize_path("/a/%2E%2E/b"), (Segs{"b"}));
}

TEST("format_path escapes what cannot appear in a segment", 1) {
    EXPECT_EQ(format_path({"files", "q3/report.pdf"}), std::string("/files/q3%2Freport.pdf"));
    EXPECT_EQ(format_path({}), std::string("/"));
    EXPECT_EQ(format_path({"a b", "100%", "x?#"}), std::string("/a%20b/100%25/x%3F%23"));
    EXPECT_EQ(format_path({"caf\xC3\xA9"}), std::string("/caf%C3%A9"));
}

TEST("normalize_path rejects bad escapes", 1) {
    for (const char* bad : {"/a%G1", "/a%4", "/a%", "/%%41"}) {
        bool threw = false;
        try { normalize_path(bad); } catch (const PathError&) { threw = true; }
        if (!threw) check::fail(__FILE__, __LINE__, std::string("accepted bad target \"") + bad + "\"");
    }
}

TEST("parse_routes reads methods, patterns and upstreams", 1) {
    auto routes = parse_routes(check::read_file("data/routes.conf"));
    EXPECT_EQ(routes.size(), std::size_t(13));
    EXPECT_EQ(routes[0].pattern, std::string("/"));
    EXPECT_TRUE(routes[0].segments.empty());
    EXPECT_EQ(routes[0].line, 3);
    EXPECT_EQ(routes[7].method, std::string("ANY"));
    const Route& r = routes[8];
    EXPECT_EQ(r.upstream, std::string("orders"));
    EXPECT_EQ(r.segments.size(), std::size_t(4));
    if (r.segments.size() == 4) {
        EXPECT_TRUE(r.segments[1].kind == SegKind::Param);
        EXPECT_EQ(r.segments[1].text, std::string("id"));
        EXPECT_TRUE(r.segments[2].kind == SegKind::Literal);
        EXPECT_EQ(r.segments[3].text, std::string("order"));
    }
    EXPECT_TRUE(routes[9].segments.back().kind == SegKind::Wildcard);
    EXPECT_EQ(routes[9].segments.back().text, std::string("path"));
}

TEST("parse_routes reports the failing line", 1) {
    auto err = [](const char* text) { return error_of<RouteError>([&] { parse_routes(text); }); };
    EXPECT_EQ(err("route GET users x\n"), std::string("line 1: pattern must start with '/': 'users'"));
    EXPECT_EQ(err("\nroute GET /a/*rest/b x\n"), std::string("line 2: wildcard must be the last segment in '/a/*rest/b'"));
    EXPECT_EQ(err("route FETCH /a x\n"), std::string("line 1: unknown method 'FETCH'"));
    EXPECT_EQ(err("route GET /a/:id/:id x\n"), std::string("line 1: parameter 'id' appears twice in '/a/:id/:id'"));
    EXPECT_EQ(err("route GET /a//b x\n"), std::string("line 1: empty segment in '/a//b'"));
    EXPECT_EQ(err("# ok\nroute GET /a x y\n"), std::string("line 2: expected: route <METHOD> <pattern> <upstream>"));
}

TEST("parse_access_log skips malformed lines", 1) {
    AccessLog log = parse_access_log(check::read_file("data/access.log"));
    EXPECT_EQ(log.requests.size(), std::size_t(22));
    EXPECT_EQ(log.malformed, 1);
    EXPECT_EQ(log.requests[3].target, std::string("/files/q3%2Freport.pdf"));
    EXPECT_EQ(log.requests[3].bytes, std::int64_t(1048576));
    EXPECT_EQ(log.requests.back().ms, 4);
    AccessLog odd = parse_access_log("GET /a 200 1 1\nGET /a 99 1 1\nget /a 200 1 1\nGET /a 200 -1 1\nGET /a 200 1\n");
    EXPECT_EQ(odd.requests.size(), std::size_t(1));
    EXPECT_EQ(odd.malformed, 4);
}

TEST("traffic report for the sample log", 1) {
    EXPECT_EQ(traffic_report(parse_access_log(check::read_file("data/access.log"))), std::string(
        "Requests: 22 (1 with an invalid target)\n"
        "Status: 2xx 16, 3xx 1, 4xx 4, 5xx 1\n"
        "Bytes: 2127414\n"
        "Top paths:\n"
        "  /users/42               5\n"
        "  /health                 4\n"
        "  /files/q3%2Freport.pdf  3\n"
        "  /docs/why%3F            2\n"
        "  /search                 2\n"));
}
