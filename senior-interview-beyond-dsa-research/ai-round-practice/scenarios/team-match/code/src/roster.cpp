#include "roster.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace {

std::vector<std::string_view> words_of(std::string_view line) {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
        std::size_t start = i;
        while (i < line.size() && !std::isspace((unsigned char)line[i])) ++i;
        if (i > start) out.push_back(line.substr(start, i - start));
    }
    return out;
}

bool is_id(std::string_view s) {
    if (s.empty() || s.size() > 40 || !std::islower((unsigned char)s[0])) return false;
    for (char c : s)
        if (!(std::islower((unsigned char)c) || std::isdigit((unsigned char)c) || c == '-' || c == '_')) return false;
    return true;
}

std::string_view value_of(std::string_view word, std::string_view key) {
    if (word.size() <= key.size() || word.substr(0, key.size()) != key || word[key.size()] != '=')
        throw RosterError("expected " + std::string(key) + "=..., got '" + std::string(word) + "'");
    return word.substr(key.size() + 1);
}

bool is_time(std::string_view s) {
    const char* shape = "dddd-dd-ddTdd:dd";
    if (s.size() != 16) return false;
    for (std::size_t i = 0; i < 16; ++i)
        if (shape[i] == 'd' ? !std::isdigit((unsigned char)s[i]) : s[i] != shape[i]) return false;
    return true;
}

}  // namespace

int Roster::find_team(std::string_view id) const {
    auto it = team_index.find(std::string(id));
    return it == team_index.end() ? -1 : it->second;
}

int Roster::find_intern(std::string_view id) const {
    auto it = intern_index.find(std::string(id));
    return it == intern_index.end() ? -1 : it->second;
}

Roster parse_roster(std::string_view text) {
    Roster r;
    std::unordered_set<long long> interviewed;  // team * 2^32 + intern
    int line_no = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++line_no;
        if (auto h = line.find('#'); h != std::string_view::npos) line = line.substr(0, h);
        auto w = words_of(line);
        if (w.empty()) continue;
        try {
            if (w[0] == "team") {
                if (w.size() != 3 || !is_id(w[1])) throw RosterError("expected: team <id> cap=<n>");
                std::string_view cap = value_of(w[2], "cap");
                if (cap.empty() || cap.size() > 5 || !std::all_of(cap.begin(), cap.end(), [](char c) { return c >= '0' && c <= '9'; }))
                    throw RosterError("bad capacity '" + std::string(cap) + "'");
                Team t{std::string(w[1]), std::stoi(std::string(cap))};
                if (!r.team_index.emplace(t.id, (int)r.teams.size()).second) throw RosterError("team " + t.id + " is declared twice");
                r.teams.push_back(std::move(t));
            } else if (w[0] == "intern") {
                if (w.size() != 4 || !is_id(w[1])) throw RosterError("expected: intern <id> applied=<time> prefs=<teams>");
                Intern in{std::string(w[1]), std::string(value_of(w[2], "applied")), {}};
                if (!is_time(in.applied)) throw RosterError("bad time '" + in.applied + "'");
                std::string_view prefs = value_of(w[3], "prefs");
                if (prefs != "-") {
                    std::size_t start = 0;
                    while (start <= prefs.size()) {
                        std::size_t comma = prefs.find(',', start);
                        if (comma == std::string_view::npos) comma = prefs.size();
                        std::string_view id = prefs.substr(start, comma - start);
                        start = comma + 1;
                        int t = r.find_team(id);
                        if (t < 0) throw RosterError("unknown team '" + std::string(id) + "'");
                        in.prefs.push_back(t);
                    }
                    std::vector<int> sorted = in.prefs;
                    std::sort(sorted.begin(), sorted.end());
                    if (auto d = std::adjacent_find(sorted.begin(), sorted.end()); d != sorted.end())
                        throw RosterError(in.id + " lists " + r.teams[*d].id + " twice");
                }
                if (!r.intern_index.emplace(in.id, (int)r.interns.size()).second)
                    throw RosterError("intern " + in.id + " is declared twice");
                r.interns.push_back(std::move(in));
            } else if (w[0] == "marks") {
                if (w.size() != 4) throw RosterError("expected: marks <team> <intern> <marks>");
                int t = r.find_team(w[1]), i = r.find_intern(w[2]);
                if (t < 0) throw RosterError("unknown team '" + std::string(w[1]) + "'");
                if (i < 0) throw RosterError("unknown intern '" + std::string(w[2]) + "'");
                const auto& prefs = r.interns[i].prefs;
                if (std::find(prefs.begin(), prefs.end(), t) == prefs.end())
                    throw RosterError(std::string(w[2]) + " did not apply to " + std::string(w[1]));
                if (!interviewed.insert((long long)t << 32 | i).second)
                    throw RosterError(std::string(w[2]) + " already has marks from " + std::string(w[1]));
                r.interviews.push_back({t, i, parse_marks(w[3])});
            } else {
                throw RosterError("expected 'team', 'intern' or 'marks', got '" + std::string(w[0]) + "'");
            }
        } catch (const std::exception& e) {
            throw RosterError("line " + std::to_string(line_no) + ": " + e.what());
        }
    }
    return r;
}
