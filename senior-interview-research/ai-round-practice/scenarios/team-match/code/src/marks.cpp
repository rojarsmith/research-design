#include "marks.hpp"

#include <cstdio>
#include <numeric>

Marks parse_marks(std::string_view text) {
    Marks out;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t comma = text.find(',', start);
        if (comma == std::string_view::npos) comma = text.size();
        std::string_view m = text.substr(start, comma - start);
        start = comma + 1;
        if (m.size() != 3 || m[1] != '.' || m[0] < '1' || m[0] > '5' || m[2] < '0' || m[2] > '9' || (m[0] == '5' && m[2] != '0'))
            throw MarksError("bad mark '" + std::string(m) + "' in '" + std::string(text) + "'");
        out.push_back(std::stod(std::string(m)));
    }
    if (out.size() > 9) throw MarksError("more than 9 marks in '" + std::string(text) + "'");
    return out;
}

double average(const Marks& marks) { return std::accumulate(marks.begin(), marks.end(), 0.0) / marks.size(); }

std::string format_average(const Marks& marks) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%.2f", average(marks));
    return buf;
}
