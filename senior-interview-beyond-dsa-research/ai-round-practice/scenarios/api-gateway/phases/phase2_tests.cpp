// Phase 2 tests: build_table and route_request.
#include <set>
#include <string>
#include <vector>

#include "check.hpp"
#include "router.hpp"
#include "routes.hpp"

namespace {

RouteTable table_of(const std::string& text) { return build_table(parse_routes(text)); }

// "upstream pattern name=value ...", "405 allow=GET,POST", "404" or "400".
std::string brief(const RouteTable& t, const RouteResult& r) {
    if (r.status == 200) {
        if (r.route < 0 || r.route >= (int)t.routes.size()) return "200 with a bad route index";
        std::string s = t.routes[r.route].upstream + " " + t.routes[r.route].pattern;
        for (const auto& [name, value] : r.params) s += " " + name + "=" + value;
        return s;
    }
    if (r.status == 405) {
        std::string s = "405 allow=";
        for (std::size_t i = 0; i < r.allow.size(); ++i) s += (i ? "," : "") + r.allow[i];
        return s;
    }
    return std::to_string(r.status);
}

std::string go(const RouteTable& t, const char* method, const char* target) {
    return brief(t, route_request(t, method, target));
}

std::uint64_t results_hash(const std::vector<RouteResult>& results) {
    std::string text;
    for (const RouteResult& r : results) {
        text += std::to_string(r.status) + ":" + std::to_string(r.route) + ":";
        for (const auto& [name, value] : r.params) text += name + "=" + value + ";";
        text += ":";
        for (const std::string& m : r.allow) text += m + ",";
        text += "\n";
    }
    return check::fnv1a(text);
}

// Random table and requests over a small set of literals, so that patterns overlap a lot.
const char* kLiterals[] = {"a", "b", "c", "users", "orders", "v1", "v2", "x", "items", "me"};
const char* kMethods[] = {"GET", "POST", "PUT", "PATCH", "DELETE", "ANY"};

std::string random_routes(int n, check::Rng& rng) {
    std::set<std::string> shapes;
    std::string text;
    while ((int)shapes.size() < n) {
        int depth = (int)rng.range(0, 4);
        std::string method = kMethods[rng.range(0, 9) == 0 ? 5 : rng.range(0, 4)];
        std::string pattern, shape = method + " ";
        for (int d = 0; d < depth; ++d) {
            long long k = d == 0 ? 0 : rng.range(0, 9);
            if (k < 6) {
                std::string lit = kLiterals[rng.range(0, 9)];
                pattern += "/" + lit;
                shape += "/" + lit;
            } else if (k < 9 || d + 1 < depth) {
                pattern += "/:p" + std::to_string(d);
                shape += "/:";
            } else {
                pattern += "/*rest";
                shape += "/*";
            }
        }
        if (depth == 0) {
            pattern = "/";
            shape += "/";
        }
        if (!shapes.insert(shape).second) continue;
        text += "route " + method + " " + pattern + " u" + std::to_string(shapes.size()) + "\n";
    }
    return text;
}

std::vector<std::pair<std::string, std::string>> random_requests(int n, check::Rng& rng) {
    const char* methods[] = {"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD"};
    const char* extra[] = {"zz", "42", "%61", "us%65rs", "", ".", "..", "q%2Fr", "b%g1"};
    std::vector<std::pair<std::string, std::string>> out;
    for (int i = 0; i < n; ++i) {
        std::string target;
        int depth = (int)rng.range(0, 5);
        for (int d = 0; d < depth; ++d)
            target += "/" + std::string(rng.range(0, 4) == 0 ? extra[rng.range(0, 8)] : kLiterals[rng.range(0, 9)]);
        if (rng.range(0, 5) == 0) target += "/";
        if (rng.range(0, 7) == 0) target += "?q=" + std::to_string(rng.range(0, 99));
        if (target.empty() || rng.range(0, 9) == 0) target = "/" + target;
        out.push_back({methods[rng.range(0, 5)], target});
    }
    return out;
}

}  // namespace

TEST("a literal beats a parameter", 2) {
    RouteTable t = table_of(check::read_file("data/routes.conf"));
    EXPECT_EQ(go(t, "GET", "/users/me"), std::string("profile /users/me"));
    EXPECT_EQ(go(t, "GET", "/users/42"), std::string("users /users/:id id=42"));
    EXPECT_EQ(go(t, "GET", "/users/42/orders/7"), std::string("orders /users/:id/orders/:order id=42 order=7"));
    EXPECT_EQ(go(t, "GET", "/users"), std::string("users /users"));
    EXPECT_EQ(go(t, "POST", "/users"), std::string("signup /users"));
}

TEST("the first segment that differs decides", 2) {
    RouteTable t = table_of(
        "route GET /a/:x/c      first\n"
        "route GET /a/b/*rest   second\n"
        "route GET /a/:x/*rest  third\n");
    EXPECT_EQ(go(t, "GET", "/a/b/c"), std::string("second /a/b/*rest rest=c"));
    EXPECT_EQ(go(t, "GET", "/a/z/c"), std::string("first /a/:x/c x=z"));
    EXPECT_EQ(go(t, "GET", "/a/z/d/e"), std::string("third /a/:x/*rest x=z rest=d/e"));
}

