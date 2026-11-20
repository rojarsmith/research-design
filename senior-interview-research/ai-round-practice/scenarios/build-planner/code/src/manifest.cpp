#include "manifest.hpp"

#include <unordered_set>

#include "duration.hpp"

namespace {

std::vector<std::string_view> split_ws(std::string_view line) {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
        std::size_t start = i;
        while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r') ++i;
        if (i > start) out.push_back(line.substr(start, i - start));
    }
    return out;
}

std::vector<std::string> split_commas(std::string_view list) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= list.size()) {
        std::size_t comma = list.find(',', start);
        if (comma == std::string_view::npos) comma = list.size();
        if (comma > start) out.emplace_back(list.substr(start, comma - start));
        start = comma + 1;
    }
    return out;
}

std::string at_line(int line, const std::string& msg) { return "line " + std::to_string(line) + ": " + msg; }

}  // namespace

Manifest parse_manifest(std::string_view text) {
    Manifest m;
    std::unordered_set<std::string> seen;
    int line_no = 0;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++line_no;

        if (auto hash = line.find('#'); hash != std::string_view::npos) line = line.substr(0, hash);
        auto words = split_ws(line);
        if (words.empty()) continue;
        if (words[0] != "target") throw ManifestError(at_line(line_no, "expected 'target', got '" + std::string(words[0]) + "'"));
        if (words.size() < 3 || words.size() > 4) throw ManifestError(at_line(line_no, "expected: target <name> <duration> [deps=...]"));

        Target t;
        t.name = std::string(words[1]);
        t.line = line_no;
        try {
            t.duration_ms = parse_duration_ms(words[2]);
        } catch (const std::exception& e) {
            throw ManifestError(at_line(line_no, e.what()));
        }
        if (words.size() == 4) {
            if (words[3].substr(0, 5) != "deps=") throw ManifestError(at_line(line_no, "expected deps=..."));
            t.deps = split_commas(words[3].substr(5));
        }
        if (!seen.insert(t.name).second) throw ManifestError(at_line(line_no, "duplicate target '" + t.name + "'"));
        m.targets.push_back(std::move(t));
    }
    return m;
}
