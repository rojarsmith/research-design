// logalerts <rules> <log> [<log> ...]
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "alerts.hpp"
#include "event.hpp"
#include "merge.hpp"
#include "rules.hpp"
#include "summary.hpp"
#include "timeparse.hpp"

static std::string slurp(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error(std::string("cannot open ") + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: logalerts <rules> <log> [<log> ...]\n";
        return 2;
    }
    try {
        std::vector<Rule> rules = parse_rules(slurp(argv[1]));
        std::vector<LogFile> files;
        for (int i = 2; i < argc; ++i) {
            files.push_back(parse_log(slurp(argv[i])));
            if (files.back().malformed) std::cerr << argv[i] << ": skipped " << files.back().malformed << " malformed lines\n";
        }
        std::vector<Event> timeline = merge_logs(files);
        std::cout << summary_report(timeline);
        for (const Alert& a : evaluate(timeline, rules))
            std::cout << "ALERT " << a.rule << " " << a.service << " " << a.code << " at " << format_timestamp(a.at_ms)
                      << " (" << a.count << " events)\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