TEST("an exact method beats ANY only on the same pattern", 2) {
    RouteTable t = table_of(
        "route ANY /a/:x  any-x\n"
        "route GET /a/:x  get-x\n"
        "route ANY /a/b   any-b\n");
    EXPECT_EQ(go(t, "GET", "/a/c"), std::string("get-x /a/:x x=c"));
    EXPECT_EQ(go(t, "POST", "/a/c"), std::string("any-x /a/:x x=c"));
    EXPECT_EQ(go(t, "GET", "/a/b"), std::string("any-b /a/b"));
}

TEST("a wildcard takes one or more segments", 2) {
    RouteTable t = table_of(check::read_file("data/routes.conf"));
    EXPECT_EQ(go(t, "GET", "/files/q3%2Freport.pdf"), std::string("files /files/*path path=q3/report.pdf"));
    EXPECT_EQ(go(t, "GET", "/files/a/b//c/"), std::string("files /files/*path path=a/b/c"));
    EXPECT_EQ(go(t, "GET", "/files"), std::string("404"));
    EXPECT_EQ(go(t, "GET", "/docs/api/v2"), std::string("docs-api /docs/api/:version version=v2"));
    EXPECT_EQ(go(t, "GET", "/docs/api/v2/auth"), std::string("docs /docs/*page page=api/v2/auth"));
    EXPECT_EQ(go(t, "GET", "/docs/api"), std::string("docs /docs/*page page=api"));
}

TEST("405 when the path matches but the method does not", 2) {
    RouteTable t = table_of(check::read_file("data/routes.conf"));
    EXPECT_EQ(go(t, "POST", "/users/42"), std::string("405 allow=DELETE,GET"));
    EXPECT_EQ(go(t, "PUT", "/users/me"), std::string("405 allow=DELETE,GET"));
    EXPECT_EQ(go(t, "DELETE", "/users/me"), std::string("users-admin /users/:id id=me"));
    EXPECT_EQ(go(t, "PATCH", "/users/42/orders"), std::string("orders /users/:id/orders id=42"));
    EXPECT_EQ(go(t, "HEAD", "/health"), std::string("405 allow=GET"));
    EXPECT_EQ(go(t, "get", "/health"), std::string("405 allow=GET"));
}

TEST("targets are normalized first; bad targets are 400", 2) {
    RouteTable t = table_of(check::read_file("data/routes.conf"));
    EXPECT_EQ(go(t, "GET", "//users/./42/?expand=orders"), std::string("users /users/:id id=42"));
    EXPECT_EQ(go(t, "GET", "/users/j%C3%B6rg"), std::string("users /users/:id id=j\xC3\xB6rg"));
    EXPECT_EQ(go(t, "GET", "/users/a%2Fb"), std::string("users /users/:id id=a/b"));
    EXPECT_EQ(go(t, "GET", "/"), std::string("web /"));
    EXPECT_EQ(go(t, "GET", "/x/.."), std::string("web /"));
    EXPECT_EQ(go(t, "GET", "/bad%zz"), std::string("400"));
    EXPECT_EQ(go(t, "GET", "/nope"), std::string("404"));
    RouteResult bad = route_request(t, "GET", "/bad%zz");
    EXPECT_EQ(bad.route, -1);
}

TEST("duplicate routes are rejected, overlapping ones are not", 2) {
    bool threw = false;
    try { table_of("route GET /a/:x one\nroute GET /a/:y two\n"); } catch (const RouteError&) { threw = true; }
    EXPECT_TRUE(threw);
    threw = false;
    try { table_of("route GET /a/:x one\nroute ANY /a/:y two\nroute GET /a/*x three\nroute POST /a/:z four\n"); }
    catch (const RouteError&) { threw = true; }
    EXPECT_TRUE(!threw);
    RouteTable empty = build_table({});
    EXPECT_EQ(go(empty, "GET", "/"), std::string("404"));
}

TEST("medium random route tables", 2) {
    struct Case { int routes; int requests; std::uint64_t seed; int ok; std::uint64_t hash; };
    const Case cases[] = {
        {60, 3000, 41, 1166, 12224625933604268356ULL},
        {300, 20000, 42, 11313, 15203938021778985406ULL},
    };
    for (const Case& c : cases) {
        check::Rng rng(c.seed);
        RouteTable t = table_of(random_routes(c.routes, rng));
        std::vector<RouteResult> results;
        int ok = 0;
        for (const auto& [method, target] : random_requests(c.requests, rng)) {
            results.push_back(route_request(t, method, target));
            ok += results.back().status == 200;
        }
        EXPECT_EQ(ok, c.ok);
        EXPECT_EQ(results_hash(results), c.hash);
    }
}
