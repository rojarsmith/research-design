// Phase 3 tests: production-sized route tables and traffic. Each test writes its route
// table and requests to data/generated/ (open them: their shapes differ), parses them,
// and then times build_table plus route_request for every request. Parsing is not timed.
#include <string>
#include <utility>
#include <vector>

#include "check.hpp"
#include "router.hpp"
#include "routes.hpp"

namespace {

using Requests = std::vector<std::pair<std::string, std::string>>;

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

const char* kNouns[] = {"orders",  "items",    "carts",     "users",   "invoices", "refunds", "payments",
                        "shipments", "returns", "reviews",  "coupons", "stores",   "prices",  "stock",
                        "tags",    "notes",    "events",    "jobs",    "files",    "tokens"};
const char* kVerbs[] = {"GET", "POST", "PUT", "PATCH", "DELETE"};

// Service s of the wide table: 20 resources with 5 routes each, plus a static wildcard.
void add_service(std::string& text, int s) {
    std::string base = "/svc" + std::to_string(s) + "/v" + std::to_string(s % 3 + 1);
    for (int r = 0; r < 20; ++r) {
        std::string res = base + "/" + kNouns[r];
        text += "route GET " + res + " s" + std::to_string(s) + "\n";
        text += "route POST " + res + " s" + std::to_string(s) + "\n";
        text += "route GET " + res + "/:id s" + std::to_string(s) + "\n";
        text += "route PUT " + res + "/:id s" + std::to_string(s) + "\n";
        text += "route DELETE " + res + "/:id s" + std::to_string(s) + "\n";
    }
    text += "route GET /svc" + std::to_string(s) + "/static/*file s" + std::to_string(s) + "-cdn\n";
}

std::string service_target(check::Rng& rng, int services) {
    int s = (int)rng.range(0, services - 1);
    std::string t = "/svc" + std::to_string(s);
    long long kind = rng.range(0, 19);
    if (kind == 0) return t + "/static/js/app" + std::to_string(rng.range(0, 99)) + ".js";
    t += "/v" + std::to_string(kind == 1 ? rng.range(1, 3) : s % 3 + 1);
    t += "/" + std::string(kind == 2 ? "widgets" : kNouns[rng.range(0, 19)]);
    if (rng.range(0, 1)) t += "/" + std::to_string(rng.range(1, 999999));
    if (rng.range(0, 9) == 0) t += "?page=" + std::to_string(rng.range(1, 9));
    return t;
}

// 400 services, 40,400 routes, every service under its own first segment.
std::pair<std::string, Requests> wide() {
    check::Rng rng(601);
    std::string routes;
    for (int s = 0; s < 400; ++s) add_service(routes, s);
    Requests reqs;
    for (int i = 0; i < 300000; ++i) reqs.push_back({kVerbs[rng.range(0, 4)], service_target(rng, 400)});
    return {routes, reqs};
}

// One large API: 25,000 routes that all start with /api/v3/tenants/:tenant/.
// Many requests use a method the path does not allow.
std::pair<std::string, Requests> monolith() {
    check::Rng rng(602);
    const int resources = 1250;
    std::string routes;
    auto res_name = [](int r) { return std::string(kNouns[r % 20]) + std::to_string(r / 20); };
    for (int r = 0; r < resources; ++r) {
        std::string base = "/api/v3/tenants/:tenant/" + res_name(r);
        std::string up = " api-" + std::to_string(r % 37) + "\n";
        routes += "route GET " + base + up;
        routes += "route POST " + base + up;
        routes += "route GET " + base + "/:id" + up;
        routes += "route PATCH " + base + "/:id" + up;
        routes += "route DELETE " + base + "/:id" + up;
        routes += "route GET " + base + "/:id/history" + up;
        routes += "route GET " + base + "/:id/history/:version" + up;
        routes += "route ANY " + base + "/:id/actions/*action" + up;
        for (int k = 0; k < 4; ++k) {
            std::string sub = base + "/:id/" + kNouns[(r + 7 * k + 3) % 20];
            routes += "route GET " + sub + up;
            routes += "route POST " + sub + up;
            routes += "route GET " + sub + "/:sid" + up;
        }
    }
    routes += "route GET /api/v3/tenants/:tenant api-tenants\n";
    routes += "route GET /api/v3/tenants/:tenant/search api-search\n";
    Requests reqs;
    for (int i = 0; i < 300000; ++i) {
        int r = (int)rng.range(0, resources + 120);  // past the end: unknown resource
        std::string t = "/api/v3/tenants/t" + std::to_string(rng.range(1, 5000)) + "/" +
                        (r < resources ? res_name(r) : "legacy" + std::to_string(r));
        long long shape = rng.range(0, 9);
        if (shape >= 2) t += "/" + std::to_string(rng.range(1, 99999));
        if (shape == 4) t += "/history";
        if (shape == 5) t += "/actions/archive/now";
        if (shape >= 6 && r < resources) t += "/" + std::string(kNouns[(r + 7 * rng.range(0, 4) + 3) % 20]);
        if (shape >= 8) t += "/" + std::to_string(rng.range(1, 999));
        reqs.push_back({kVerbs[rng.range(0, 4)], t});
    }
    return {routes, reqs};
}

// A normal day for 20 services, plus 1,000 requests from a vulnerability scanner: targets of
// 12 to 20 KB full of repeated slashes and dot segments that cancel out.
std::pair<std::string, Requests> scanner() {
    check::Rng rng(603);
    std::string routes;
    for (int s = 0; s < 20; ++s) add_service(routes, s);
    const char* junk[] = {"//", "////", "/./", "/x/..", "/%2e", "//%2E/", "/y/%2E%2E", "///////"};
    Requests reqs;
    for (int i = 0; i < 101000; ++i) {
        std::string t = service_target(rng, 20);
        if (i % 101 == 100) {
            std::string noise;
            std::size_t len = (std::size_t)rng.range(12000, 20000);
            while (noise.size() < len) noise += junk[rng.range(0, 7)];
            t = noise + t;
        }
        reqs.push_back({kVerbs[rng.range(0, 4)], t});
    }
    return {routes, reqs};
}

void run_large(const std::string& name, const std::pair<std::string, Requests>& data, double limit_ms, int ok,
               std::uint64_t hash) {
    std::string routes_text = check::read_file(check::write_generated(name + "-routes.conf", data.first));
    std::string req_text;
    for (const auto& [m, t] : data.second) req_text += m + " " + t + "\n";
    std::string req_file = check::read_file(check::write_generated(name + "-requests.txt", req_text));

    std::vector<Route> routes = parse_routes(routes_text);
    Requests reqs;
    std::size_t pos = 0;
    while (pos < req_file.size()) {
        std::size_t nl = req_file.find('\n', pos);
        std::size_t sp = req_file.find(' ', pos);
        reqs.push_back({req_file.substr(pos, sp - pos), req_file.substr(sp + 1, nl - sp - 1)});
        pos = nl + 1;
    }
    std::vector<RouteResult> results;
    results.reserve(reqs.size());
    check::measure(limit_ms, [&] {
        RouteTable table = build_table(std::move(routes));
        for (const auto& [m, t] : reqs) results.push_back(route_request(table, m, t));
    });
    int got_ok = 0;
    for (const RouteResult& r : results) got_ok += r.status == 200;
    EXPECT_EQ(got_ok, ok);
    EXPECT_EQ(results_hash(results), hash);
}

}  // namespace

TEST("large-1 wide table: 400 services", 3) { run_large("large-1-wide", wide(), 1500, 133254, 2713305023478660295ULL); }

TEST("large-2 one big API under a shared prefix", 3) { run_large("large-2-monolith", monolith(), 1500, 113739, 16037923479918048327ULL); }

TEST("large-3 scanner traffic with very long targets", 3) { run_large("large-3-scanner", scanner(), 1500, 44736, 7597578710749171539ULL); }
