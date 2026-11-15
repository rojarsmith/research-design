#include "rules.hpp"

#include <cctype>

namespace {

std::int64_t parse_span_ms(std::string_view text) {
    std::size_t i = 0;
    while (i < text.size() && std::isdigit((unsigned char)text[i])) ++i;
    if (i == 0) throw RuleError("bad duration '" + std::string(text) + "'");
    std::int64_t value = std::stoll(std::string(text.substr(0, i)));
    std::string_view unit = text.substr(i);
    if (unit == "ms") return value;
    if (unit == "s") return value * 1000;
    if (unit == "m") return value * 60000;
    if (unit == "h") return value * 3600000;
    throw RuleError("bad duration '" + std::string(text) + "'");
}

}  // namespace

std::vector<Rule> parse_rules(std::string_view text) {
    std::vector<Rule> rules;
    int line_no = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++line_no;
        if (auto h = line.find('#'); h != std::string_view::npos) line = line.substr(0, h);

        std::vector<std::string_view> words;
        std::size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && std::isspace((unsigned char)line[i])) ++i;
            std::size_t start = i;
            while (i < line.size() && !std::isspace((unsigned char)line[i])) ++i;
            if (i > start) words.push_back(line.substr(start, i - start));
        }
        if (words.empty()) continue;
        std::string where = "line " + std::to_string(line_no) + ": ";
        if (words[0] != "rule" || words.size() < 2) throw RuleError(where + "expected: rule <name> key=value ...");

        Rule r;
        r.name = std::string(words[1]);
        try {
            for (std::size_t k = 2; k < words.size(); ++k) {
                auto eq = words[k].find('=');
                if (eq == std::string_view::npos) throw RuleError("expected key=value, got '" + std::string(words[k]) + "'");
                std::string_view key = words[k].substr(0, eq), value = words[k].substr(eq + 1);
                if (key == "service") r.service = std::string(value);
                else if (key == "code") r.code = std::string(value);
                else if (key == "level") {
                    auto lv = parse_level(value);
                    if (!lv) throw RuleError("unknown level '" + std::string(value) + "'");
                    r.min_level = *lv;
                } else if (key == "threshold") r.threshold = std::stoi(std::string(value));
                else if (key == "window") r.window_ms = parse_span_ms(value);
                else if (key == "cooldown") r.cooldown_ms = parse_span_ms(value);
                else throw RuleError("unknown key '" + std::string(key) + "'");
            }
        } catch (const RuleError& e) {
            throw RuleError(where + e.what());
        } catch (const std::exception&) {
            throw RuleError(where + "bad number");
        }
        if (r.threshold < 1 || r.window_ms <= 0) throw RuleError(where + "threshold must be >= 1 and window > 0");
        rules.push_back(std::move(r));
    }
    return rules;
}

bool rule_matches(const Rule& rule, const Event& e) {
    return (rule.service == "*" || rule.service == e.service) && e.level >= rule.min_level &&
           (rule.code == "*" || rule.code == e.code);
}
