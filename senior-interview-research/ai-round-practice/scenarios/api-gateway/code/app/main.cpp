// gwroute report <access.log>
// gwroute route <routes> <requests>      (requests: one "METHOD target" per line)
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "accesslog.hpp"
#include "router.hpp"
#include "routes.hpp"
#include "traffic.hpp"

static std::string slurp(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error(std::string("cannot open ") + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "";
    if (!((mode == "report" && argc == 3) || (mode == "route" && argc == 4))) {
        std::cerr << "usage: gwroute report <access.log>\n       gwroute route <routes> <requests>\n";
        return 2;
    }
    try {
        if (mode == "report") {
            AccessLog log = parse_access_log(slurp(argv[2]));
            if (log.malformed) std::cerr << argv[2] << ": skipped " << log.malformed << " malformed lines\n";
            std::cout << traffic_report(log);
            return 0;
        }
        RouteTable table = build_table(parse_routes(slurp(argv[2])));
        std::istringstream requests(slurp(argv[3]));
        std::string method, target;
        while (requests >> method >> target) {
            RouteResult r = route_request(table, method, target);
            std::cout << method << " " << target << " -> ";
            if (r.status == 200) {
                std::cout << table.routes[r.route].upstream;
                for (std::size_t i = 0; i < r.params.size(); ++i)
                    std::cout << (i ? ", " : " (") << r.params[i].first << "=" << r.params[i].second;
                std::cout << (r.params.empty() ? "" : ")") << "\n";
            } else if (r.status == 405) {
                std::cout << "405 allow:";
                for (const std::string& m : r.allow) std::cout << " " << m;
                std::cout << "\n";
            } else {
                std::cout << r.status << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
