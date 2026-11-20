// netcheck report <netlist>
// netcheck check <netlist> <layout>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "connectivity.hpp"
#include "layout.hpp"
#include "netlist.hpp"
#include "pinreport.hpp"

static std::string slurp(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error(std::string("cannot open ") + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "";
    if (!((mode == "report" && argc == 3) || (mode == "check" && argc == 4))) {
        std::cerr << "usage: netcheck report <netlist>\n       netcheck check <netlist> <layout>\n";
        return 2;
    }
    try {
        Netlist n = parse_netlist(slurp(argv[2]));
        if (mode == "report") {
            std::cout << pin_report(n);
            return 0;
        }
        Board b = load_board(n, parse_layout(slurp(argv[3])));
        CheckResult r = check_board(n, b);
        for (const Open& o : r.opens) {
            std::cout << "OPEN  " << o.net << ":";
            for (const auto& island : o.islands) {
                std::cout << " [";
                for (std::size_t i = 0; i < island.size(); ++i) std::cout << (i ? " " : "") << island[i];
                std::cout << "]";
            }
            std::cout << "\n";
        }
        for (const Short& s : r.shorts) {
            std::cout << "SHORT";
            for (const std::string& net : s.nets) std::cout << " " << net;
            std::cout << "\n";
        }
        std::cout << r.opens.size() << " opens, " << r.shorts.size() << " shorts\n";
        return r.opens.empty() && r.shorts.empty() ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}
